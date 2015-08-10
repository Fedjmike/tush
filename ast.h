#pragma once

#include <vector.h>

#include "forward.h"

typedef enum astKind {
    astBOP, astFnApp, astSymbol,
    astUnitLit, astStrLit, astFileLit, astGlobLit, astListLit, astTupleLit,
    astInvalid,
    astKindNo
} astKind;

typedef enum opKind {
    opNull = 0,
    opPipe, opWrite,
    opLogicalAnd, opLogicalOr,
    opEqual, opNotEqual, opLess, opLessEqual, opGreater, opGreaterEqual,
    opAdd, opSubtract, opConcat,
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
            /*StrLit FileLit GlobLit*/
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

ast* astCreateUnitLit (void);
ast* astCreateStrLit (const char* str);
ast* astCreateFileLit (const char* str);
ast* astCreateGlobLit (const char* str);
ast* astCreateListLit (vector(ast*) elements);
ast* astCreateTupleLit (vector(ast*) elements);

ast* astCreateInvalid (void);

void astDestroy (ast* node);

const char* opKindGetStr (opKind kind);
const char* astKindGetStr (astKind kind);
