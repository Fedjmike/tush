#pragma once

#include <vector.h>

typedef enum typeKind {
    type_Integer, type_Number, type_File, type_Fn,
    type_KindNo
} typeKind;

typedef struct type type;

typedef struct typeSys {
    /*Store all the types ever allocated, free them at the end*/
    type* basics[type_KindNo];
    vector(type*) fns;
} typeSys;

typeSys typesInit (void);
typeSys* typesFree (typeSys* ts);

type* typeInteger (typeSys* ts);
type* typeFile (typeSys* ts);
type* typeFn (typeSys* ts, type* from, type* to);

//todo
type* typeFnChain (typeSys ts, int n, ...);
