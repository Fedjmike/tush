#include <sys/stat.h>
#include <glob.h>

#include <gc.h>

#include "type.h"
#include "value.h"
#include "sym.h"

static value* builtinSize (const value* file) {
    const char* filename = valueGetFilename(file);

    if (!filename)
        return valueCreateInvalid();

    struct stat st;
    bool fail = stat(filename, &st);

    if (fail)
        return valueCreateInvalid();

    return valueCreateInt(st.st_size);
}

value* builtinExpandGlob (const char* pattern, value* arg) {
    (void) arg;

    vector(char*) results = {};

    glob_t matches = {};
    int error = glob(pattern, 0, 0, &matches);

    if (!error) {
        results = vectorInit(matches.gl_pathc, GC_malloc);

        /*Turn the matches into a vector*/
        for (unsigned int n = 0; n < matches.gl_pathc; n++)
            vectorPush(&results, valueCreateStr(matches.gl_pathv[n]));
    }

    globfree(&matches);

    return valueCreateVector(results);
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
