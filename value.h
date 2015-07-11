#pragma once

#include "common.h"
#include "forward.h"

typedef enum valueKind {
    valueFn, valueFile, valueInt
} valueKind;

typedef struct value {
    valueKind kind;

    union {
        /*Fn*/
        //todo n-ary optimization
        value* (*fnptr)(value*);
        /*File*/
        char* filename;
        /*Int*/
        int64_t integer;
    };
} value;

static void valuePrint (const value* v);

static value* valueCall (const value* fn, value* arg);

/*==== Inline implementations ====*/

inline void valuePrint (const value* v) {
    if (v->kind == valueInt)
        printf("%ld: Int\n", v->integer);

    else
        ;//todo
}

inline value* valueCall (const value* fn, value* arg) {
    if (fn->kind == valueFn)
        return fn->fnptr(arg);

    else {
        errprintf("Unhandled value kind, %d\n", fn->kind);
        return 0;
    }
}
