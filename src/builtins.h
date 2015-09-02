#include "forward.h"

/*workingDir must be GC allocated*/
value* builtinExpandGlob (const char* pattern);

void addBuiltins (typeSys* ts, sym* global);
