#pragma once

#include "forward.h"

typedef struct analyzerResult {
    int errors;
} analyzerResult;

analyzerResult analyze (typeSys* ts, ast* node);
