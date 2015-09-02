#include "forward.h"

/*workingDir must be GC allocated*/
value* builtinExpandGlob (const char* pattern, const char* workingDir);

void addBuiltins (typeSys* ts, sym* global);
