#pragma once

#include <vector.h>

#include "forward.h"
//#include "type.h"

typedef enum astKind {
    astBOP, astFnApp, astStrLit, astSymbolLit,
    astInvalid,
    astMAX
} astKind;

typedef enum opKind {
    opNull = 0, opWrite, opAppend
} opKind;

//todo different ast malloc depending on # of children

typedef struct ast {
    astKind kind;

    vector(ast*) children;
    ast *l, *r;
    opKind op;

    union {
        sym* symbol;
        char* str;
    } literal;
} ast;

ast* astCreateBOP (ast* l, ast* r, opKind op);
ast* astCreateFnApp (ast* fn, vector(ast*) args);
ast* astCreateStrLit (const char* str);
ast* astCreateSymbolLit (sym* symbol);

ast* astCreateInvalid (void);

void astDestroy (ast* node);
