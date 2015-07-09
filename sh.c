#include <stdio.h>

#include "common.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"

#include "ast-printer.h"

int main (int argc, char** argv) {
    (void) argc, (void) argv;

    char* str = "*.cpp wc | sort";

    lexerCtx* lexer = lexerCreate(str);
    ast* tree = parse(lexer);

    printAST(tree);

    astDestroy(tree);
    lexerDestroy(lexer);
}
