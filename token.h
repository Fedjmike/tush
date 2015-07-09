#pragma once

#include "common.h"

typedef enum tokenKind {
    tokenNormal, tokenOp, tokenLitStr, tokenLitChar, tokenEOF
} tokenKind;

typedef struct token {
    tokenKind kind;
    /*Owned by the lexer*/
    const char* buffer;
} token;

static token tokenMakeEOF ();

inline token tokenMakeEOF () {
    return (token) {tokenEOF, ""};
}
