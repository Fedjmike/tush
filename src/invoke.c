#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <common.h>

#include "common.h"

bool invokeSyncronously (char** argv) {
    const char* program = argv[0];

    int childID;

    switch ((childID = fork())) {
    case -1:
        errprintf("Failed to start a new process\n");
        return true;

    /*Child (the program)*/
    case 0:
        execvp(program, argv);

        /*exec* only returns if there was an error*/
        errprintf("Failed to execute '%s'\n", program);
        exit(1);

    /*Parent (the shell)*/
    default: {
        int status;
        wait(&status);
        //todo return status?
    }}

    return false;
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
        dup(programPipe[1]);

        execvp(program, argv);

        errprintf("Failed to execute '%s'\n", program);
        exit(1);

    /*Parent*/
    default:
        close(programPipe[1]);
        return fdopen(programPipe[0], "r");
    }
}
