#pragma once

#include <vector.h>

typedef struct dirCtx {
    vector(char*) searchPaths;
    char* workingDir;
} dirCtx;

/*Takes ownership of the parameters. workingDir is allowed to be null.*/
static dirCtx dirsInit (vector(char*) searchPaths, char* workingDir);

static dirCtx* dirsFree (dirCtx* dirs);

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
