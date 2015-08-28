#pragma once

#include <vector.h>

#include "common.h"
#include "forward.h"

/*Opaque, use interface below*/
typedef struct value value;

typedef struct valueIter {
    const value* iterable;

    /*Information depends on the iterable implementations kind*/
    union {
        /*Vector*/
        int n;
    };
} valueIter;

/*The value creators allocate objects with a garbage collector!

  These objects are only kept alive by references stored in:
    - The stack and data segments (variables and parameters)
    - Other GC allocated objects

  Therefore if you want a value to be owned by a manually managed object,
  allocate the owner with GC_MALLOC_UNCOLLECTABLE and free with GC_FREE.
  This is just a normal allocation that gets scanned for GC object
  references.*/

typedef value* (*simpleClosureFn)(const void* env, const value* arg);

/*All objects given to these creators must be GC allocated*/

value* valueCreateInvalid (void);
value* valueCreateUnit (void);
value* valueCreateInt (int integer);
value* valueCreateStr (char* str);
value* valueCreateFn (value* (*fnptr)(const value*));
value* valueCreateFile (const char* filename);

value* valueCreateSimpleClosure (const void* env, simpleClosureFn fnptr);

/*Represents a closure by an expression (AST tree) plus arguments to it.
    - These args come in the form of two vectors, a simple map from symbol
      to value, hence their elements must correspond.
    - argSymbols contains first the captured symbols, then the args.
    - The expression can be evaluated once argValues is filled with all
      the corresponding elements.*/
value* valueCreateASTClosure (vector(sym*) argSymbols, vector(value*) argValues, ast* body);

value* valueCreateVector (vector(value*) elements);

/*==== (Kind generic) Operations ====*/

bool valueIsInvalid (const value* v);

/*Both of these return the width of the string representation of a value.
  valuePrint actually prints it.*/
int valueGetWidthOfStr (const value* v);
int valuePrint (const value* v);

/*==== Kind specific operations ====*/

int64_t valueGetInt (const value* num);
const char* valueGetStr (const value* str);
const char* valueGetStrWithLength (const value* str, size_t* length_out);

value* valueCall (const value* fn, const value* arg);

const char* valueGetFilename (const value* file);

bool valueGetIterator (const value* iterable, valueIter* iter_out);
int valueGuessIterLength (valueIter iterator);
const value* valueIterRead (valueIter* iterator);

/*Convert an iterable to a vector*/
vector(const value*) valueGetVector (const value* iterable);

const value* valueGetTupleNth (const value* tuple, int n);
