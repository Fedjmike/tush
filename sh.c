#include <stdio.h>
#include <gc/gc.h>

#include "common.h"
#include "type.h"
#include "sym.h"
#include "ast.h"

#include "builtins.h"

#include "lexer.h"
#include "parser.h"
#include "analyzer.h"

#include "ast-printer.h"

#include "value.h"
#include "runner.h"

/*==== Compiler ====*/

typedef struct compilerCtx {
    typeSys ts;
    sym* global;
} compilerCtx;

ast* compile (compilerCtx* ctx, const char* str) {
    /*Turn the string into an AST*/
    lexerCtx lexer = lexerInit(str);
    ast* tree = parse(ctx->global, &lexer);
    lexerDestroy(&lexer);

    /*Add types and other semantic information*/
    analyze(&ctx->ts, tree);
    printAST(tree);

    return tree;
}

compilerCtx compilerInit (void) {
    return (compilerCtx) {
        .ts = typesInit(),
        .global = symInit()
    };
}

compilerCtx* compilerFree (compilerCtx* ctx) {
    symEnd(ctx->global);
    typesFree(&ctx->ts);
    return ctx;
}

/*==== Gosh ====*/

void gosh (compilerCtx* ctx, const char* str) {
    ast* tree = compile(ctx, str);

    /*Run the AST*/
    envCtx env = {};
    value* result = run(&env, tree);

    /*Print the value and type*/
    valuePrint(result);
    printf(" :: %s\n", typeGetStr(tree->dt));

    astDestroy(tree);
}

/*==== ====*/

const char* const samples[] = {
    "sh.c size",
    "[sh.c, parser.c]",
    "[sh.c, parser.c] | size"
};

int main (int argc, char** argv) {
    GC_INIT();

    compilerCtx ctx = compilerInit();
    addBuiltins(&ctx.ts, ctx.global);

    if (argc == 1)
        gosh(&ctx, samples[2]);

    else if (argc == 2)
        gosh(&ctx, argv[1]);

    else
        printf("Unknown arguments.\n");

    compilerFree(&ctx);
}
