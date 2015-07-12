#include "ast.h"

#include <stdlib.h>

#include "common.h"

static ast* astCreate (astKind kind, ast init) {
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

    if (node->kind == astStrLit)
        free(node->literal.str);

    free(node);
}

ast* astCreateFnApp (ast* fn, vector(ast*) args) {
    return astCreate(astFnApp, (ast) {
        .l = fn,
        .children = args
    });
}

ast* astCreateStrLit (const char* str) {
    return astCreate(astStrLit, (ast) {
        .literal.str = strdup(str),
    });
}

ast* astCreateSymbolLit (sym* symbol) {
    return astCreate(astSymbolLit, (ast) {
        .literal.symbol = symbol,
    });
}
