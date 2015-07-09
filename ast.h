#pragma once

#include <vector.h>

#include "token.h"

typedef enum astKind {
    astFnApp, astLiteral
} astKind;

typedef struct ast ast;

typedef struct ast {
    astKind kind;

    vector(ast*) children;
    ast *l, *r;

    char* literal;
} ast;

ast* astFnAppCreate (ast* fn, vector(ast*) args);
ast* astLiteralCreate (char* literal);

void astDestroy (ast* node);
