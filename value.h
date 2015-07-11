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

/*valueCreateXXX allocate objects with a garbage collector!

  These objects are only kept alive by references stored in:
    - The stack and data segments (variables and parameters)
    - Other GC allocated objects

  Therefore if you want a value to be owned by a manually managed object,
  allocate that one with GC_MALLOC_UNCOLLECTABLE and free with GC_FREE.
  This is just a normal allocation that gets scanned for GC object
  references.*/

value* valueCreateInt (int integer);
value* valueCreateFn (value* (*fnptr)(value*));
value* valueCreateFile (const char* filename);

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
