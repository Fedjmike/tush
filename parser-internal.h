#include <stdio.h>

#include "forward.h"
#include "token.h"
#include "lexer.h"

enum {
    parserNoisy = false
};

typedef struct parserCtx {
    lexerCtx* lexer;
    token current;

    int errors;
} parserCtx;

static parserCtx* parserCreate (lexerCtx* lexer);
static void parserDestroy (parserCtx* ctx);

static void error (parserCtx* ctx, const char* msg);

static bool see (parserCtx* ctx, const char* look);
static bool see_kind (parserCtx* ctx, tokenKind look);
static bool waiting_for (parserCtx* ctx, const char* look);

static void accept (parserCtx* ctx);
static void expected (parserCtx* ctx, const char* expected);

static void match (parserCtx* ctx, const char* look);
static bool try_match (parserCtx* ctx, const char* look);

/*==== Inline implementations ====*/

inline static parserCtx* parserCreate (lexerCtx* lexer) {
    parserCtx* ctx = calloc(1, sizeof(*ctx));
    ctx->lexer = lexer;
    ctx->current = lexerNext(ctx->lexer);
    ctx->errors = 0;
    return ctx;
}

inline static void parserDestroy (parserCtx* ctx) {
    free(ctx);
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
