const char* pathGetHome (void);

/*==== Path contractions ====*/

typedef struct pathcontracted {
    char* str;
    const char* valid_for;
} pathcontracted;

/*Take a path and replace a directory with another string, if it prefixes the path.
  If it doesn't, the given path is duplicated.

  e.g. prefix="/home/name" prefixes path="/home/name/desktop"
       it can be contracted by replacement="~" to "~/desktop" */
void pathContract (pathcontracted* contr, const char* path, const char* prefix, const char* replacement);

/*Redo the contraction if the path has changed
  (if the path ptr is different, not the contents)*/
void pathcontrRevalidate (pathcontracted* contr, const char* path, const char* prefix, const char* replacement);
