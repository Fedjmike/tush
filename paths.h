#include <common.h>

const char* pathGetHome (void);

/*Take a path and replace a directory with another string, if it prefixes the path.
  If it doesn't, the given path is duplicated.

  e.g. prefix="/home/name" prefixes path="/home/name/desktop"
       it can be contracted by replacement="~" to "~/desktop" */
char* pathContract (const char* path, const char* prefix, const char* replacement, stdalloc allocator);

/*Duplicate a path with the separators ('/') replaced with nulls
  so that the segments may be easily iterated over and treated as
  separate strings. The end of the all the segments is indicated
  by a final segment starting with null.

  e.g. "xxx/yyy/zzz" -> "xxx\0yyy\0zzz\0\0"*/
char* pathGetSegments (const char* path, stdalloc allocator)
