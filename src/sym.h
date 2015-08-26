#pragma once

#include <vector.h>

#include "forward.h"

typedef enum symKind {
    symScope, symNormal
} symKind;

/**
 * Owns its name and children. All symbols are owned by the global
 * symbol created by @see symInit and destroyed with @see symEnd.
 */
typedef struct sym {
    symKind kind;

    char* name;
    type* dt;
    value* val;

    sym* parent;
    vector(sym*) children;
} sym;

sym* symInit (void);
void symEnd (sym* global);

sym* symAdd (sym* parent, const char* name);
sym* symAddScope (sym* parent);

/*Returns null on failure*/
sym* symLookup (sym* scope, const char* name);
