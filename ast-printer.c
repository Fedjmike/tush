#include "ast-printer.h"

#include <stdio.h>

static void printFnApp (const ast* node) {
    puts("fn app");

    vectorFor(&node->children, (vectorIter) printAST);

    printf("fn: ");
    printAST(node->l);
}

static void printStrLit (const ast* node) {
    printf("string literal: %s\n", node->literal.str);
}

void printAST (const ast* node) {
    (void (*[])(const ast* node)) {
        [astFnApp] = printFnApp,
        [astStrLit] = printStrLit
    }[node->kind](node);
}
