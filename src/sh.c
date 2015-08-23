/*For fmemopen*/
#define _XOPEN_SOURCE 700
/*For asprintf*/
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <gc/gc.h>

#include "common.h"
#include "type.h"
#include "sym.h"
#include "ast.h"

#include "terminal.h"
#include "paths.h"
#include "dirctx.h"
#include "builtins.h"

#include "lexer.h"
#include "parser.h"
#include "analyzer.h"

#include "ast-printer.h"

#include "value.h"
#include "runner.h"
#include "display.h"

_Atomic unsigned int internalerrors = 0;

/*==== Compiler ====*/

typedef struct compilerCtx {
    typeSys ts;
    dirCtx dirs;

    sym* global;
} compilerCtx;

ast* compile (compilerCtx* ctx, const char* str, int* errors) {
    /*Turn the string into an AST*/
    ast* tree; {
        lexerCtx lexer = lexerInit(str);
        parserResult result = parse(ctx->global, &lexer);
        lexerDestroy(&lexer);

        tree = result.tree;
        *errors += result.errors;
    }

    /*Add types and other semantic information*/
    {
        analyzerResult result = analyze(&ctx->ts, tree);
        *errors += result.errors;
    }

    if (false)
    	printAST(tree);

    return tree;
}

compilerCtx compilerInit (void) {
    return (compilerCtx) {
        .ts = typesInit(),
        .dirs = dirsInit(initVectorFromPATH(), getWorkingDir()),
        .global = symInit()
    };
}

compilerCtx* compilerFree (compilerCtx* ctx) {
    symEnd(ctx->global);
    dirsFree(&ctx->dirs);
    typesFree(&ctx->ts);
    return ctx;
}

/*==== Gosh ====*/

typedef struct goshResult {
    value* v;
    type* dt;
} goshResult;

goshResult gosh (compilerCtx* ctx, const char* str, bool display) {
    errctx internalerrors = errcount();
    int errors = 0;

    ast* tree = compile(ctx, str, &errors);

    value* result = 0;
    type* dt = tree->dt;

    if (errors == 0 && no_errors_recently(internalerrors)) {
        /*Run the AST*/
        envCtx env = {};
        result = run(&env, tree);

        if (display && no_errors_recently(internalerrors))
            displayResult(result, dt);
    }

    astDestroy(tree);

    return (goshResult) {.v = result, .dt = dt};
}

/*==== REPL ====*/

typedef struct promptCtx {
    char* str;
    size_t size;
    const char* valid_for;
} promptCtx;

void writePrompt (promptCtx* prompt, const char* wdir, const char* homedir) {
    if (prompt->valid_for == wdir)
        return;

    /*Tilde contract the working directory*/
    char* wdir_contr = pathContract(wdir, homedir, "~", malloc);

    FILE* promptf = fmemopen(prompt->str, prompt->size, "w");
    fprintf_style(promptf, "{%s} $ ", styleYellow, wdir_contr);
    fclose(promptf);

    prompt->valid_for = wdir;
    free(wdir_contr);
}

void replCD (compilerCtx* compiler, const char* input) {
    goshResult result = gosh(compiler, input, false);

    if (!result.v)
        return;

    if (!typeIsKind(type_File, result.dt)) {
        printf(":cd requires a File argument, given %s\n", typeGetStr(result.dt));
        return;
    }

    const char* newWD = valueGetFilename(result.v);
    bool error = dirsChangeWD(&compiler->dirs, newWD);

    if (error)
        printf("Unable to enter directory \"%s\"\n", newWD);
}

void replAST (compilerCtx* compiler, const char* input) {
    int errors = 0;
    ast* tree = compile(compiler, input, &errors);

    if (!errors) {
        if (tree)
            printAST(tree);

        else
            errprintf("Null AST tree generated\n");
    }
}

void repl (compilerCtx* compiler) {
    const char* homedir = getHomeDir();

    char* historyFilename;
    asprintf(&historyFilename, "%s/.gosh_history", homedir);

    read_history(historyFilename);

    promptCtx prompt = {.size = 1024};
    prompt.str = malloc(prompt.size);

    while (true) {
        /*Regenerate the prompt (if necessary)*/
        writePrompt(&prompt, compiler->dirs.workingDir, homedir);

        char* input = readline(prompt.str);

        /*Skip empty strings*/
        if (input[0] == 0)
            continue;

        else if (!strcmp(input, ":exit"))
            break;

        add_history(input);
        write_history(historyFilename);

        if (!strncmp(input, ":cd ", 4))
            replCD(compiler, input+4);

        else if (!strncmp(input, ":ast ", 5))
            replAST(compiler, input+5);

        else
            gosh(compiler, input, true);
    }

    free(prompt.str);
    free(historyFilename);
}

/*==== ====*/

int main (int argc, char** argv) {
    GC_INIT();

    rl_basic_word_break_characters = " \t\n\"\\'`@$><=;|&{([,";

    compilerCtx compiler = compilerInit();
    addBuiltins(&compiler.ts, compiler.global);

    if (argc == 1)
        repl(&compiler);

    else if (argc == 2)
        gosh(&compiler, argv[1], true);

    else {
        char* input = strjoinwith(argc, argv, " ", malloc);
        gosh(&compiler, input, true);
        free(input);
    }

    compilerFree(&compiler);
}
