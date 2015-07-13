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

    /*Not used by all types
      Allocated in typeGetStr, if at all*/
    char* str;
} type;

/*==== Type ctors and dtors ====*/

static type* typeCreate (typeKind kind, type init) {
    assert(kind != type_KindNo);

    type* dt = malloci(sizeof(type), &init);
    dt->kind = kind;
    return dt;
}

static void typeDestroy (type* dt) {
    free(dt->str);
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

type* typeInvalid (typeSys* ts) {
    return typeBasic(ts, type_Invalid);
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
        if (ts->basics[i])
            typeDestroy(ts->basics[i]);

    vectorFreeObjs(&ts->fns, (vectorDtor) typeDestroy);

    return ts;
}

/*==== ====*/

const char* typeGetStr (type* dt) {
    switch (dt->kind) {
    case type_Integer: return "Integer";
    case type_Number: return "Number";
    case type_File: return "File";
    case type_Invalid: return "<invalid>";
    case type_KindNo: return "<KindNo, not real>";
    default: return "<unhandled type kind>";

    case type_Fn: {
        const char *from = typeGetStr(dt->from),
                   *to = typeGetStr(dt->to);

        dt->str = malloc(strlen(from) + 4 + strlen(to) + 1);
        sprintf(dt->str, "%s -> %s", from, to);
        return dt->str;
    }
    }
}

/*==== Tests and operations ====*/

bool typeAppliesToFn (typeSys* ts, type* arg, type* fn, type** result) {
    if (fn->from == arg) {
        *result = fn->to;
        return true;

    } else {
        *result = typeInvalid(ts);
        return false;
    }
}
