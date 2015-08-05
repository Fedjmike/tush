#include "parser.h"
#include "parser-internal.h"

#include <string.h>

#include "paths.h"

#include "sym.h"
#include "ast.h"

static ast* parserS (parserCtx* ctx);

static ast* parserExpr (parserCtx* ctx);
static ast* parserBOP (parserCtx* ctx, int level);
static ast* parserFnApp (parserCtx* ctx);
static ast* parserAtom (parserCtx* ctx);
static ast* parserPath (parserCtx* ctx);

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
    ast* node = parserExpr(ctx);

    if (see_kind(ctx, tokenEOF))
        accept(ctx);

    else
        error(ctx)("Expected end of text, found '%s'\n", ctx->current.buffer);

    return node;
}

static ast* parserExpr (parserCtx* ctx) {
    return parserBOP(ctx, 0);
}

/**
 * BOP = Pipe
 * Pipe    = Logical  [{ "|" | "|>" Logical }]
 * Logical = Equality [{ "&&" | "||" Equality }]
 * Equality = Sum     [{ "==" | "!=" | "<" | "<=" | ">" | ">=" Sum }]
 * Sum     = Product  [{ "+" | "-" Product }]
 * Product = Exit     [{ "*" | "/" | "%" Exit }]
 * Exit = FnApp
 *
 * The grammar is ambiguous, but these operators are left associative,
 * which is to say:
 *    x op y op z == (x op y) op z
 */
static ast* parserBOP (parserCtx* ctx, int level) {
    /* (1) Operator precedence parsing!
      As all the productions above are essentially the same, with
      different operators, we can parse them with the same function.

      Read the comments in numbered order, this was (1)*/

    /* (5) Finally, once we reach the top level we escape the recursion
           into... */
    if (level == 5)
        return parserFnApp(ctx);

    /* (2) The left hand side is the production one level up*/
    ast* node = parserBOP(ctx, level+1);
    opKind op;

    /* (3) Accept operators associated with this level, and store
           which kind is found*/
    while (  level == 0
             ? (op =   try_match(ctx, "|") ? opPipe
                     : try_match(ctx, "|>") ? opWrite : opNull)
           : level == 1
             ? (op =   try_match(ctx, "&&") ? opLogicalAnd
                     : try_match(ctx, "||") ? opLogicalOr : opNull)
           : level == 2
             ? (op =   try_match(ctx, "==") ? opEqual
                     : try_match(ctx, "!=") ? opNotEqual
                     : try_match(ctx, "<") ? opLess
                     : try_match(ctx, "<=") ? opLessEqual
                     : try_match(ctx, ">") ? opGreater
                     : try_match(ctx, ">=") ? opGreaterEqual : opNull)
           : level == 3
             ? (op =   try_match(ctx, "+") ? opAdd
                     : try_match(ctx, "-") ? opSubtract : opNull)
           : level == 4
             ? (op =   try_match(ctx, "*") ? opMultiply
                     : try_match(ctx, "/") ? opDivide
                     : try_match(ctx, "%") ? opModulo : opNull)
           : (op = opNull)) {
        /* (4) Bundle it up with an RHS, also the level up*/
        ast* rhs = parserBOP(ctx, level+1);
        node = astCreateBOP(node, rhs, op);
    }

    return node;
}

static bool waiting_for_delim (parserCtx* ctx) {
    return waiting(ctx) && !see(ctx, "|") && !see(ctx, ",") && !see(ctx, ")") && !see(ctx, "]");
}

/**
 * FnApp = { Atom | ( "`" Atom "`" ) }
 *
 * A series of at least one expr-atom. If multiple, then the last is a
 * function to which the others are applied. Instead, one of them may
 * be explicitly marked in backticks as the function.
 */
static ast* parserFnApp (parserCtx* ctx) {
    /*Filled iff there is a backtick function*/
    ast* fn = 0;

    vector(ast*) nodes = vectorInit(3, malloc);
    int exprs = 0;

    /*Require at least one expr*/
    if (!see(ctx, "`")) {
        vectorPush(&nodes, parserAtom(ctx));
        exprs++;
    }

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
    return strchr(str, '/') || strchr(str, '.') || strchr(str, '*');
}

/**
 * Atom =   ( "(" [ Expr ] ")" )
 *        | ( "[" [{ Expr }] "]" )
 *        | Path | <Str> | <Symbol>
 */
static ast* parserAtom (parserCtx* ctx) {
    ast* node;

    if (try_match(ctx, "(")) {
        /*Empty brackets => unit literal*/
        if (see(ctx, ")"))
            node = astCreateUnitLit();

        else
            node = parserExpr(ctx);

        match(ctx, ")");

    /*List literal*/
    } else if (try_match(ctx, "[")) {
        vector(ast*) nodes = vectorInit(4, malloc);

        if (waiting_for(ctx, "]")) do {
            vectorPush(&nodes, parserExpr(ctx));
        } while (try_match(ctx, ","));

        node = astCreateListLit(nodes);

        match(ctx, "]");

    } else if (see_kind(ctx, tokenStrLit)) {
        node = astCreateStrLit(ctx->current.buffer);
        accept(ctx);

    } else if (see_kind(ctx, tokenNormal)) {
        sym* symbol;

        if (isPathToken(ctx->current.buffer))
            node = parserPath(ctx);

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

/**
 * Path = <PathLit> | <GlobLit>
 *
 * A glob literal is a path with a wildcard somewhere in it.
 */
static ast* parserPath (parserCtx* ctx) {
    bool modifier = false,
         glob = false;

    const char* str = ctx->current.buffer;

    /*Path modifier*/
    if (*str == '/') {
        modifier = true;
        str++;
    }

    /*Search the path segments looking for glob operators
      (yes, could just strchr directly but in the future this fn
       will use the segments)*/

    char* segments = pathGetSegments(str, malloc);

    for (char* segment = segments; *segment; segment += strlen(segment)+1) {
        if (strchr(segment, '*'))
            glob = true;
    }

    free(segments);

    return (glob ? astCreateGlobLit : astCreateFileLit)(ctx->current.buffer);
}
