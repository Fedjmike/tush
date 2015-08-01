#include "paths.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "common.h"

const char* pathGetHome (void) {
    return getenv("HOME");
}

/*==== Path contractions ====*/

/*Check whether a path begins with another one, the prefix*/
static bool pathIsPrefix (const char* path, const char* prefix, size_t prefix_length) {
    return    !strncmp(path, prefix, prefix_length)
              /*The prefix must end in, or be followed by, a slash*/
           && (path[prefix_length] == '/' || path[prefix_length+1] == '/');
}

void pathContract (pathcontracted* contr, const char* path, const char* prefix, const char* replacement) {
    size_t prefix_length = strlen(prefix);

    free(contr->str);

    /*Able to make the substition?*/
    if (pathIsPrefix(path, prefix, prefix_length)) {
        contr->str = malloc(strlen(path) - prefix_length + 1);

        bool prefix_includes_slash = path[prefix_length] == '/';

        sprintf(contr->str, "%s/%s", replacement,
                path + prefix_length + (prefix_includes_slash ? 1 : 2));

    } else
        contr->str = strdup(path);

    if (contr->valid_for)
        contr->valid_for = path;
}

void pathcontrRevalidate (pathcontracted* contr, const char* path, const char* prefix, const char* replacement) {
    /*Invalid?*/
    if (contr->valid_for != path) {
        pathContract(contr, path, prefix, replacement);

        /*Avoid doing this again until the path changes*/
        contr->valid_for = path;
    }
}
