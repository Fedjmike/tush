#include "value.h"

#include <stdio.h>
#include <assert.h>
#include <gc/gc.h>

typedef enum valueKind {
    valueInvalid, valueInt, valueFn, valueFile
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

value* valueCreateInvalid (void) {
    static value* invalid;

    if (!invalid)
        invalid = valueCreate(valueInvalid, (value) {});

    return invalid;
}

/*==== Operations on values ====*/

void valuePrint (const value* v) {
    switch (v->kind) {
    case valueInt:
        printf("%ld", v->integer);
        break;

    case valueInvalid:
        printf("<invalid>");
        break;

    default:
        ;//todo
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

const char* valueKindGetStr (valueKind kind) {
    switch (kind) {
    case valueFn: return "Fn";
    case valueFile: return "File";
    case valueInt: return "Int";
    default: return "<unhandled value kind>";
    }
}
