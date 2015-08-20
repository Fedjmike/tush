#pragma once

#include "forward.h"

typedef struct envCtx {

} envCtx;

value* run (envCtx* env, const ast* tree);
