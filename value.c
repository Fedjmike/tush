#include "value.h"

#include <stdio.h>
#include <assert.h>
#include <gc/gc.h>

typedef enum valueKind {
    valueInt, valueFn, valueFile
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
    };
} value;

/*==== Value creators ====*/

static value* valueCreate (value init) {
    value* v = GC_MALLOC(sizeof(value));
    *v = init;
    return v;
}

value* valueCreateInt (int integer) {
    return valueCreate((value) {
        .kind = valueInt, .integer = integer
    });
}

value* valueCreateFn (value* (*fnptr)(value*)) {
    return valueCreate((value) {
        .kind = valueFn, .fnptr = fnptr
    });
}

value* valueCreateFile (const char* filename) {
    return valueCreate((value) {
        .kind = valueFile, .filename = strdup(filename)
    });
}

/*==== Operations on values ====*/

void valuePrint (const value* v) {
    if (v->kind == valueInt)
        printf("%ld: Int\n", v->integer);

    else
        ;//todo
}

value* valueCall (const value* fn, value* arg) {
    if (fn->kind == valueFn)
        return fn->fnptr(arg);

    else {
        errprintf("Unhandled value kind, %d\n", fn->kind);
        return 0;
    }
}

const char* valueGetFilename (const value* value) {
    assert(value->kind == valueFile);
    return value->filename;
}