#include <stdio.h>

#include "common.h"
#include "sym.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"

#include "ast-printer.h"

#include "value.h"
#include "runner.h"

void addBuiltins (sym* global) {
    symAdd(global, "size");
}

int main (int argc, char** argv) {
    (void) argc, (void) argv;

    sym* global = symInit();
    addBuiltins(global);

    char* str = "sh.c size";
    //char* str = "*.cpp wc | sort";

    lexerCtx lexer = lexerInit(str);
    ast* tree = parse(global, &lexer);
    lexerDestroy(&lexer);

    envCtx env = {};
    value* result = run(&env, tree);

    valuePrint(result);

    astDestroy(tree);

    symEnd(global);
}
