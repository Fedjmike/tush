#include "sym.h"

#include <assert.h>

#include "common.h"

static void symAddChild(sym* parent, sym* child) {
    assert(parent);
    assert(child);
    assert(!child->parent);

    vectorPush(&parent->children, child);
    child->parent = parent;
}

static sym* symCreate (const char* name) {
    return malloci(sizeof(sym), &(sym) {
        .children = vectorInit(4, malloc),
        .name = strdup(name)
    });
}

static void symDestroy (sym* symbol) {
    vectorFreeObjs(&symbol->children, (vectorDtor) symDestroy);
    free(symbol->name);
    free(symbol);
}

sym* symInit (void) {
    return symCreate("<global namespace>");
}

void symEnd (sym* global) {
    symDestroy(global);
}

sym* symAdd (sym* parent, const char* name) {
    sym* symbol = symCreate(name);
    symAddChild(parent, symbol);
    return symbol;
}
