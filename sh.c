#include <stdio.h>
#include <gc/gc.h>

#include "common.h"
#include "type.h"
#include "sym.h"
#include "ast.h"

#include "lexer.h"
#include "parser.h"
#include "analyzer.h"

#include "ast-printer.h"

#include "value.h"
#include "runner.h"

void addBuiltin (sym* global, const char* name, type* dt) {
    symAdd(global, name)->dt = dt;
}

void addBuiltins (typeSys* ts, sym* global) {
    addBuiltin(global, "size", typeFn(ts, typeFile(ts), typeInteger(ts)));
}

int main (int argc, char** argv) {
    (void) argc, (void) argv;

    GC_INIT();

    typeSys ts = typesInit();

    sym* global = symInit();
    addBuiltins(&ts, global);

    char* str = "sh.c size";
    //char* str = "*.cpp wc | sort";

    lexerCtx lexer = lexerInit(str);
    ast* tree = parse(global, &lexer);
    lexerDestroy(&lexer);

    analyze(&ts, tree);

    envCtx env = {};
    value* result = run(&env, tree);

    valuePrint(result);

    astDestroy(tree);

    symEnd(global);

    typesFree(&ts);
}
