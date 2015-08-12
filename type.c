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
        /*Tuple*/
        vector(type*) types;
    };

    /*Not used by all types
      Allocated in typeGetStr, if at all*/
    char* str;
} type;


static bool typeKindIsntUnitary (typeKind kind) {
    return kind == type_Fn || kind == type_List || kind == type_Tuple;
}

/*==== Type ctors and dtors ====*/

static type* typeCreate (typeKind kind, type init) {
    assert(kind != type_KindNo);

    type* dt = malloci(sizeof(type), &init);
    dt->kind = kind;
    return dt;
}

static void typeDestroy (type* dt) {
    if (dt->kind == type_Tuple)
        vectorFree(&dt->types);

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

type* typeTuple (typeSys* ts, vector(type*) types) {
    type* dt = typeCreate(type_Tuple, (type) {
        .types = types
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

type* typeTupleChain (int arity, typeSys* ts, ...) {
    va_list args;
    va_start(args, ts);

    vector(type*) types = vectorInit(arity, malloc);

    /*Turn the arg list into a vector*/
    for (int i = 0; i < arity; i++) {
        type* dt = va_arg(args, type*);
        vectorPush(&types, dt);
    }

    va_end(args);

    return typeTuple(ts, types);
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

        bool higherOrderFn = dt->from->kind == type_Fn;

        dt->str = malloc((higherOrderFn ? 2 : 0) + strlen(from) + 4 + strlen(to) + 1);
        sprintf(dt->str, higherOrderFn ? "(%s) -> %s" : "%s -> %s", from, to);
        return dt->str;
    }

    case type_List: {
        const char* elements = typeGetStr(dt->elements);
        dt->str = malloc(strlen(elements) + 2 + 1);
        sprintf(dt->str, "[%s]", elements);
        return dt->str;
    }

    case type_Tuple: {
        vector(const char*) typeStrs = vectorInit(dt->types.length, malloc);
        int length = 0;

        /*Work out the length of all the subtypes and store their strings*/
        for_vector (type* dt, dt->types, {
            const char* typeStr = typeGetStr(dt);
            length += strlen(typeStr) + 2;
            vectorPush(&typeStrs, typeStr);
        })

        length += 3;
        dt->str = malloc(length);
        strcpy(dt->str, "(");

        bool first = true;
        int pos = 1;

        for_vector(const char* typeStr, typeStrs, {
            pos += snprintf(dt->str+pos, length-pos, first ? "%s" : ", %s", typeStr);
            first = false;
        })

        strcpy(dt->str+pos, ")");

        vectorFree(&typeStrs);

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

        case type_Tuple:
            if (l->types.length != r->types.length)
                return false;

            for (int i = 0; i < l->types.length; i++) {
                type *typeL = vectorGet(l->types, i),
                     *typeR = vectorGet(r->types, i);

                if (!typeIsEqual(typeL, typeR))
                    return false;
            }

            return true;

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

vector(const type*) typeGetTupleTypes (type* dt) {
    assert(typeIsKind(type_Tuple, dt));
    return dt->types;
}
