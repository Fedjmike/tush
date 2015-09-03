#include "forward.h"

/*workingDir must be GC allocated. If null, the glob looks for absolute
  paths and [pattern] must start with a slash.*/
value* builtinExpandGlob (const char* pattern, const char* workingDir);

void addBuiltins (typeSys* ts, sym* global);
