#include <glob.h>

#include <gc.h>
#include <vector.h>
#include <nicestat.h>

#include "type.h"
#include "value.h"
#include "sym.h"

value* builtinExpandGlob (const char* pattern, const char* workingDir) {
    /*No working dir => the path is absolute*/
    if (!workingDir) {
        /*Must be provided to glob() as such*/
        if (!precond(pattern[0] == '/')) {
            size_t length = strlen(pattern) + 2;
            char* absolutepattern = GC_MALLOC(length);

            absolutepattern[0] = '/';
            strcpy(absolutepattern+1, pattern);

            pattern = absolutepattern;
        }
    }

    value* result;

    glob_t matches = {};
    int error = glob(pattern, 0, 0, &matches);

    if (error)
        result = valueStoreTuple(0);

    else {
        vector(value*) results = vectorInit(matches.gl_pathc, GC_malloc);

        /*Box the strings in value objects*/
        for (unsigned int n = 0; n < matches.gl_pathc; n++)
            vectorPush(&results, valueCreateFile(matches.gl_pathv[n], workingDir));

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

static value* builtinLinecount (const value* file) {
    const char* filename = valueGetFilename(file);

    if (!filename)
        return valueCreateInvalid();

    FILE* f = fopen(filename, "r");

    if (!f)
        return valueCreateInvalid();

    int lines = 1;
    int lastch = 0;

    while (true) {
        int ch = fgetc(f);

        if (ch == '\n')
            lines++;

        else if (ch == EOF)
            break;

        lastch = ch;
    }

    if (lastch == '\n')
        lines--;

    fclose(f);

    return valueCreateInt(lines);
}

static value* builtinSum (const value* numbers) {
    int64_t total = 0;

    valueIter iter;

    if (valueGetIterator(numbers, &iter))
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

static value* builtinGetTupleNth (const value* tuple, int n) {
    const value* nth = valueGetTupleNth(tuple, n);

    if (!nth)
        return valueCreateInvalid();

    return (value*) nth;
}

static value* builtinFst (const value* pair) {
    return builtinGetTupleNth(pair, 0);
}

static value* builtinSnd (const value* pair) {
    return builtinGetTupleNth(pair, 1);
}

int compareTuple (const value** left, const value** right) {
    const value *firstOfLeft = valueGetTupleNth(*left, 0),
                *firstOfRight = valueGetTupleNth(*right, 0);

    int64_t indexOfLeft = valueGetInt(firstOfLeft),
            indexOfRight = valueGetInt(firstOfRight);

    return indexOfLeft - indexOfRight;
}

static value* builtinSort (const value* table) {
    vector(const value*) rows = vectorDup(valueGetVector(table), GC_malloc);

    qsort(rows.buffer, rows.length, sizeof(void*),
          (int (*)(const void *, const void *)) compareTuple);

    return valueStoreVector(rows);
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

    addBuiltin(global, "lc",
               typeFn(ts, File, Int),
               valueCreateFn(builtinLinecount));

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
        type *A = typeVar(ts),
             *B = typeVar(ts);
        type* A_B = typeTuple(ts, vectorInitChain(2, malloc, A, B));

        addBuiltin(global, "fst",
                   typeForall(ts, vectorInitChain(2, malloc, A, B),
                       typeFn(ts, A_B, A)),
                   valueCreateFn(builtinFst));
    }

    {
        type *A = typeVar(ts),
             *B = typeVar(ts);
        type* A_B = typeTuple(ts, vectorInitChain(2, malloc, A, B));

        addBuiltin(global, "snd",
                   typeForall(ts, vectorInitChain(2, malloc, A, B),
                       typeFn(ts, A_B, B)),
                   valueCreateFn(builtinSnd));
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
