#include <common.h>

const char* pathGetHome (void);

/*Take a path and replace a directory with another string, if it prefixes the path.
  If it doesn't, the given path is duplicated.

  e.g. prefix="/home/name" prefixes path="/home/name/desktop"
       it can be contracted by replacement="~" to "~/desktop" */
char* pathContract (const char* path, const char* prefix, const char* replacement, stdalloc allocator);
