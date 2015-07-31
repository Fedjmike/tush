#pragma once

#include <errno.h>
#include <unistd.h>
#include <vector.h>

typedef struct dirCtx {
    vector(char*) searchPaths;
    char* workingDir;
} dirCtx;

/*Get the current working directory. Fails if there are permission issues*/
static char* dirGetWD (void);

/*envPATH is the colon-seperated PATH environment variable
  workingDir:
  - is now owned by the dirCtx,
  - is allowed to be null.*/
static dirCtx dirsInit (const char* envPATH, char* workingDir);

static dirCtx* dirsFree (dirCtx* dirs);

/*==== Inline implementations ====*/

inline static char* dirGetWD (void) {
    int buffer_size = 256;

    do {
        char* buffer = malloc(buffer_size);

        if (getcwd(buffer, buffer_size) != 0)
            return buffer;

        free(buffer);
        buffer_size *= 2;

    /*Keep trying with a larger buffer, if that was the problem*/
    } while (errno == ERANGE);

    return 0;
}

inline static void initVectorFromPATH (vector(char*)* paths, const char* envPATH) {
    /*Iterate through each colon-separated segment of the PATH,
      add the paths to the vector*/
    for (const char *path = envPATH,
                    *nextcolon = /*not-null, for the entry condition*/ envPATH;
         nextcolon;
         path = nextcolon+1) {
        /*Note: strchr can be used on UTF8 strings as long as searching for a Latin char*/
        nextcolon = strchr(path, ':');

        char* dupe;

        /*Not the last path, followed by a colon*/
        if (nextcolon) {
            size_t pathlength = nextcolon-path;
            /*Copy up to the colon, add a null*/
            dupe = strncpy(malloc(pathlength+1), path, pathlength);
            dupe[pathlength] = 0;

        } else
            dupe = strdup(path);

        vectorPush(paths, dupe);
    }
}

inline static dirCtx dirsInit (const char* envPATH, char* workingDir) {
    dirCtx dirs = {
        .searchPaths = vectorInit(16, malloc),
        .workingDir = workingDir
    };

    initVectorFromPATH(&dirs.searchPaths, envPATH);

    return dirs;
}

inline static dirCtx* dirsFree (dirCtx* dirs) {
    vectorFreeObjs(&dirs->searchPaths, free);
    free(dirs->workingDir);
    return dirs;
};
