#include "ast.h"

#include <stdlib.h>
#include <vector.h>

#include "common.h"

static ast* astCreate (astKind kind, ast init) {
    precond(kind != astKindNo);

    ast* node = malloci(sizeof(*node), &init);
    node->kind = kind;
    return node;
}

void astDestroy (ast* node) {
    if (node->l)
        astDestroy(node->l);

    if (node->r)
        astDestroy(node->r);

    vectorFreeObjs(&node->children, (vectorDtor) astDestroy);

    if (   node->kind == astStrLit || node->kind == astFileLit
        || node->kind == astGlobLit)
        free(node->literal.str);

    if (node->kind == astFnLit && node->captured) {
        vectorFree(node->captured);
        /*Stored indirectly*/
        free(node->captured);
    }

    free(node);
}

ast* astCreateListLit (vector(ast*) elements) {
    return astCreate(astListLit, (ast) {
        .children = elements,
    });
}

ast* astCreateTupleLit (vector(ast*) elements) {
    return astCreate(astTupleLit, (ast) {
        .children = elements,
    });
}

ast* astCreateFnLit (vector(ast*) args, ast* expr, vector(sym*) captured) {
    return astCreate(astFnLit, (ast) {
        .children = args, .r = expr,
        .captured = malloci(sizeof(vector), &captured)
    });
}

ast* astCreateUnitLit (void) {
    return astCreate(astUnitLit, (ast) {});
}

ast* astCreateIntLit (int64_t integer) {
    return astCreate(astIntLit, (ast) {
        .literal.integer = integer,
    });
}

ast* astCreateFloatLit (double number) {
    return astCreate(astFloatLit, (ast) {
        .literal.number = number,
    });
}

ast* astCreateBoolLit (bool truth) {
    return astCreate(astBoolLit, (ast) {
        .literal.truth = truth,
    });
}

ast* astCreateStrLit (const char* str) {
    return astCreate(astStrLit, (ast) {
        .literal.str = strdup(str),
    });
}

ast* astCreateFileLit (const char* str, astFlags flags) {
    return astCreate(astFileLit, (ast) {
        .flags = flags, .literal.str = strdup(str),
    });
}

ast* astCreateGlobLit (const char* str, astFlags flags) {
    return astCreate(astGlobLit, (ast) {
        .flags = flags, .literal.str = strdup(str),
    });
}

ast* astCreateSymbol (sym* symbol) {
    return astCreate(astSymbol, (ast) {
        .symbol = symbol
    });
}

ast* astCreateFnApp (vector(ast*) args, ast* fn) {
    return astCreate(astFnApp, (ast) {
        .r = fn,
        .children = args
    });
}

ast* astCreateBOP (ast* l, ast* r, opKind op) {
    return astCreate(astBOP, (ast) {
        .l = l,
        .r = r,
        .op = op
    });
}

ast* astCreateTypeHint (ast* symbol, type* dt) {
    precond(symbol->kind == astSymbol);
    precond(symbol->symbol);

    return astCreate(astTypeHint, (ast) {
        .l = symbol, .dt = dt, .symbol = symbol->symbol
    });
}

ast* astCreateLet (sym* symbol, ast* init) {
    return astCreate(astLet, (ast) {
        .symbol = symbol, .r = init
    });
}

ast* astCreateInvalid (void) {
    return astCreate(astInvalid, (ast) {});
}

ast* astDup (const ast* original, malloc_t malloc) {
    ast* node = malloc(sizeof(ast));
    *node = *original;

    if (original->l)
        node->l = astDup(original->l, malloc);

    if (original->r)
        node->r = astDup(original->r, malloc);

    if (!vectorNull(original->children)) {
        node->children = vectorInit(original->children.length, malloc);

        for_vector (ast* child, original->children, {
            vectorPush(&node->children, astDup(child, malloc));
        })
    }

    switch (node->kind) {
    case astStrLit:
    case astFileLit:
    case astGlobLit:
        if (original->literal.str) {
            size_t length = strlen(original->literal.str)+1;
            node->literal.str = malloc(length);
            memcpy(node->literal.str, original->literal.str, length);
        }

        break;

    case astFnLit:
        if (original->captured) {
            node->captured = malloc(sizeof(vector));
            *node->captured = vectorDup(*original->captured, malloc);
        }

        break;

    default:
        ;
    }

    return node;
}

/*==== ====*/

const char* opKindGetStr (opKind kind) {
    switch (kind) {
    case opPipe: return "|";
    case opPipeZip: return "|:";
    case opWrite: return "|>";
    case opLogicalAnd: return "&&";
    case opLogicalOr: return "||";
    case opEqual: return "==";
    case opNotEqual: return "!=";
    case opLess: return "<";
    case opLessEqual: return "<=";
    case opGreater: return ">";
    case opGreaterEqual: return ">=";
    case opAdd: return "+";
    case opSubtract: return "-";
    case opConcat: return "++";
    case opMultiply: return "*";
    case opDivide: return "/";
    case opModulo: return "%";
    case opNull: return "<null op kind>";
    }

    return "<unhandled op kind>";
}

const char* astKindGetStr (astKind kind) {
    switch (kind) {
    case astUnitLit: return "UnitLit";
    case astIntLit: return "IntLit";
    case astFloatLit: return "FloatLit";
    case astBoolLit: return "BoolLit";
    case astStrLit: return "StrLit";
    case astFileLit: return "FileLit";
    case astGlobLit: return "GlobLit";
    case astListLit: return "ListLit";
    case astTupleLit: return "TupleLit";
    case astFnLit: return "FnLit";
    case astBOP: return "BOP";
    case astFnApp: return "FnApp";
    case astSymbol: return "Symbol";
    case astLet: return "Let";
    case astTypeHint: return "TypeHint";
    case astInvalid: return "Invalid";
    case astKindNo: return "<KindNo; not real>";
    }

    return "<unhandled AST kind>";
}
