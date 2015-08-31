#pragma once

#include <vector.h>

#include "common.h"
#include "forward.h"

/*Opaque, use interface below*/
typedef struct value value;

typedef enum iterKind {
    iterVector, iterPair, iterTriple, iterInvalid
} iterKind;

typedef struct valueIter {
    iterKind kind;
    const value* iterable;
    /*The index of the most recently read item*/
    int index;
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

value* valueStoreTuple (int n, ...);
value* valueStoreArray (int n, value** const array);
/*Takes ownership of v*/
value* valueStoreVector (vector(value*) v);

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

//todo can fail
//fallback param?
const char* valueGetFilename (const value* file);

/*---- Iterables ----*/

int valueGuessIterableLength (const value* iterable);

/*Get an iterator for an iterable value, through an out parameter.
  Returns true on failure, and gives an iterator that has no elements.*/
bool valueGetIterator (const value* iterable, valueIter* iter_out);

/*Read the next value from an iterator. Returns null when the iterator
  has reached the end.*/
const value* valueIterRead (valueIter* iterator);

/*Iterate over an iterable. If the value given is not an iterable,
  no iterations will occur and no error state indicated.

  \param   i
  The name of a variable that will be declared int, and hold the
  current index.

  \param   name
  The name of a variable that will be declared const value* and
  hold the current value in the iterable.

  \param   const value* iterable
  The value to be iterated over.

  \param   continuation
  The body of the iteration (including braces)*/
#define for_iterable_value_indexed(i, decl, iterable, continuation) \
    do {valueIter __for_iter; \
        valueGetIterator((iterable), &__for_iter); \
        for (const value* __for_element; \
             (__for_element = valueIterRead(&__for_iter));) { \
            decl = __for_element; int (i) = __for_iter.index; \
            {continuation} \
        } \
    } while (0);

#define for_iterable_value(decl, iterable, continuation) \
    for_iterable_value_indexed(__for_dummy, decl, iterable, {(void) __for_dummy; continuation})

/*Convert an iterable to a vector*/
vector(const value*) valueGetVector (const value* iterable);

const value* valueGetTupleNth (const value* tuple, int n);
