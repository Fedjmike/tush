#pragma once

#include <vector.h>

#include "forward.h"

typedef struct envCtx {
    /*The elements of these vectors correspond to form a map*/
    //todo intmap
    vector(sym*) symbols;
    vector(sym*) values;

    dirCtx* dirs;
} envCtx;

/*Assumes well-formed input. In particular, the AST should be typed.*/
value* run (envCtx* env, const ast* tree);
