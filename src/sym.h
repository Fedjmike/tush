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

/*Guaranteed to return at least something descriptive*/
const char* symGetName (const sym* symbol);

/*Looks for a symbol visible from a scope, therefore searching the
  recursively up the parents scopes.
    - Returns null on failure.
    - Returns the most recent definition of that name.*/
sym* symLookup (const sym* scope, const char* name);

/*Checks whether a symbol is inside this scope or any scopes also
  contained in this scope.*/
bool symIsInside (const sym* look, const sym* scope);
