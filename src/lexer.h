#pragma once

#include <stdlib.h>
#include <ctype.h>
#include <hashmap.h>

#include "common.h"
#include "token.h"

enum {
    lexerDefaultBufSize = 128,
    lexerNoisy = false
};

typedef struct lexerCtx {
    const char* input;
    int pos;

    /*The current token being lexed*/
    char* buffer;
    int bufsize;
    int length;
} lexerCtx;

static lexerCtx lexerInit (const char* str);
static lexerCtx* lexerDestroy (lexerCtx* ctx);

static bool lexerEOF (lexerCtx* ctx);
static token lexerNext (lexerCtx* ctx);

static int lexerPos (lexerCtx* ctx);

/*==== Inline implementations ====*/

hashmap(tokenKind) lexerKeywords;

static void lexerKeywordsInit (void);

inline static lexerCtx lexerInit (const char* str) {
    static bool opsInited = false;

    if (!opsInited) {
        lexerKeywordsInit();
        opsInited = true;
    }

    return (lexerCtx) {
        .input = str,
        .pos = 0,

        .buffer = calloc(lexerDefaultBufSize, 1),
        .bufsize = lexerDefaultBufSize,
        .length = 0
    };
}

inline static lexerCtx* lexerDestroy (lexerCtx* ctx) {
    free(ctx->buffer);
    return ctx;
}

inline static int lexerPos (lexerCtx* ctx) {
    return ctx->pos;
}

inline static char lexerCurrent (lexerCtx* ctx) {
    return ctx->input[ctx->pos];
}

inline static bool lexerEOF (lexerCtx* ctx) {
    return lexerCurrent(ctx) == 0;
}

inline static void lexerSkip (lexerCtx* ctx) {
    if (!lexerEOF(ctx))
        ctx->pos++;
}

inline static void lexerEat (lexerCtx* ctx) {
    /*Double the token buffer if its full*/
    if (ctx->length == ctx->bufsize)
        ctx->buffer = realloc(ctx->buffer, ctx->bufsize *= 2);

    ctx->buffer[ctx->length++] = lexerCurrent(ctx);
    lexerSkip(ctx);
}

/*---- ----*/

inline static tokenKind lexerCharOrStr (lexerCtx* ctx) {
    char quote = lexerCurrent(ctx);
    lexerSkip(ctx);

    while (lexerCurrent(ctx) != quote) {
        if (lexerCurrent(ctx) == '\\')
            lexerEat(ctx);

        lexerEat(ctx);
    }

    //todo eat but create other buffer without quotes
    lexerSkip(ctx);

    return quote == '"' ? tokenStrLit : tokenCharLit;
}

inline static tokenKind lexerWord (lexerCtx* ctx) {
    /*Eat while digit*/
    while (isdigit(lexerCurrent(ctx)) && !lexerEOF(ctx))
        lexerEat(ctx);

    /*If that's the end of the token, then it's an integer literal*/

    switch (lexerCurrent(ctx)) {
    case '[': case ']':
    case '{': case '}':
    case '(': case ')':
    case '"': case '\'':
    case '`': case ',':
    case '\n': case '\r':
    case '\t': case ' ':
        return tokenIntLit;
    }

    if (lexerEOF(ctx))
        return tokenIntLit;

    /*Words can contain matching brackets and braces,
      but an unmatched one indicates the end of the word

      e.g. "{module.[ch]]" is three tokens
           "{" "module.[ch]" "]"

      There is no attempt to check their proper nesting.*/

    enum {
        bracket, brace,
        matchingMAX
    };

    int depths[matchingMAX] = {};

    bool exit = false;

    do {
        lexerEat(ctx);

        switch (lexerCurrent(ctx)) {
        case '[': depths[bracket]++; break;
        case '{': depths[brace]++; break;

        /*Don't exit if closing a bracket open within the word*/
        case ']':
            if (depths[bracket]-- == 0)
                exit = true;

        break;
        case '}':
            if (depths[brace]-- == 0)
                exit = true;

        break;
        case ',':
            if (depths[bracket] == 0 && depths[brace] == 0)
                exit = true;

        break;
        case '(': case ')':
        case '"': case '\'':
        case '`':
        case '\n': case '\r':
        case '\t': case ' ':
            exit = true;
        }
    } while (!exit && !lexerEOF(ctx));

    return tokenNormal;
}

inline static token lexerNext (lexerCtx* ctx) {
    /*Skip whitespace*/
    while (isspace(lexerCurrent(ctx)))
        lexerSkip(ctx);

    if (lexerEOF(ctx))
        return tokenMakeEOF();

    ctx->length = 0;

    token tok = {
        .kind = tokenNormal,
        .buffer = ctx->buffer
    };

    switch (lexerCurrent(ctx)) {
    /*String or character literal*/
    case '"': case '\'':
        tok.kind = lexerCharOrStr(ctx);

    break;

    /*"Word"*/
    default:
        tok.kind = lexerWord(ctx);

    break;

    /*Operator*/
    case '(': case ')':
    case '[': case ']':
    case '{': case '}':
    case ',': case '`':
    case '!': case '\\':
        tok.kind = tokenOp;
        lexerEat(ctx);
    }

    ctx->buffer[ctx->length++] = 0;

    /*Reassign the kind if the buffer matches an operator or keyword*/
    if (tok.kind == tokenNormal) {
        tokenKind newkind = (tokenKind) hashmapMap(&lexerKeywords, tok.buffer);

        if (newkind)
            tok.kind = newkind;
    }

    if (lexerNoisy)
    	printf("%s %s\n", tok.buffer, tok.kind == tokenOp ? "op" : "other");

    return tok;
};

static inline void lexerKeywordsInit (void) {
    lexerKeywords = hashmapInit(1024, calloc);

    static const char* ops[] = {
        "|", "|:", "|>", "&&", "||",
        "==", "!=", "<", "<=", ">", ">=",
        /* * would override globs (todo)*/
        /* - would override the root (todo)*/
        "+", "++",
        "/", "%",
        "->", "::"
    };

    static const char* kws[] = {
        "if", "while", "for", "switch", "case",
        "break", "continue", "return", "let", "true", "false"
    };

    /*Add the operators*/

    int opNo = sizeof(ops) / sizeof(*ops);

    for (int i = 0; i < opNo; i++)
        hashmapAdd(&lexerKeywords, ops[i], (void*) tokenOp);

    /*And keywords*/

    int kwNo = sizeof(kws) / sizeof(*kws);

    for (int i = 0; i < kwNo; i++)
        hashmapAdd(&lexerKeywords, kws[i], (void*) tokenKeyword);
}
