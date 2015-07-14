#pragma once

#include <vector.h>

typedef enum typeKind {
    type_Integer, type_Number, type_File,
    type_Fn, type_List,
    type_Invalid,
    type_KindNo
} typeKind;

typedef struct type type;

/*This store all the types ever allocated, and frees them at the end*/
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

type* typeInteger (typeSys* ts);
type* typeFile (typeSys* ts);
type* typeFn (typeSys* ts, type* from, type* to);
type* typeList (typeSys* ts, type* elements);

type* typeInvalid (typeSys* ts);

//todo
type* typeFnChain (typeSys ts, int n, ...);

/*==== ====*/

const char* typeGetStr (type* dt);

/*==== Tests and operations ====
  Many of these simultaneously:
    1. Check a semantic condition
    2. Produce a type as an out-parameter*/

bool typeAppliesToFn (typeSys* ts, type* arg, type* fn, type** result_out);

bool typeIsList (typeSys* ts, type* dt, type** elements_out);
