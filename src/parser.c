#include "parser.h"
#include "parser-internal.h"

#include <string.h>

#include "paths.h"

#include "sym.h"
#include "ast.h"

static ast* parseExpr (parserCtx* ctx);

/**
 * Pattern = <Name>
 */
static ast* parserPattern (parserCtx* ctx) {
    ast* node;

    //todo accept only [\w\d-]

    if (see_kind(ctx, tokenNormal)) {
        sym* symbol = symAdd(ctx->scope, ctx->current.buffer);
        accept(ctx);

        node = astCreateSymbol(symbol);

    } else {
        expected(ctx, "function argument");
        node = astCreateInvalid();
    }

    return node;
}

/**
 * FnLit = "\" [{ Pattern }] "->" Expr
 */
static ast* parseFnLit (parserCtx* ctx) {
    match(ctx, "\\");

    vector(sym*) captured = vectorInit(8, malloc);

    /*Enter a new lexical scope and function context*/
    enter_fn(ctx, symAddScope(ctx->scope), &captured);

    /*Arg patterns*/

    vector(ast*) args = vectorInit(2, malloc);

    while (see_kind(ctx, tokenNormal))
        vectorPush(&args, parserPattern(ctx));

    /*Body*/

    match(ctx, "->");
    ast* expr = parseExpr(ctx);

    /*Restore the previous scope*/
    exit_fn(ctx);

    return astCreateFnLit(args, expr, captured);
}

/**
 * Symbol = <QualifiedName>
 */
static ast* parseSymbol (parserCtx* ctx, sym* symbol) {
    bool capturing = false;

    /*Find the first (outermost) fn that needs to capture this value,
      then add it to the capture list for that and all further fns.*/
    for_vector (parserFnCtx* fn, ctx->fns, {
        if (capturing || !symIsInside(symbol, fn->scope)) {
            capturing = true;
            vectorPush(fn->captured, symbol);
        }
    })

    return astCreateSymbol(symbol);
}

/**
 * Path = <PathLit> | <GlobLit>
 *
 * A glob literal is a path with a wildcard somewhere in it.
 */
static ast* parsePath (parserCtx* ctx) {
    bool modifier = false,
         glob = false;

    astFlags flags = flagNone;

    const char* str = ctx->current.buffer;

    /*Inspect the first char*/
    if (str[0] == '/' || str[0] == '-') {
        /*Root*/
        if (str[0] == '-') {
            flags |= flagAbsolutePath;

            if (str[1] == 0)
                str = "-/";

        /*Path modifier*/
        } else
            modifier = true;

        /*Move past this char.
          This "translates" the string into its OS path equivalent*/
        str++;
    }

    /*If the string contains either path segments or an extension
      then it cannot be searched for in PATH*/
    if (   strchr(str, '/') == 0
        && strchr(str, '.') == 0)
        flags |= flagAllowPathSearch;

    /*Search the path segments looking for glob operators
      (yes, could just strchr directly but in the future this fn
       will use the segments)*/

    char* segments = pathGetSegments(str, malloc);

    for (char* segment = segments; *segment; segment += strlen(segment)+1) {
        if (strchr(segment, '*'))
            glob = true;
    }

    free(segments);

    /*To be implemented*/
    (void) modifier;

    return (glob ? astCreateGlobLit : astCreateFileLit)(str, flags);
}

static bool isPathToken (const char* str) {
    return strchr(str, '/') || strchr(str, '.') || strchr(str, '*');
}

/**
 * Atom =   ( "(" [ Expr [{ "," Expr }] ] ")" )
 *        | ( "[" [{ Expr }] "]" )
 *        | FnLit | Path | <Str> | Symbol
 */
static ast* parseAtom (parserCtx* ctx) {
    ast* node;

    if (try_match(ctx, "(")) {
        /*Empty brackets => unit literal*/
        if (see(ctx, ")"))
            node = astCreateUnitLit();

        else {
            node = parseExpr(ctx);

            /*Tuple literal*/
            if (see(ctx, ",")) {
                vector(ast*) nodes = vectorInit(3, malloc);
                vectorPush(&nodes, node);

                while (try_match(ctx, ","))
                    vectorPush(&nodes, parseExpr(ctx));

                node = astCreateTupleLit(nodes);
            }
        }

        match(ctx, ")");

    /*List literal*/
    } else if (try_match(ctx, "[")) {
        vector(ast*) nodes = vectorInit(4, malloc);

        if (waiting_for(ctx, "]")) do {
            vectorPush(&nodes, parseExpr(ctx));
        } while (try_match(ctx, ","));

        node = astCreateListLit(nodes);

        match(ctx, "]");

    } else if (see(ctx, "\\")) {
        node = parseFnLit(ctx);

    } else if (see(ctx, "true") || see(ctx, "false")) {
        node = astCreateBoolLit(see(ctx, "true"));
        accept(ctx);

    } else if (see_kind(ctx, tokenIntLit)) {
        node = astCreateIntLit(atoi(ctx->current.buffer));
        accept(ctx);

    } else if (see_kind(ctx, tokenStrLit)) {
        node = astCreateStrLit(ctx->current.buffer);
        accept(ctx);

    } else if (see_kind(ctx, tokenNormal)) {
        sym* symbol;

        if (isPathToken(ctx->current.buffer))
            node = parsePath(ctx);

        else if ((symbol = symLookup(ctx->scope, ctx->current.buffer)))
            node = parseSymbol(ctx, symbol);

        else
            node = parsePath(ctx);

        accept(ctx);

    } else {
        expected(ctx, "expression");
        node = astCreateInvalid();
    }

    return node;
}

static bool waiting_for_delim (parserCtx* ctx) {
    bool seeLowPrecOp =    see_kind(ctx, tokenOp)
                        && !see(ctx, "(")
                        && !see(ctx, "[")
                        && !see(ctx, "!");

    return waiting(ctx) && !seeLowPrecOp;
}

/**
 * FnApp = { Atom | ( "`" Atom "`" ) }
 *
 * A series of at least one expr-atom. If multiple, then the last is a
 * function to which the others are applied. Instead, one of them may
 * be explicitly marked in backticks as the function.
 */
static ast* parseFnApp (parserCtx* ctx) {
    /*Filled iff there is a backtick function*/
    ast* fn = 0;

    vector(ast*) nodes = vectorInit(3, malloc);

    /*Require at least one expr*/
    if (!see(ctx, "!"))
        vectorPush(&nodes, parseAtom(ctx));

    while (waiting_for_delim(ctx)) {
        if (try_match(ctx, "!")) {
            if (fn) {
                error(ctx)("Multiple explicit functions: '%s'\n", ctx->current.buffer);
                vectorPush(&nodes, fn);
            }

            fn = parseAtom(ctx);

        } else
            vectorPush(&nodes, parseAtom(ctx));
    }

    if (fn)
        return astCreateFnApp(nodes, fn);

    else if (nodes.length == 0) {
        /*Shouldn't happen due to the way it parses*/
        errprintf("FnApp took no AST nodes");
        return astCreateInvalid();

    } else if (nodes.length == 1) {
        /*No application*/
        ast* node = vectorPop(&nodes);
        vectorFree(&nodes);
        return node;

    } else {
    	/*The last node is the fn*/
        fn = vectorPop(&nodes);
        return astCreateFnApp(nodes, fn);
    }
}

/**
 * BOP = Pipe
 * Pipe    = Logical  [{ "|" | "|>" Logical }]
 * Logical = Equality [{ "&&" | "||" Equality }]
 * Equality = Sum     [{ "==" | "!=" | "<" | "<=" | ">" | ">=" Sum }]
 * Sum     = Product  [{ "+" | "-" | "++" Product }]
 * Product = Exit     [{ "*" | "/" | "%" Exit }]
 * Exit = FnApp
 *
 * The grammar is ambiguous, but these operators are left associative,
 * which is to say:
 *    x op y op z == (x op y) op z
 */
static ast* parseBOP (parserCtx* ctx, int level) {
    /* (1) Operator precedence parsing!
      As all the productions above are essentially the same, with
      different operators, we can parse them with the same function.

      Read the comments in numbered order, this was (1)*/

    /* (5) Finally, once we reach the top level we escape the recursion
           into... */
    if (level == 5)
        return parseFnApp(ctx);

    /* (2) The left hand side is the production one level up*/
    ast* node = parseBOP(ctx, level+1);
    opKind op;

    /* (3) Accept operators associated with this level, and store
           which kind is found*/
    while (  level == 0
             ? (op =   try_match(ctx, "|") ? opPipe
                     : try_match(ctx, "|:") ? opPipeZip
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
                     : try_match(ctx, "-") ? opSubtract
                     : try_match(ctx, "++") ? opConcat : opNull)
           : level == 4
             ? (op =   try_match(ctx, "*") ? opMultiply
                     : try_match(ctx, "/") ? opDivide
                     : try_match(ctx, "%") ? opModulo : opNull)
           : (op = opNull)) {
        /* (4) Bundle it up with an RHS, also the level up*/
        ast* rhs = parseBOP(ctx, level+1);
        node = astCreateBOP(node, rhs, op);
    }

    return node;
}

static ast* parseExpr (parserCtx* ctx) {
    return parseBOP(ctx, 0);
}

/**
 * Let = "let" <Name> "=" Expr
 */
static ast* parseLet (parserCtx* ctx) {
    match(ctx, "let");

    sym* symbol = 0;

    if (see_kind(ctx, tokenNormal)) {
    	/*Shadow any previous definitions*/
        symbol = symAdd(ctx->scope, ctx->current.buffer);
        accept(ctx);

    } else
        expected(ctx, "variable name");

    match(ctx, "=");

    ast* init = parseExpr(ctx);

    return astCreateLet(symbol, init);
}

/**
 * Statement = Let | Expr
 */
static ast* parseStatement (parserCtx* ctx) {
    if (see(ctx, "let"))
        return parseLet(ctx);

    else
        return parseExpr(ctx);
}

static ast* parseS (parserCtx* ctx) {
    ast* node = parseStatement(ctx);

    if (see_kind(ctx, tokenEOF))
        accept(ctx);

    else
        error(ctx)("Expected end of text, found '%s'\n", ctx->current.buffer);

    return node;
}

parserResult parse (sym* global, lexerCtx* lexer) {
    parserCtx ctx = parserInit(global, lexer);
    ast* tree = parseS(&ctx);

    if (!tree) {
        errprintf("No syntax tree created\n");
        tree = astCreateInvalid();
    }

    parserResult result = {
        .tree = tree,
        .errors = ctx.errors
    };

    parserFree(&ctx);

    return result;
}
