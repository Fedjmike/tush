#include <stdio.h>

#include <vector.h>

#include "forward.h"
#include "token.h"
#include "sym.h"
#include "lexer.h"

enum {
    parserNoisy = false
};

typedef struct parserFnCtx {
    sym* scope;
    /*Symbols captured by this function, to be put in a closure environment.
      Free variables, e.g. y in \x -> x + y*/
    vector(sym*)* captured;
} parserFnCtx;

typedef struct parserCtx {
    /*The global symbol table*/
    sym* global;
    /*The current scope*/
    sym* scope;

    /*A stack of functions, representing the lexical context*/
    vector(parserFnCtx*) fns;

    typeSys* ts;

    lexerCtx* lexer;
    token current;

    int errors;
} parserCtx;

static parserCtx parserInit (sym* global, typeSys* ts, lexerCtx* lexer);
static parserCtx* parserFree (parserCtx* ctx);

static void enter_fn (parserCtx* ctx, sym* newscope, vector(sym*)* captured);
static void exit_fn (parserCtx* ctx);
static parserFnCtx* innermost_fn (parserCtx* ctx);

static printf_t* error (parserCtx* ctx);

static bool see (parserCtx* ctx, const char* look);
static bool see_kind (parserCtx* ctx, tokenKind look);
static bool waiting (parserCtx* ctx);
static bool waiting_for (parserCtx* ctx, const char* look);

static void accept (parserCtx* ctx);
static void expected (parserCtx* ctx, const char* expected);

static void match (parserCtx* ctx, const char* look);
static bool try_match (parserCtx* ctx, const char* look);

/*==== Inline implementations ====*/

inline static parserCtx parserInit (sym* global, typeSys* ts, lexerCtx* lexer) {
    return (parserCtx) {
        .global = global,
        .scope = global,
        .fns = vectorInit(10, malloc),
        .ts = ts,
        .lexer = lexer,
        .current = lexerNext(lexer),
        .errors = 0
    };
}

inline static parserCtx* parserFree (parserCtx* ctx) {
    /*There shouldn't be any unfinished fn scopes left when this is called*/
    precond(ctx->fns.length == 0);
    vectorFreeObjs(&ctx->fns, free);
    return ctx;
}

inline static void enter_fn (parserCtx* ctx, sym* newscope, vector(sym*)* captured) {
    vectorPush(&ctx->fns, malloci(sizeof(parserFnCtx), &(parserFnCtx) {
        .scope = newscope,
        .captured = captured
    }));
    ctx->scope = newscope;
}

inline static void exit_fn (parserCtx* ctx) {
    /*Assumption: the previous scope is the current scope's parent
      (note: it is not the previous fn's scope, there may be `let`s)*/
    free(vectorPop(&ctx->fns));
    ctx->scope = ctx->scope->parent;
}

inline static parserFnCtx* innermost_fn (parserCtx* ctx) {
    return vectorTop(ctx->fns);
}

inline static printf_t* error (parserCtx* ctx) {
    ctx->errors++;
    printf("%d: error: ", lexerPos(ctx->lexer));
    return printf;
}

/*---- Simple interfaces to the lexer ----*/

inline static bool see (parserCtx* ctx, const char* look) {
    return !strcmp(ctx->current.buffer, look);
}

inline static bool see_kind (parserCtx* ctx, tokenKind look) {
    return ctx->current.kind == look;
}

inline static bool waiting (parserCtx* ctx) {
    return ctx->current.kind != tokenEOF;
}

inline static bool waiting_for (parserCtx* ctx, const char* look) {
    return waiting(ctx) && !see(ctx, look);
}

inline static void accept (parserCtx* ctx) {
    if (parserNoisy)
        printf("%s\n", ctx->current.buffer);

    ctx->current = lexerNext(ctx->lexer);
}

inline static void expected (parserCtx* ctx, const char* expected) {
    if (ctx->current.kind == tokenEOF)
        error(ctx)("Expected '%s', found the end of text\n", expected);

    else
        error(ctx)("Expected '%s', found '%s'\n", expected, ctx->current.buffer);

    accept(ctx);
}

inline static void match (parserCtx* ctx, const char* look) {
    if (!try_match(ctx, look))
        expected(ctx, look);
}

inline static bool try_match (parserCtx* ctx, const char* look) {
    if (see(ctx, look)) {
        accept(ctx);
        return true;

    } else
        return false;
}
