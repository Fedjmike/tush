#include <glob.h>

#include <gc.h>
#include <vector.h>
#include <nicestat.h>

#include "type.h"
#include "value.h"
#include "sym.h"

value* builtinExpandGlob (const char* pattern, value* arg) {
    /*arg is ()*/
    (void) arg;

    value* result;

    glob_t matches = {};
    int error = glob(pattern, 0, 0, &matches);

    if (error)
        result = valueStoreTuple(0);

    else {
        vector(value*) results = vectorInit(matches.gl_pathc, GC_malloc);

        /*Box the strings in value objects*/
        for (unsigned int n = 0; n < matches.gl_pathc; n++)
            vectorPush(&results, valueCreateFile(matches.gl_pathv[n]));

        result = valueStoreVector(results);
    }

    globfree(&matches);

    return result;
}

static value* builtinSize (const value* file) {
    const char* filename = valueGetFilename(file);

    if (!filename)
        return valueCreateInvalid();

    stat_t st;
    bool fail = nicestat(filename, &st);

    if (fail)
        return valueCreateInvalid();

    return valueCreateInt(st.size);
}

static value* builtinSum (const value* numbers) {
    int64_t total = 0;

    //todo adapt for Number, when it exists

    valueIter iter;
    bool fail = valueGetIterator(numbers, &iter);

    if (fail)
        return valueCreateInvalid();

    for (const value* number; (number = valueIterRead(&iter));)
        total += valueGetInt(number);

    return valueCreateInt(total);
}

static value* builtinZipf (const value* fn, const value* arg) {
    return valueStoreTuple(2, valueCall(fn, arg), arg);
}

static value* builtinZipfCurried (const value* fn) {
	/*Store the first parameter until we can do any computation*/
    return valueCreateSimpleClosure(fn, (simpleClosureFn) builtinZipf);
}

int compareTuple (const value** left, const value** right) {
    const value *firstOfLeft = valueGetTupleNth(*left, 0),
                *firstOfRight = valueGetTupleNth(*right, 0);

    int64_t indexOfLeft = valueGetInt(firstOfLeft),
            indexOfRight = valueGetInt(firstOfRight);

    return indexOfLeft - indexOfRight;
}

static value* builtinSort (const value* list) {
    vector(const value*) vec = vectorDup(valueGetVector(list), malloc);

    qsort(vec.buffer, vec.length, sizeof(void*),
          (int (*)(const void *, const void *)) compareTuple);

    return valueStoreVector(vec);
}

static void addBuiltin (sym* global, const char* name, type* dt, value* val) {
    sym* symbol = symAdd(global, name);
    symbol->dt = dt;
    symbol->val = val;
}

void addBuiltins (typeSys* ts, sym* global) {
    type *File = typeUnitary(ts, type_File),
         *Int = typeUnitary(ts, type_Int);

    addBuiltin(global, "size",
               typeFn(ts, File, Int),
               valueCreateFn(builtinSize));

    addBuiltin(global, "sum",
               typeFn(ts, typeList(ts, Int), Int),
               valueCreateFn(builtinSum));

    {
        type *A = typeVar(ts),
             *B = typeVar(ts);
        type* B_A = typeTuple(ts, vectorInitChain(2, malloc, B, A));

        addBuiltin(global, "zipf",
                   /*'a 'b => ('a -> 'b) -> 'a -> ('b, 'a)*/
                   typeForall(ts, vectorInitChain(2, malloc, A, B),
                       typeFn(ts, typeFn(ts, A, B),
                       typeFn(ts, A, B_A))),
                   valueCreateFn(builtinZipfCurried));
    }

    {
        type* A = typeVar(ts);
        type* Int_A = typeTuple(ts, vectorInitChain(2, malloc, Int, A));

        addBuiltin(global, "sort",
                   /*[(Int, 'a)] -> [(Int, 'a)]*/
                   typeForall(ts, vectorInitChain(1, malloc, A),
                       typeFn(ts, typeList(ts, Int_A),
                                  typeList(ts, Int_A))),
                   valueCreateFn(builtinSort));
    }
}
