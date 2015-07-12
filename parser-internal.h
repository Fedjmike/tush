#include <stdio.h>

#include "forward.h"
#include "token.h"
#include "lexer.h"

enum {
    parserNoisy = false
};

typedef struct parserCtx {
    /*The global symbol table*/
    sym* global;
    /*The current scope*/
    sym* scope;

    lexerCtx* lexer;
    token current;

    int errors;
} parserCtx;

static parserCtx parserInit (sym* global, lexerCtx* lexer);
static parserCtx* parserFree (parserCtx* ctx);

static void error (parserCtx* ctx, const char* msg);

static bool see (parserCtx* ctx, const char* look);
static bool see_kind (parserCtx* ctx, tokenKind look);
static bool waiting_for (parserCtx* ctx, const char* look);

static void accept (parserCtx* ctx);
static void expected (parserCtx* ctx, const char* expected);

static void match (parserCtx* ctx, const char* look);
static bool try_match (parserCtx* ctx, const char* look);

/*==== Inline implementations ====*/

inline static parserCtx parserInit (sym* global, lexerCtx* lexer) {
    return (parserCtx) {
        .global = global,
        .scope = global,
        .lexer = lexer,
        .current = lexerNext(lexer),
        .errors = 0
    };
}

inline static parserCtx* parserFree (parserCtx* ctx) {
    /*Nothing to clean up*/
    return ctx;
}

inline static void error (parserCtx* ctx, const char* msg) {
    printf("error: %s\n", msg);
    ctx->errors++;
}

inline static bool see (parserCtx* ctx, const char* look) {
    return !strcmp(ctx->current.buffer, look);
}

inline static bool see_kind (parserCtx* ctx, tokenKind look) {
    return ctx->current.kind == look;
}

inline static bool waiting_for (parserCtx* ctx, const char* look) {
    return !see(ctx, look) && ctx->current.kind != tokenEOF;
}

inline static void accept (parserCtx* ctx) {
    if (parserNoisy)
        printf("%s\n", ctx->current.buffer);

    ctx->current = lexerNext(ctx->lexer);
}

inline static void expected (parserCtx* ctx, const char* expected) {
    (void) expected;
    error(ctx, "expected");
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
