#include <sys/stat.h>

#include "type.h"
#include "value.h"
#include "sym.h"

static value* builtinSize (value* file) {
    const char* filename = valueGetFilename(file);

    if (!filename)
        return valueCreateInvalid();

    struct stat st;
    bool fail = stat(filename, &st);

    if (fail)
        return valueCreateInvalid();

    return valueCreateInt(st.st_size);
}

static void addBuiltin (sym* global, const char* name, type* dt, value* val) {
    sym* symbol = symAdd(global, name);
    symbol->dt = dt;
    symbol->val = val;
}

void addBuiltins (typeSys* ts, sym* global) {
    addBuiltin(global, "size",
               typeFnChain(2, ts, type_File, type_Integer),
               valueCreateFn(builtinSize));
}
