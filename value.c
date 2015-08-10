#include "value.h"

#include <stdio.h>
#include <assert.h>
#include <gc/gc.h>
#include <common.h>

typedef enum valueKind {
    valueInvalid, valueUnit, valueInt, valueStr,
    valueFn, valueSimpleClosure, valueFile, valueVector
} valueKind;

typedef struct value {
    valueKind kind;

    union {
        /*Str*/
        struct {
            char* str;
            size_t strlen;
        };
        /*Fn*/
        value* (*fnptr)(const value*);
        /*SimpleClosure*/
        struct {
            value* (*simpleClosure)(const void* env, const value*);
            const void* simpleEnv;
        };
        /*File*/
        char* filename;
        /*Int*/
        int64_t integer;
        /*Vector*/
        vector(value*) vec;
    };
} value;

static const char* valueKindGetStr (valueKind kind);

/*==== Value creators ====*/

static value* valueCreate (valueKind kind, value init) {
    value* v = GC_MALLOC(sizeof(value));
    *v = init;
    v->kind = kind;
    return v;
}

value* valueCreateUnit (void) {
    return valueCreate(valueUnit, (value) {});
}

value* valueCreateInt (int integer) {
    return valueCreate(valueInt, (value) {
        .integer = integer
    });
}

value* valueCreateStr (char* str) {
    size_t length = strlen(str);

    return valueCreate(valueStr, (value) {
        .str = strcpy(GC_MALLOC(length+1), str), .strlen = length
    });
}

value* valueCreateFn (value* (*fnptr)(const value*)) {
    return valueCreate(valueFn, (value) {
        .fnptr = fnptr
    });
}

value* valueCreateSimpleClosure (const void* env, simpleClosureFn fnptr) {
    return valueCreate(valueSimpleClosure, (value) {
        .simpleClosure = fnptr, .simpleEnv = env
    });
}

value* valueCreateFile (const char* filename) {
    return valueCreate(valueFile, (value) {
        .filename = GC_STRDUP(filename)
    });
}

value* valueCreateVector (vector(value*) elements) {
    return valueCreate(valueVector, (value) {
        .vec = elements
    });
}

value* valueCreateInvalid (void) {
    static value* invalid;

    if (!invalid)
        invalid = valueCreate(valueInvalid, (value) {});

    return invalid;
}

/*==== ====*/

const char* valueKindGetStr (valueKind kind) {
    switch (kind) {
    case valueUnit: return "Unit";
    case valueStr: return "Str";
    case valueFn: return "Fn";
    case valueSimpleClosure: return "SimpleClosure";
    case valueFile: return "File";
    case valueInt: return "Int";
    case valueVector: return "Vector";
    case valueInvalid: return "<Invalid value>";
    }

    return "<unhandled value kind>";
}

/*==== Operations on values ====*/

bool valueIsInvalid (const value* v) {
    return v->kind == valueInvalid;
}

int valuePrintImpl (const value* v, bool dryrun) {
    switch (v->kind) {
    case valueUnit:
        return dryrun ? 2 : printf("()");

    case valueInt:
        return dryrun ? intlen(v->integer)
                      : printf("%ld", v->integer);

    case valueStr:
        return dryrun ? strlen(v->str) + 2
                      : printf("\"%s\"", v->str);

    case valueFn:
        //todo implement
        assert(!dryrun);
        return dryrun ? 0 : printf("<fn at %p>", v->fnptr);

    case valueSimpleClosure:
        //todo implement
        assert(!dryrun);
        return dryrun ? 0 : printf("<fn at %p with env. %p>", v->simpleClosure, v->simpleEnv);

    case valueFile:
        return dryrun ? strlen(v->filename) : printf("%s", v->filename);

    case valueVector: {
        int length = dryrun ? 1 : printf("[");

        for (int i = 0; i < v->vec.length; i++) {
            if (i != 0)
                length += dryrun ? 2 : printf(", ");

            value* element = vectorGet(v->vec, i);
            length += valuePrintImpl(element, dryrun);
        }

        return length += dryrun ? 1 : printf("]");
    }

    case valueInvalid:
        return (dryrun ? strlen : (__typeof__(&strlen)) printf)("<invalid>");
    }

    errprintf("Unhandled value kind, %s\n", valueKindGetStr(v->kind));
    return 0;
}

int valueGetWidthOfStr (const value* v) {
    return valuePrintImpl(v, true);
}

int valuePrint (const value* v) {
    return valuePrintImpl(v, false);
}

value* valueCall (const value* fn, const value* arg) {
    switch (fn->kind) {
    case valueFn:
        return fn->fnptr(arg);

    case valueSimpleClosure:
        return fn->simpleClosure(fn->simpleEnv, arg);

    default:
        errprintf("Unhandled value kind, %s\n", valueKindGetStr(fn->kind));
        return valueCreateInvalid();
    }
}

const char* valueGetFilename (const value* value) {
    if (value->kind == valueInvalid)
        return 0;

    assert(   value->kind == valueFile
           || value->kind == valueStr);

    if (value->kind == valueFile)
        return value->filename;

    else
        return value->str;
}

static bool isIterable (const value* iterable) {
    return iterable->kind == valueVector;
}

bool valueGetIterator (const value* iterable, valueIter* iter) {
    if (!isIterable(iterable))
        return false;

    else if (iterable->kind == valueVector) {
        *iter = (valueIter) {
            .iterable = iterable, .n = 0
        };
        return true;

    } else {
        errprintf("Unhandled iterable kind, %s\n", valueKindGetStr(iterable->kind));
        return false;
    }
}

int valueGuessIterLength (valueIter iterator) {
    assert(iterator.iterable->kind == valueVector);
    return iterator.iterable->vec.length;
}

value* valueIterRead (valueIter* iterator) {
    assert(iterator->iterable->kind == valueVector);
    return vectorGet(iterator->iterable->vec, iterator->n++);
}

vector(const value*) valueGetVector (const value* iterable) {
    assert(isIterable(iterable));
    return iterable->vec;
}
