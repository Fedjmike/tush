#pragma once

#include <vector.h>

#include "forward.h"
//#include "type.h"

typedef enum astKind {
    astBOP, astFnApp, astStrLit, astSymbolLit, astListLit,
    astInvalid,
    astKindNo
} astKind;

typedef enum opKind {
    opNull = 0, opWrite, opAppend
} opKind;

//todo different ast malloc depending on # of children

/*Owns all members and children except the symbol*/
typedef struct ast {
    astKind kind;

    vector(ast*) children;
    ast *l, *r;
    opKind op;
    type* dt;

    union {
        union {
            /*StrLit*/
            char* str;
            /*SymbolLit*/
            sym* symbol;
        } literal;

        /*FnApp*/
        bool listApp;
    };
    //todo combine bools into a flags field
} ast;

ast* astCreateBOP (ast* l, ast* r, opKind op);
ast* astCreateFnApp (ast* fn, vector(ast*) args);
ast* astCreateStrLit (const char* str);
ast* astCreateSymbolLit (sym* symbol);
ast* astCreateListLit (vector(ast*) elements);

ast* astCreateInvalid (void);

void astDestroy (ast* node);

const char* astKindGetStr (astKind kind);
