#pragma once

#include "common.h"
#include "forward.h"

/*Opaque, use interface below*/
typedef struct value value;

/*The value creators allocate objects with a garbage collector!

  These objects are only kept alive by references stored in:
    - The stack and data segments (variables and parameters)
    - Other GC allocated objects

  Therefore if you want a value to be owned by a manually managed object,
  allocate the owner with GC_MALLOC_UNCOLLECTABLE and free with GC_FREE.
  This is just a normal allocation that gets scanned for GC object
  references.*/

value* valueCreateInt (int integer);
value* valueCreateFn (value* (*fnptr)(value*));
value* valueCreateFile (const char* filename);

void valuePrint (const value* v);

value* valueCall (const value* fn, value* arg);

const char* valueGetFilename (const value* file);
