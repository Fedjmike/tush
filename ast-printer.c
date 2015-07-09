#include "ast-printer.h"

#include <stdio.h>

static void printFnApp (const ast* node) {
    puts("fn app");

    vectorFor(&node->children, (vectorIter) printAST);

    printf("fn: ");
    printAST(node->l);
}

static void printLiteral (const ast* node) {
    printf("literal: %s\n", node->literal);
}

void printAST (const ast* node) {
    (void (*[])(const ast* node)) {
        [astFnApp] = printFnApp,
        [astLiteral] = printLiteral
    }[node->kind](node);
}
