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
        value* (*fnptr)(value*);
        /*SimpleClosure*/
        struct {
            value* (*simpleClosure)(void* env, value*);
            void* simpleEnv;
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

value* valueCreateFn (value* (*fnptr)(value*)) {
    return valueCreate(valueFn, (value) {
        .fnptr = fnptr
    });
}

value* valueCreateSimpleClosure (void* env, value* (*fnptr)(void* env, value* arg)) {
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

void valuePrint (const value* v) {
    switch (v->kind) {
    case valueUnit:
        printf("()");
        return;

    case valueInt:
        printf("%ld", v->integer);
        return;

    case valueStr:
        printf("\"%s\"", v->str);
        return;

    case valueFn:
        printf("<fn at %p>", v->fnptr);
        return;

    case valueSimpleClosure:
        printf("<fn at %p with env. %p>", v->simpleClosure, v->simpleEnv);
        return;

    case valueFile:
        printf("%s", v->filename);
        return;

    case valueVector: {
        printf("[");

        for (int i = 0; i < v->vec.length; i++) {
            if (i != 0)
                printf(", ");

            value* element = vectorGet(v->vec, i);
            valuePrint(element);
        }

        printf("]");
        return;
    }

    case valueInvalid:
        printf("<invalid>");
        return;
    }

    errprintf("Unhandled value kind, %s\n", valueKindGetStr(v->kind));
}

value* valueCall (const value* fn, value* arg) {
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

static bool isIterable (value* iterable) {
    return iterable->kind == valueVector;
}

bool valueGetIterator (value* iterable, valueIter* iter) {
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

vector(const value*) valueGetVector (value* iterable) {
    assert(isIterable(iterable));
    return iterable->vec;
}
