#include "paths.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "common.h"

const char* pathGetHome (void) {
    return getenv("HOME");
}

/*Check whether a path begins with another one, the prefix*/
static bool pathIsPrefix (const char* path, const char* prefix, size_t prefix_length) {
    return    !strncmp(path, prefix, prefix_length)
              /*The prefix must end in, or be followed by, a slash*/
           && (path[prefix_length] == '/' || path[prefix_length+1] == '/');
}

char* pathContract (const char* path, const char* prefix, const char* replacement, stdalloc allocator) {
    size_t prefix_length = strlen(prefix);

    /*Able to make the substition?*/
    if (pathIsPrefix(path, prefix, prefix_length)) {
        char* contracted = allocator(strlen(path) - prefix_length + 1);

        bool prefix_includes_slash = path[prefix_length] == '/';

        sprintf(contracted, "%s/%s", replacement,
                path + prefix_length + (prefix_includes_slash ? 1 : 2));

        return contracted;

    } else
        return strcpy(allocator(strlen(path)), path);
}