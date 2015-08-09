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
    return kind == type_Fn || kind == type_List;
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

type* typeUnitary (typeSys* ts, typeKind kind) {
    assert(!typeKindIsntUnitary(kind));

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

type* typeFnChain (int kindNo, typeSys* ts, ...) {
    type* result;

    va_list args;
    va_start(args, ts);

    for (int i = 0; i < kindNo; i++) {
        typeKind kind = va_arg(args, int);
        assert(!typeKindIsntUnitary(kind));

        type* dt = typeUnitary(ts, kind);

        /*On the first iteration the result is just the unitary type
          (using the induction variable to make it easy for the compiler to optimize)*/
        if (i == 0)
            result = dt;

        else
            result = typeFn(ts, result, dt);
    }

    va_end(args);

    return result;
}

const char* typeGetStr (type* dt) {
    if (dt->str)
        return dt->str;

    switch (dt->kind) {
    case type_Unit: return "()";
    case type_Integer: return "Integer";
    case type_Str: return "Str";
    case type_Number: return "Number";
    case type_File: return "File";
    case type_Invalid: return "<invalid>";
    case type_KindNo: return "<KindNo, not real>";

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

    return "<unhandled type kind>";
}

/*==== Tests and operations ====*/

bool typeIsInvalid (type* dt) {
    return dt->kind == type_Invalid;
}

bool typeIsKind (typeKind kind, type* dt) {
    return dt->kind == kind;
}

bool typeIsEqual (type* l, type* r) {
    assert(l);
    assert(r);

    if (l == r)
        return true;

    else if (l->kind != r->kind)
        return false;

    else {
        switch (l->kind) {
        case type_Fn:
            return    typeIsEqual(l->from, r->from)
                   && typeIsEqual(l->to, r->to);

        case type_List:
            return typeIsEqual(l->elements, r->elements);

        default:
            errprintf("Unhandled type kind, %s\n", typeGetStr(l));
            return false;
            //todo
        }
    }
}

bool typeIsFn (type* dt) {
    return dt->kind == type_Fn;
}

bool typeAppliesToFn (type* arg, type* fn) {
    assert(arg);
    return fn->kind == type_Fn && typeIsEqual(fn->from, arg);
}

bool typeUnitAppliesToFn (type* fn) {
    return fn->kind == type_Fn && fn->from->kind == type_Unit;
}

type* typeGetFnResult (type* fn) {
    assert(fn->kind == type_Fn);
    return fn->to;
}

bool typeIsList (type* dt) {
    return dt->kind == type_List;
}

type* typeGetListElements (type* dt) {
    assert(typeIsList(dt));
    return dt->elements;
}
