#include <stdio.h>

#include "common.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"

#include "ast-printer.h"

#include "value.h"
#include "runner.h"

int main (int argc, char** argv) {
    (void) argc, (void) argv;

    char* str = "sh.c size";
    //char* str = "*.cpp wc | sort";

    lexerCtx lexer = lexerInit(str);
    ast* tree = parse(&lexer);
    lexerDestroy(&lexer);

    envCtx env = {};
    value* result = run(&env, tree);

    valuePrint(result);

    astDestroy(tree);
}
