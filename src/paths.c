/*For realpath and PATH_MAX*/
#define _XOPEN_SOURCE 700

#include "paths.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <limits.h>
#include <nicestat.h>

#include "common.h"

char* pathGetAbsolute (const char* path, malloc_t malloc) {
    char* absolute = malloc(PATH_MAX+1);
    //todo realpath accesses the fs (symlinks etc) - use something else
    bool success = realpath(path, absolute);

    if (success)
        return absolute;

    else {
        free(absolute);
        return 0;
    }
}

bool pathIsDir (const char* path) {
    stat_t file;
    bool error = nicestat(path, &file);
    return !error && file.mode == file_dir;
}

char* getWorkingDir (alloc_t alloc) {
    int buffer_size = 256;

    do {
        char* buffer = alloc.malloc(buffer_size);

        if (getcwd(buffer, buffer_size) != 0)
            return buffer;

        alloc.free(buffer);
        buffer_size *= 2;

    /*Keep trying with a larger buffer, if that was the problem*/
    } while (errno == ERANGE);

    return 0;
}

const char* getHomeDir (void) {
    return getenv("HOME");
}

vector(char*) initVectorFromPATH (alloc_t alloc) {
    vector(char*) paths = vectorInit(16, alloc.malloc);

    //TODO: secure_getenv ??
    const char* PATH = getenv("PATH");

    /*Iterate through each colon-separated segment of the PATH,
      add the paths to the vector*/
    for (const char *path = PATH,
                    *nextcolon = /*not-null, for the entry condition*/ PATH;
         nextcolon;
         path = nextcolon+1) {
        /*Note: strchr can be used on UTF8 strings as long as searching for a Latin char*/
        nextcolon = strchr(path, ':');

        char* dupe;

        /*Not the last path, followed by a colon*/
        if (nextcolon) {
            size_t pathlength = nextcolon-path;
            /*Copy up to the colon, add a null*/
            dupe = strncpy(alloc.malloc(pathlength+1), path, pathlength);
            dupe[pathlength] = 0;

        } else
            dupe = alloc.strdup(path);

        vectorPush(&paths, dupe);
    }

    return paths;
}

/*Check whether a path begins with another one, the prefix*/
static bool pathIsPrefix (const char* path, const char* prefix, size_t prefix_length) {
    return    !strncmp(path, prefix, prefix_length)
              /*The prefix must end in, or be followed by, a slash*/
           && (path[prefix_length] == '/' || path[prefix_length+1] == '/');
}

char* pathContract (const char* path, const char* prefix, const char* replacement, malloc_t malloc) {
    size_t prefix_length = strlen(prefix);

    /*Able to make the substition?*/
    if (pathIsPrefix(path, prefix, prefix_length)) {
        char* contracted = malloc(strlen(path) - prefix_length + 2);

        bool prefix_includes_slash = path[prefix_length] == '/';

        sprintf(contracted, "%s/%s", replacement,
                path + prefix_length + (prefix_includes_slash ? 1 : 2));

        return contracted;

    } else
        return strcpy(malloc(strlen(path)+1), path);
}

char* pathGetSegments (const char* path, malloc_t malloc) {
    /*Copy the string*/
    size_t length = strlen(path);
    char* segments = malloc(length+2);
    memcpy(segments, path, length+1);
    /*With a second null*/
    segments[length+1] = 0;

    /*Replace slashes with nulls*/
    for (char *current = segments, *nextslash;
         (nextslash = strchr(current, '/'));
         current = nextslash+1) {
        *nextslash = 0;
    }

    return segments;
}
