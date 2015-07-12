#include "parser.h"
#include "parser-internal.h"

#include <string.h>

#include "ast.h"

static ast* parserS (parserCtx* ctx);

static ast* parserPipe (parserCtx* ctx);
static ast* parserFnApp (parserCtx* ctx);
static ast* parserAtom ();

ast* parse (sym* global, lexerCtx* lexer) {
    parserCtx ctx = parserInit(global, lexer);
    ast* tree = parserS(&ctx);
    parserFree(&ctx);

    return tree;
}

static ast* parserS (parserCtx* ctx) {
    return parserPipe(ctx);
}

static ast* parserPipe (parserCtx* ctx) {
    ast* node = parserFnApp(ctx);

    while (try_match(ctx, "|")) {
        ast* fn = parserFnApp(ctx);

        vector nodes = vectorInit(1, malloc);
        vectorPush(&nodes, node);
        node = astCreateFnApp(fn, nodes);
    }

    return node;
}

static ast* parserFnApp (parserCtx* ctx) {
    /*Filled iff there is a backtick function*/
    ast* fn = 0;

    vector(ast*) nodes = vectorInit(3, malloc);
    int exprs = 0;

    for (; waiting_for(ctx, ")") && waiting_for(ctx, "|"); exprs++) {
        if (try_match(ctx, "`")) {
            if (fn) {
                error(ctx, "Multiple explicit functions in backticks");
                vectorPush(&nodes, fn);
            }

            fn = parserAtom(ctx);
            match(ctx, "`");

        } else
            vectorPush(&nodes, parserAtom(ctx));
    }

    if (exprs == 1) {
        ast* node = vectorGet(nodes, 0);
        vectorFree(&nodes);
        return node;

    } else {
        /*If not explicitly marked in backticks, the last expr was the fn*/
        if (!fn)
            fn = vectorPop(&nodes);

        return astCreateFnApp(fn, nodes);
    }
}

static ast* parserAtom (parserCtx* ctx) {
    ast* node = astCreateLitStr(ctx->current.buffer);

    if (see_kind(ctx, tokenNormal))
        accept(ctx);

    else
        expected(ctx, "expression");

    return node;
}
