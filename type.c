#include "type.h"

#include <assert.h>
#include <vector.h>

#include "common.h"

typedef struct type {
    typeKind kind;

    union {
        /*Fn*/
        struct {
            type *from, *to;
        };
    };
} type;

/*==== Type ctors and dtors ====*/

static type* typeCreate (typeKind kind, type init) {
    type* dt = malloci(sizeof(type), &init);
    dt->kind = kind;
    return dt;
}

static void typeDestroy (type* dt) {
    free(dt);
}

static type* typeBasic (typeSys* ts, typeKind kind) {
    assert(kind != type_Fn);

    /*Only allocate one struct per basic type*/
    if (!ts->basics[kind])
        ts->basics[kind] = typeCreate(kind, (type) {});

    return ts->basics[kind];
}

type* typeInteger (typeSys* ts) {
    return typeBasic(ts, type_Integer);
}

type* typeFile (typeSys* ts) {
    return typeBasic(ts, type_File);
}

type* typeFn (typeSys* ts, type* from, type* to) {
    type* dt = typeCreate(type_Fn, (type) {
        .from = from, .to = to
    });
    vectorPush(&ts->fns, dt);
    return dt;
}

/*==== Type system ====*/

typeSys typesInit (void) {
    return (typeSys) {
        .basics = {},
        .fns = vectorInit(100, malloc)
    };
}

typeSys* typesFree (typeSys* ts) {
    for (unsigned int i = 0; i < sizeof(ts->basics)/sizeof(*ts->basics); i++)
        typeDestroy(ts->basics[i]);

    vectorFreeObjs(&ts->fns, (vectorDtor) typeDestroy);

    return ts;
}
