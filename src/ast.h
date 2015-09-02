#pragma once

#include <vector.h>

#include "forward.h"

typedef enum astKind {
    astBOP, astFnApp, astSymbol,
    astUnitLit, astIntLit, astFloatLit, astBoolLit, astStrLit,
    astFileLit, astGlobLit, astListLit, astTupleLit, astFnLit,
    astLet,
    astInvalid,
    astKindNo
} astKind;

typedef enum astFlags {
    flagNone = 0,
    /*FnApp*/
    flagUnixInvocation = 1 << 0,
    /*BOP[o=Pipe]*/
    flagListApplication = 1 << 1,
    /*FileLit*/
    flagAbsolutePath = 1 << 2,
    flagAllowPathSearch = 1 << 3
} astFlags;

typedef enum opKind {
    opNull = 0,
    opPipe, opPipeZip, opWrite,
    opLogicalAnd, opLogicalOr,
    opEqual, opNotEqual, opLess, opLessEqual, opGreater, opGreaterEqual,
    opAdd, opSubtract, opConcat,
    opMultiply, opDivide, opModulo
} opKind;

/*Owns all members and children except the symbol*/
typedef struct ast {
    astKind kind;
    astFlags flags;

    vector(ast*) children;
    ast *l, *r;
    opKind op;
    type* dt;

    union {
        union {
            /*IntLit*/
            int64_t integer;
            /*FloatLit*/
            double number;
            /*BoolLit*/
            bool truth;
            /*StrLit FileLit GlobLit*/
            char* str;
        } literal;

        /*FnLit*/
        vector(sym*)* captured;
        /*Symbol Let*/
        sym* symbol;
    };
} ast;

ast* astCreateBOP (ast* l, ast* r, opKind op);
ast* astCreateFnApp (vector(ast*) args, ast* fn);
ast* astCreateSymbol (sym* symbol);

ast* astCreateUnitLit (void);
ast* astCreateIntLit (int64_t integer);
ast* astCreateFloatLit (double number);
ast* astCreateBoolLit (bool truth);
ast* astCreateStrLit (const char* str);
ast* astCreateFileLit (const char* str, astFlags flags);
ast* astCreateGlobLit (const char* str, astFlags flags);
ast* astCreateListLit (vector(ast*) elements);
ast* astCreateTupleLit (vector(ast*) elements);
ast* astCreateFnLit (vector(ast*) args, ast* expr, vector(sym*) captured);

ast* astCreateLet (sym* symbol, ast* init);

ast* astCreateInvalid (void);

void astDestroy (ast* node);

/*Duplicate an entire AST tree, including all owned objects*/
ast* astDup (const ast* tree, stdalloc allocator);

const char* opKindGetStr (opKind kind);
const char* astKindGetStr (astKind kind);
