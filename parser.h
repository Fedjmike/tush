#pragma once

#include "forward.h"

typedef struct parserResult {
    ast* tree;
    int errors;
} parserResult;

parserResult parse (sym* global, lexerCtx* lexer);
