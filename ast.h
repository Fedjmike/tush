#pragma once

#include <vector.h>

#include "forward.h"

typedef enum astKind {
    astBOP, astFnApp, astSymbol,
    astStrLit, astFileLit, astListLit,
    astInvalid,
    astKindNo
} astKind;

typedef enum opKind {
    opNull = 0,
    opPipe, opWrite,
    opLogicalAnd, opLogicalOr,
    opEqual, opNotEqual, opLess, opLessEqual, opGreater, opGreaterEqual,
    opAdd, opSubtract,
    opMultiply, opDivide, opModulo
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
            /*StrLit FileLit*/
            char* str;
        } literal;

        /*Symbol*/
        sym* symbol;
        /*PipeApp*/
        bool listApp;
    };
    //todo combine bools into a flags field
} ast;

ast* astCreateBOP (ast* l, ast* r, opKind op);
ast* astCreateFnApp (vector(ast*) args, ast* fn);
ast* astCreateSymbol (sym* symbol);

ast* astCreateStrLit (const char* str);
ast* astCreateFileLit (const char* str);
ast* astCreateListLit (vector(ast*) elements);

ast* astCreateInvalid (void);

void astDestroy (ast* node);

const char* opKindGetStr (opKind kind);
const char* astKindGetStr (astKind kind);
