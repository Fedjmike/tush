#pragma once

#include <errno.h>
#include <unistd.h>

#include <common.h>
#include <vector.h>

char* pathGetAbsolute (const char* path, malloc_t malloc);

/*Stats the path to see if it's a directory. Returns false for non-files.*/
bool pathIsDir (const char* path);

/*Get the current working directory. Caller owns the result.
  Fails if there are permission issues.*/
char* getWorkingDir (void);

const char* getHomeDir (void);

vector(char*) initVectorFromPATH (void);

/*Take a path and replace a directory with another string, if it prefixes the path.
  If it doesn't, the given path is duplicated.

  e.g. prefix="/home/name" prefixes path="/home/name/desktop"
       it can be contracted by replacement="~" to "~/desktop" */
char* pathContract (const char* path, const char* prefix, const char* replacement, malloc_t malloc);

/*Duplicate a path with the separators ('/') replaced with nulls
  so that the segments may be easily iterated over and treated as
  separate strings. The end of all the segments is indicated by a
  final segment starting with null.

  e.g. "xxx/yyy/zzz" -> "xxx\0yyy\0zzz\0\0"*/
char* pathGetSegments (const char* path, malloc_t malloc);
