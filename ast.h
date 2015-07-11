#pragma once

#include <vector.h>

#include "forward.h"

typedef enum astKind {
    astFnApp, astLiteral
} astKind;

typedef struct ast {
    astKind kind;

    vector(ast*) children;
    ast *l, *r;

    char* literal;
} ast;

ast* astCreateFnApp (ast* fn, vector(ast*) args);
ast* astCreateLiteral (char* literal);

void astDestroy (ast* node);
