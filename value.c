#include "value.h"

#include <gc/gc.h>

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
