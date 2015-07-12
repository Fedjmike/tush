#pragma once

#include <vector.h>

#include "forward.h"

/**
 * Owns its name and children. All symbols are owned by the global
 * symbol created by @see symInit and destroyed with @see symEnd.
 */
typedef struct sym {
    char* name;
    type* dt;

    sym* parent;
    vector(sym*) children;
} sym;

sym* symInit (void);
void symEnd (sym* global);

sym* symAdd (sym* parent, const char* name);

/*Returns null on failure*/
sym* symLookup (sym* scope, const char* name);
