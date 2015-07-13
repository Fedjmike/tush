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
        /*List*/
        type* elements;
    };

    /*Not used by all types
      Allocated in typeGetStr, if at all*/
    char* str;
} type;


static bool typeKindIsntUnitary (typeKind kind) {
    return kind != type_Fn && kind != type_List;
}

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

static type* typeUnitary (typeSys* ts, typeKind kind) {
    assert(typeKindIsntUnitary(kind));

    /*Only allocate one struct per unitary type*/
    if (!ts->unitaries[kind])
        ts->unitaries[kind] = typeCreate(kind, (type) {});

    return ts->unitaries[kind];
}

type* typeInteger (typeSys* ts) {
    return typeUnitary(ts, type_Integer);
}

type* typeFile (typeSys* ts) {
    return typeUnitary(ts, type_File);
}

type* typeFn (typeSys* ts, type* from, type* to) {
    type* dt = typeCreate(type_Fn, (type) {
        .from = from, .to = to
    });
    vectorPush(&ts->others, dt);
    return dt;
}

type* typeList (typeSys* ts, type* elements) {
    type* dt = typeCreate(type_List, (type) {
        .elements = elements
    });
    vectorPush(&ts->others, dt);
    return dt;
}

type* typeInvalid (typeSys* ts) {
    return typeUnitary(ts, type_Invalid);
}

/*==== Type system ====*/

typeSys typesInit (void) {
    return (typeSys) {
        .unitaries = {},
        .others = vectorInit(100, malloc)
    };
}

typeSys* typesFree (typeSys* ts) {
    for (unsigned int i = 0; i < sizeof(ts->unitaries)/sizeof(*ts->unitaries); i++)
        if (ts->unitaries[i])
            typeDestroy(ts->unitaries[i]);

    vectorFreeObjs(&ts->others, (vectorDtor) typeDestroy);

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

    case type_List: {
        const char* elements = typeGetStr(dt->elements);
        dt->str = malloc(strlen(elements) + 2 + 1);
        sprintf(dt->str, "[%s]", elements);
        return dt->str;
    }
    }
}

/*==== Tests and operations ====*/

static bool typeEquals (type* l, type* r) {
    if (l == r || l->kind == type_Invalid || r->kind == type_Invalid)
        return true;

    else
        //todo
        return false;
}

bool typeAppliesToFn (typeSys* ts, type* arg, type* fn, type** result) {
    if (typeEquals(fn->from, arg)) {
        *result = fn->to;
        return true;

    } else {
        *result = typeInvalid(ts);
        return false;
    }
}
