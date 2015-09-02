#pragma once

#include <unistd.h>
#include <vector.h>

#include "common.h"
#include "paths.h"

typedef struct dirCtx {
    vector(char*) searchPaths;
    /*This is for UI purposes and shouldn't be used to construct
      actual paths.*/
    char* workingDir;
} dirCtx;

/*Takes ownership of the parameters. workingDir is allowed to be null.*/
static dirCtx dirsInit (vector(char*) searchPaths, char* workingDir);

static dirCtx* dirsFree (dirCtx* dirs);

static bool dirsChangeWD (dirCtx* dirs, const char* newWD);

static char* dirsSearch (dirCtx* dirs, const char* str, malloc_t malloc);

/*==== Inline implementations ====*/

inline static dirCtx dirsInit (vector(char*) searchPaths, char* workingDir) {
    return (dirCtx) {
        .searchPaths = searchPaths,
        .workingDir = workingDir
    };
}

inline static dirCtx* dirsFree (dirCtx* dirs) {
    vectorFreeObjs(&dirs->searchPaths, free);
    free(dirs->workingDir);
    return dirs;
};

inline static bool dirsChangeWD (dirCtx* dirs, const char* newWD) {
    char* absolute = pathGetAbsolute(newWD, malloc);

    /*The actual change of directory must be made after getting the
      absolute path as it would affect doing so.
      However, the use of the new path depends on whether the chdir
      succeeded. Therefore this function is the proper place for the
      chdir to happen.*/
    bool error = chdir(newWD);

    if (!error) {
        if (!absolute) {
            //todo errno
            errprintf("Unable to turn the working directory, \"%s\", into an absolute path\n", newWD);

            absolute = malloc(strlen(newWD) + 5);
            sprintf(absolute, "??""?/%s", newWD);
        }

        free(dirs->workingDir);
        dirs->workingDir = absolute;
    }

    return error;
}

inline static char* dirsSearch (dirCtx* dirs, const char* str, malloc_t malloc) {
    enum {bufsize = 512};
    char* fullpath = malloc(bufsize);

    for_vector (const char* dir, dirs->searchPaths, {
        int printed = snprintf(fullpath, bufsize, "%s/%s", dir, str);

        if (printed == bufsize) {
            //todo realloc
            errprintf("Search path '%s' too long\n", dir);
            continue;
        }

        bool fail = access(fullpath, F_OK);

        if (!fail)
            return fullpath;
    })

    return 0;
}
