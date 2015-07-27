#include "parser.h"
#include "parser-internal.h"

#include <string.h>

#include "sym.h"
#include "ast.h"

static ast* parserS (parserCtx* ctx);

static ast* parserExpr (parserCtx* ctx);
static ast* parserPipe (parserCtx* ctx);
static ast* parserFnApp (parserCtx* ctx);
static ast* parserAtom ();

parserResult parse (sym* global, lexerCtx* lexer) {
    parserCtx ctx = parserInit(global, lexer);
    ast* tree = parserS(&ctx);

    parserResult result = {
        .tree = tree,
        .errors = ctx.errors
    };

    parserFree(&ctx);

    return result;
}

static ast* parserS (parserCtx* ctx) {
    return parserExpr(ctx);
}

static ast* parserExpr (parserCtx* ctx) {
    return parserPipe(ctx);
}

static ast* parserPipe (parserCtx* ctx) {
    ast* node = parserFnApp(ctx);

    while (try_match(ctx, "|")) {
        ast* fn = parserFnApp(ctx);
        node = astCreatePipeApp(node, fn);
    }

    return node;
}

static bool waiting_for_delim (parserCtx* ctx) {
    return waiting(ctx) && !see(ctx, "|") && !see(ctx, ",") && !see(ctx, ")") && !see(ctx, "]");
}

static ast* parserFnApp (parserCtx* ctx) {
    /*Filled iff there is a backtick function*/
    ast* fn = 0;

    vector(ast*) nodes = vectorInit(3, malloc);
    int exprs = 0;

    for (; waiting_for_delim(ctx); exprs++) {
        if (try_match(ctx, "`")) {
            if (fn) {
                error(ctx)("Multiple explicit functions in backticks: '%s'\n", ctx->current.buffer);
                vectorPush(&nodes, fn);
            }

            fn = parserAtom(ctx);
            match(ctx, "`");

        } else
            vectorPush(&nodes, parserAtom(ctx));
    }

    if (exprs == 1) {
        ast* node = vectorPop(&nodes);
        vectorFree(&nodes);
        return node;

    } else {
        /*If not explicitly marked in backticks, the last expr was the fn*/
        if (!fn)
            fn = vectorPop(&nodes);

        return astCreateFnApp(nodes, fn);
    }
}

static bool isPathToken (const char* str) {
    return strchr(str, '/') || strchr(str, '.');
}

static ast* parserAtom (parserCtx* ctx) {
    ast* node;

    /*List literal*/
    if (try_match(ctx, "[")) {
        vector(ast*) nodes = vectorInit(4, malloc);

        if (waiting_for(ctx, "]")) do {
            vectorPush(&nodes, parserExpr(ctx));
        } while (try_match(ctx, ","));

        node = astCreateListLit(nodes);

        match(ctx, "]");

    } else if (see_kind(ctx, tokenNormal)) {
        sym* symbol;

        if (isPathToken(ctx->current.buffer))
            node = astCreateFileLit(ctx->current.buffer);

        else if ((symbol = symLookup(ctx->scope, ctx->current.buffer)))
            node = astCreateSymbol(symbol);

        else
            node = astCreateStrLit(ctx->current.buffer);

        accept(ctx);

    } else {
        expected(ctx, "expression");
        node = astCreateInvalid();
    }

    return node;
}
