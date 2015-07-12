#pragma once

#include <vector.h>

#include "forward.h"

typedef enum astKind {
    astFnApp, astLitStr, astLitSymbol
} astKind;

typedef struct ast {
    astKind kind;

    vector(ast*) children;
    ast *l, *r;

    union {
        sym* symbol;
        char* str;
    } literal;
} ast;

ast* astCreateFnApp (ast* fn, vector(ast*) args);
ast* astCreateLitStr (const char* str);
ast* astCreateLitSymbol (sym* symbol);

void astDestroy (ast* node);
