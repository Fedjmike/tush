#pragma once

#include <vector.h>

typedef enum typeKind {
    type_Unit,
    type_Integer, type_Number,
    type_Str,
    type_File,
    type_Fn, type_List, type_Tuple,
    type_Var, type_Forall,
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
type* typeTuple (typeSys* ts, vector(type*) types);

type* typeVar (typeSys* ts);
type* typeForall (typeSys* ts, vector(type*) typevars, type* dt);

type* typeInvalid (typeSys* ts);

/*A helper function for construction fn types.
  Accepts a list of typeKinds, which must be unitary types.*/
type* typeFnChain (int kindNo, typeSys* ts, ...);

/*Accepts a list of type* */
type* typeTupleChain (int arity, typeSys* ts, ...);

/*==== ====*/

const char* typeGetStr (const type* dt);

/*==== Tests and getters ====*/

bool typeIsKind (typeKind kind, const type* dt);
bool typeIsInvalid (const type* dt);
bool typeIsFn (const type* dt);

bool typeIsEqual (const type* l, const type* r);

bool typeIsFn (type* dt);
bool typeAppliesToFn (typeSys* ts, const type* arg, const type* fn, type** result);
bool typeUnitAppliesToFn (type* fn, type** result);

bool typeIsList (const type* dt);
type* typeGetListElements (const type* dt);

vector(const type*) typeGetTupleTypes (type* dt);
