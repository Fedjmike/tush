#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <common.h>

#include "common.h"

static _Noreturn void invoke (const char* program, char** argv) {
    execv(program, argv);
    /*exec* only returns if there was an error*/

    perror("error: failed to execute program");
    printf("       program '%s'\n", program);
    exit(1);
}

int invokeSyncronously (char** argv) {
    const char* program = argv[0];

    pid_t child;

    switch ((child = fork())) {
    case -1:
        errprintf("Failed to start a new process\n");
        return -1;

    /*Child (the program)*/
    case 0:
        invoke(program, argv);

    /*Parent (the shell)*/
    default: {
        int status;

        if (waitpid(child, &status, 0) != child)
            return -2;

        if (WIFEXITED(status))
            return WEXITSTATUS(status);

        else
            return -3;
    }}
}

FILE* invokePiped (char** argv) {
    int programPipe[2];

    if (pipe(programPipe) < 0) {
        errprintf("Failed to create a pipe\n");
        return 0;
    }

    const char* program = argv[0];

    //todo: how does fork deal with resources owned by the parent?

    switch (fork()) {
    case -1:
        errprintf("Failed to start a new process\n");
        return 0;

    /*Child*/
    case 0:
        /*Use the pipe as stdout*/
        fclose(stdout);
        dup2(programPipe[1], STDIN_FILENO);

        invoke(program, argv);

    /*Parent*/
    default:
        close(programPipe[1]);
        return fdopen(programPipe[0], "r");
    }
}
