#pragma once

#include <vector.h>

typedef enum typeKind {
    type_Unit,
    type_Integer, type_Number,
    type_Str,
    type_File,
    type_Fn, type_List,
    type_Invalid,
    type_KindNo
} typeKind;

typedef struct type type;

/*This stores all the types ever allocated, and frees them at the end*/
typedef struct typeSys {
    /*Unitary types are those that all instances are conceptually the
      same. That is, the type has no parameters. Therefore only one ever
      need be allocated, and we can easily index them by kind.*/
    type* unitaries[type_KindNo];

    vector(type*) others;
} typeSys;

typeSys typesInit (void);
typeSys* typesFree (typeSys* ts);

/*==== Type getters ====
  Types are immutable and their allocation is handled by the type
  system. These functions give you a reference to them.*/

type* typeUnitary (typeSys* ts, typeKind kind);

type* typeInteger (typeSys* ts);
type* typeFile (typeSys* ts);
type* typeFn (typeSys* ts, type* from, type* to);
type* typeList (typeSys* ts, type* elements);

type* typeInvalid (typeSys* ts);

/*A helper function for construction fn types.
  Accepts a list of typeKinds, which must be unitary types.*/
type* typeFnChain (int kindNo, typeSys* ts, ...);

/*==== ====*/

const char* typeGetStr (type* dt);

/*==== Tests and getters ====*/

bool typeIsInvalid (type* dt);
bool typeIsKind (type* dt, typeKind kind);

bool typeIsEqual (type* l, type* r);

bool typeIsFn (type* dt);
bool typeAppliesToFn (type* arg, type* fn);
bool typeUnitAppliesToFn (type* fn);
type* typeGetFnResult (type* fn);

bool typeIsList (type* dt);
type* typeGetListElements (type* dt);
