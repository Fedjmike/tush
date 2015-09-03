/*For fmemopen*/
#define _XOPEN_SOURCE 700
/*For asprintf*/
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <gc.h>

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

/*Parse and semantically analyze a string. Returns the typed AST.*/
ast* compile (compilerCtx* ctx, const char* str, int* errors) {
    /*Store the error count ourselves if given a null ptr*/
    if (!errors)
        errors = &(int) {0};

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
        .dirs = dirsInit(),
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

void gosh (compilerCtx* ctx, const char* str, bool display) {
    errctx internalerrors = errcount();
    int errors = 0;

    ast* tree = compile(ctx, str, &errors);

    if (errors == 0 && no_errors_recently(internalerrors)) {
        /*Run the AST*/
        envCtx env = {.dirs = &ctx->dirs};
        value* result = run(&env, tree);

        if (display)
            displayResult(result, tree->dt);
    }

    astDestroy(tree);
}

/*==== REPL ====*/

void repl_errorf (const char* format, ...) {
    fputs("error: ", stderr);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format,  args);
    va_end(args);
}

/*   :cd <dir>
  Expects a File returning expression. If given, runs it and attempts
  to change the working directory.*/
void replCD (compilerCtx* compiler, const char* input) {
    int errors = 0;
    ast* tree = compile(compiler, input, &errors);

    if (errors || typeIsInvalid(tree->dt))
        ;

    else if (!typeIsKind(type_File, tree->dt))
        repl_errorf(":cd requires a File argument, given %s\n", typeGetStr(tree->dt));

    /*Types fine, try running it*/
    else {
        value* result = run(&(envCtx) {.dirs = &compiler->dirs}, tree);

        if (!result || valueIsInvalid(result))
            ;

        else {
            const char* newWD = valueGetFilename(result);
            bool error = dirsChangeWD(&compiler->dirs, newWD);

            if (error)
                repl_errorf("unable to enter directory \"%s\"\n", newWD);
        }
    }

    astDestroy(tree);
}

/*   :ast <expr>
  Displays that syntax tree of a program, without running it.*/
void replAST (compilerCtx* compiler, const char* input) {
    ast* tree = compile(compiler, input, 0);

    if (tree)
        printAST(tree);

    astDestroy(tree);
}

/*   :type <expr>
  Displays the type of an expression, without running it*/
void replType (compilerCtx* compiler, const char* input) {
    int errors = 0;
    ast* tree = compile(compiler, input, &errors);

    if (tree && !errors)
        puts(typeGetStr(tree->dt));

    astDestroy(tree);
}

/*   :mem-stats
  Some memory usage statistics.*/
void replMemStats (compilerCtx* compiler, const char* input) {
    (void) compiler;

    //todo unicode charptr++
    for (; *input; input++) {
        if (!isspace(*input)) {
            repl_errorf(":mem-stats takes no arguments, given %s\n", input);
            return;
        }
    }

#if GC_VERSION_MAJOR >= 7
#if GC_VERSION_MINOR >= 3
    struct GC_prof_stats_s stats;
    GC_get_prof_stats(&stats, sizeof(stats));

    printf("allocated: %lu bytes \t (since last collection)\n"
           "    freed: %lu bytes\n",
           stats.bytes_allocd_since_gc, stats.bytes_reclaimed_since_gc);
    printf("allocated: %lu bytes \t (before that)\n"
           "    freed: %lu bytes\n",
           stats.allocd_bytes_before_gc, stats.reclaimed_bytes_before_gc);
    printf("heap size: %lu bytes\n", stats.heapsize_full);
#endif // GC_VERSION_MINOR
#endif // GC_VERSION_MAJOR
}

typedef struct replCommand {
    const char* name;
    size_t length;
    void (*handler)(compilerCtx* compiler, const char* input);
} replCommand;

static replCommand commands[] = {
    {"cd", strlen("cd"), replCD},
    {"ast", strlen("ast"), replAST},
    {"type", strlen("type"), replType},
    {"mem-stats", strlen("mem-stats"), replMemStats}
};

/*Execute a string if it is a built-in command, by searching through
  the `commands` array.
  \param input is the string *after* the colon which indicates the
         beginning of a built-in.*/
void replCmd (compilerCtx* compiler, const char* input) {
    char* firstSpace = strchr(input, ' ');
    size_t cmdLength = firstSpace ? (size_t)(firstSpace - input) : strlen(input);

    if (cmdLength == 0) {
        repl_errorf("no command name given\n");
        return;
    }

    /*Look through all the commands...*/
    for (unsigned int i = 0; i < sizeof(commands) / sizeof(*commands); i++) {
        replCommand cmd = commands[i];

        /*... for one of the same length*/
        if (cmdLength != cmd.length)
            continue;

        /*... where the name matches*/
        bool nameMatches = !strncmp(input, cmd.name, cmd.length);

        if (nameMatches) {
            cmd.handler(compiler, input + cmd.length);
            return;
        }
    }

    repl_errorf("no command named ':");
    /*Using printf precisions, %.*s, would convert the length to int*/
    fwrite(input, sizeof(*input), cmdLength, stderr);
    fprintf(stderr, "'\n");
}

typedef struct promptCtx {
    char* str;
    size_t size;
    const char* valid_for;
} promptCtx;

/*Rewrite a prompt string given the working dir and home dir.
  Rewrites it if and only if the working directory has changed.*/
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

void repl (compilerCtx* compiler) {
    const char* homedir = getHomeDir();

    char* historyFilename;
    bool historyStaticStr = false;

    if (!precond(asprintf(&historyFilename, "%s/.gosh_history", homedir))) {
        historyFilename = "./.gosh_history";
        historyStaticStr = true;
    }

    read_history(historyFilename);

    promptCtx prompt = {.size = 1024};
    prompt.str = malloc(prompt.size);

    while (true) {
        /*Regenerate the prompt (if necessary)*/
        writePrompt(&prompt, compiler->dirs.workingDirDisplay, homedir);

        char* input = readline(prompt.str);

        /*Skip empty strings*/
        if (!input || input[0] == 0)
            continue;

        else if (!strcmp(input, ":exit"))
            break;

        if (input[0] == ':')
            replCmd(compiler, input+1);

        else
            gosh(compiler, input, true);

        //todo move both history and collection to a separate thread
        //(but the still sync it will the prompt)

        add_history(input);
        write_history(historyFilename);

        //todo wait til false (nothing to collect)
        GC_collect_a_little();
    }

    free(prompt.str);

    if (!historyStaticStr)
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
