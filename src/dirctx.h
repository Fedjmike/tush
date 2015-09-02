#pragma once

#include <unistd.h>
#include <gc.h>
#include <vector.h>

#include "common.h"
#include "paths.h"

typedef struct dirCtx {
    vector(char*) searchPaths;
    /*This is for UI purposes and shouldn't be used to construct
      actual paths.*/
    char* workingDirDisplay;
} dirCtx;

static dirCtx dirsInit ();
static dirCtx* dirsFree (dirCtx* dirs);

static bool dirsChangeWD (dirCtx* dirs, const char* newWD);

static const char* dirsSearch (dirCtx* dirs, const char* str);

/*==== Inline implementations ====*/

inline static dirCtx dirsInit () {
    char* workingDir = getWorkingDir(gcalloc);

    return (dirCtx) {
        .searchPaths = initVectorFromPATH(gcalloc),
        .workingDirDisplay = workingDir,
        .workingDirReal = GC_STRDUP(workingDir)
    };
}

inline static dirCtx* dirsFree (dirCtx* dirs) {
    /*Nothing to do because everything was GC allocated*/
    return dirs;
};

inline static bool dirsChangeWD (dirCtx* dirs, const char* newWD) {
    char* absolute = pathGetAbsolute(newWD, GC_malloc);

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

            absolute = GC_malloc(strlen(newWD) + 5);
            sprintf(absolute, "??""?/%s", newWD);
        }

        dirs->workingDirDisplay = absolute;
    }

    return error;
}

inline static const char* dirsSearch (dirCtx* dirs, const char* str) {
    const char* path = 0;

    enum {bufsize = 512};
    char* fullpath = malloc(bufsize);

    for_vector (const char* dir, dirs->searchPaths, {
        int printed = snprintf(fullpath, bufsize, "%s/%s", dir, str);

        if (printed == bufsize) {
            //todo realloc
            errprintf("Search path '%s' too long\n", dir);
            continue;
        }

        bool notfound = access(fullpath, F_OK);

        if (!notfound)
            path = dir;
    })

    free(fullpath);

    return path;
}
