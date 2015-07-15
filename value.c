#include "value.h"

#include <stdio.h>
#include <assert.h>
#include <gc/gc.h>

typedef enum valueKind {
    valueInvalid, valueInt, valueFn, valueFile, valueVector
} valueKind;

typedef struct value {
    valueKind kind;

    union {
        /*Fn*/
        value* (*fnptr)(value*);
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

value* valueCreateInt (int integer) {
    return valueCreate(valueInt, (value) {
        .integer = integer
    });
}

value* valueCreateFn (value* (*fnptr)(value*)) {
    return valueCreate(valueFn, (value) {
        .fnptr = fnptr
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
    case valueFn: return "Fn";
    case valueFile: return "File";
    case valueInt: return "Int";
    default: return "<unhandled value kind>";
    }
}

/*==== Operations on values ====*/

void valuePrint (const value* v) {
    switch (v->kind) {
    case valueInt:
        printf("%ld", v->integer);
        break;

    case valueFn:
        printf("[fn at %p]", v->fnptr);
        break;

    case valueFile:
        printf("%s", v->filename);
        break;

    case valueVector: {
        printf("[");

        for (int i = 0; i < v->vec.length; i++) {
            if (i != 0)
                printf(", ");

            value* element = vectorGet(v->vec, i);
            valuePrint(element);
        }

        printf("]");
        break;
    }

    case valueInvalid:
        printf("<invalid>");
        break;

    default:
        errprintf("Unhandled value kind, %s\n", valueKindGetStr(v->kind));
    }
}

value* valueCall (const value* fn, value* arg) {
    if (fn->kind == valueFn)
        return fn->fnptr(arg);

    else {
        errprintf("Unhandled value kind, %s\n", valueKindGetStr(fn->kind));
        return valueCreateInvalid();
    }
}

const char* valueGetFilename (const value* value) {
    if (value->kind != valueFile)
        return 0;

    return value->filename;
}

bool valueGetIterator (value* iterable, valueIter* iter) {
    if (iterable->kind != valueVector)
        return false;

    else {
        *iter = (valueIter) {
            .iterable = iterable, .n = 0
        };
        return true;
    }
}

int valueGuessIterSize (valueIter iterator) {
    assert(iterator.iterable->kind == valueVector);
    return iterator.iterable->vec.length;
}

value* valueIterRead (valueIter* iterator) {
    assert(iterator->iterable->kind == valueVector);
    return vectorGet(iterator->iterable->vec, iterator->n++);
}
