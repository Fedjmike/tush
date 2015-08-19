#include "type.h"
#include "type-internal.h"

#include <assert.h>
#include <vector.h>
#include <hashmap.h>

#include "common.h"

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

type* typeInt (typeSys* ts) {
    return typeUnitary(ts, type_Int);
}

type* typeFile (typeSys* ts) {
    return typeUnitary(ts, type_File);
}

static type* typeNonUnitary (typeSys* ts, typeKind kind, type init) {
    type* dt = typeCreate(kind, init);
    vectorPush(&ts->others, dt);
    return dt;
}

type* typeFn (typeSys* ts, type* from, type* to) {
    return typeNonUnitary(ts, type_Fn, (type) {
        .from = from, .to = to
    });
}

type* typeList (typeSys* ts, type* elements) {
    return typeNonUnitary(ts, type_List, (type) {
        .elements = elements
    });
}

type* typeTuple (typeSys* ts, vector(type*) types) {
    return typeNonUnitary(ts, type_Tuple, (type) {
        .types = types
    });
}

type* typeVar (typeSys* ts) {
    return typeNonUnitary(ts, type_Var, (type) {});
}

type* typeForall (typeSys* ts, vector(type*) typevars, type* dt) {
    return typeNonUnitary(ts, type_Forall, (type) {
        .typevars = typevars, .dt = dt
    });
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

/*==== String representation ===*/

typedef struct strCtx {
    /*Use a vector because n is small?*/
    intmap(type*, const char*) typevars;
    int namesGiven;
} strCtx;

static const char* strNewTypevar (strCtx* ctx) {
    static const char* strs[10] = {"'a", "'b", "'c", "'d", "'e", "'f", "'g", "'h", "'i", "'j"};

    if (ctx->namesGiven >= 10) {
        errprintf("Run out of static strings to allocate\n");
        return "'";

    } else
        return strs[ctx->namesGiven++];
}

static const char* strMapTypevar (strCtx* ctx, const type* dt) {
    if (mapNull(ctx->typevars))
        ctx->typevars = intmapInit(32, malloc);

    const char* str = intmapMap(&ctx->typevars, (intptr_t) dt);

    /*Get a new string if the typevar isn't already mapped*/
    if (!str) {
        str = strNewTypevar(ctx);
        intmapAdd(&ctx->typevars, (intptr_t) dt, (void*) str);
    }

    return str;
}

static const char* typeGetStrImpl (strCtx* ctx, type* dt) {
    if (dt->str)
        return dt->str;

    switch (dt->kind) {
    case type_Unit: return "()";
    case type_Int: return "Int";
    case type_Num: return "Num";
    case type_Str: return "Str";
    case type_File: return "File";
    case type_Invalid: return "<invalid>";
    case type_KindNo: return "<KindNo, not real>";

    case type_Fn: {
        const char *from = typeGetStrImpl(ctx, dt->from),
                   *to = typeGetStrImpl(ctx, dt->to);

        bool higherOrderFn = dt->from->kind == type_Fn;

        dt->str = malloc((higherOrderFn ? 2 : 0) + strlen(from) + 4 + strlen(to) + 1);
        sprintf(dt->str, higherOrderFn ? "(%s) -> %s" : "%s -> %s", from, to);
        return dt->str;
    }

    case type_List: {
        const char* elements = typeGetStrImpl(ctx, dt->elements);
        dt->str = malloc(strlen(elements) + 2 + 1);
        sprintf(dt->str, "[%s]", elements);
        return dt->str;
    }

    case type_Tuple: {
        vector(const char*) typeStrs = vectorInit(dt->types.length, malloc);
        int length = 0;

        /*Work out the length of all the subtypes and store their strings*/
        for_vector (type* dt, dt->types, {
            const char* typeStr = typeGetStrImpl(ctx, dt);
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

    case type_Var:
        return strMapTypevar(ctx, dt);

    case type_Forall:
        //todo add typevars to ensure order
        return typeGetStrImpl(ctx, dt->dt);
    }

    return "<unhandled type kind>";
}

strCtx strCtxInit () {
    return (strCtx) {
        /*Avoid a malloc unless necessary*/
        .typevars = {},
        .namesGiven = 0
    };
}

strCtx* strCtxFree (strCtx* ctx) {
    intmapFree(&ctx->typevars);
    return ctx;
}

const char* typeGetStr (const type* dt) {
	/*The context keeps track of typevars already given names
	  and which names have been given.*/
    strCtx ctx = strCtxInit();
    const char* str = typeGetStrImpl(&ctx, (type*) dt);
    strCtxFree(&ctx);

    return str;
}

/*==== Tests ====*/

static void seeThroughQuantifier (const type** dt) {
    if ((*dt)->kind == type_Forall)
        *dt = (*dt)->dt;
}

/*Returns whether the type is logically of a kind
  There may be indirection through quantifiers.
  Do not use this if you will access the structure of the type.*/
bool typeIsKind (typeKind kind, const type* dt) {
    seeThroughQuantifier(&dt);
    return dt->kind == kind;
}

bool typeIsInvalid (const type* dt) {
    seeThroughQuantifier(&dt);
    return dt->kind == type_Invalid;
}

bool typeIsFn (const type* dt) {
    return typeIsKind(type_Fn, dt);
}

bool typeIsList (const type* dt) {
    return typeIsKind(type_List, dt);
}

bool typeIsEqual (const type* l, const type* r) {
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

/*==== Operations ====*/

bool typeAppliesToFn (typeSys* ts, const type* arg, const type* fn, type** result) {
    assert(arg);

    if (!typeIsFn(fn))
        return false;

    else if (fn->kind == type_Fn) {
        bool applies = typeIsEqual(fn->from, arg);

        if (applies && result)
            *result = fn->to;

        return applies;

    /*The function is quantified, so find the types which satisfy this application*/
    } else if (fn->kind == type_Forall && fn->dt->kind == type_Fn) {
        type* unifiedFn = unifyArgWithFn(ts, arg, fn);

        if (unifiedFn)
            *result = unifiedFn->to;

        return unifiedFn != 0;

    } else {
        errprintf("Unknown function representation, kind %d, %s\n", fn->kind, typeGetStr(fn));
        *result = typeInvalid(ts);
        return true;
    }
}

bool typeUnitAppliesToFn (const type* fn, type** result) {
    bool applies = fn->kind == type_Fn && fn->from->kind == type_Unit;

    if (applies && result)
        *result = fn->to;

    return applies;
}

bool typeIsListOf (const type* dt, type** elements) {
    seeThroughQuantifier(&dt);

    if (dt->kind == type_List) {
        *elements = dt->elements;
        return true;

    } else
        return false;
}

bool typeIsTupleOf (const type* dt, vector(const type*)* types) {
    seeThroughQuantifier(&dt);

    if (dt->kind == type_Tuple) {
        *types = dt->types;
        return true;

    } else
        return false;
}
