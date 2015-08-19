#include <vector.h>
#include <hashmap.h>

#include "common.h"
#include "type.h"
#include "type-internal.h"

typedef struct inference {
    vector(const type*) typevars;
    const type* closed;
} inference;

typedef struct inferences {
    intset(const type* typevar) bound;
    vector(inference*) v;
} inferences;

static inference* infsAdd (inferences* infs, const type* typevar, const type* closed) {
    inference* inf = malloci(sizeof(inference), &(inference) {
        .typevars = vectorInit(3, malloc),
        .closed = closed
    });
    vectorPush(&inf->typevars, typevar);
    vectorPush(&infs->v, inf);
    return inf;
}

static void infDestroy (inference* inf) {
    vectorFree(&inf->typevars);
    free(inf);
}

inferences infsInit (stdalloc allocator) {
    return (inferences) {
        .bound = intsetInit(16, allocator),
        .v = vectorInit(8, allocator)
    };
}

inferences* infsFree (inferences* infs) {
    intsetFree(&infs->bound);
    vectorFreeObjs(&infs->v, (vectorDtor) infDestroy);
    return infs;
}

static bool infAddClosed (inference* inf, const type* closed) {
    if (inf->closed)
        return true;

    inf->closed = closed;
    return false;
}

static void infAddTypevar (inference* inf, const type* typevar) {
    vectorPush(&inf->typevars, typevar);
}

static inference* infsLookup (const inferences* infs, const type* given) {
    for_vector (inference* inf, infs->v, {
        if (vectorFind(inf->typevars, (void*) given) >= 0)
            return inf;
    })

    return 0;
}

static bool infsMerge (inferences* infs, inference* l, inference* r) {
    /*Conflict*/
    if (l->closed && r->closed && l->closed != r->closed)
        //todo ptr unequal, structurally equal?
        return true;

    /*Transfer everything from the right to the left*/

    vectorPushFromVector(&l->typevars, r->typevars);

    if (r->closed)
        l->closed = r->closed;

    vectorRemoveReorder(&infs->v, vectorFind(infs->v, r));
    infDestroy(r);

    return false;
}

bool inferEqual (inferences* infs, const type* l, const type* r) {
    //todo
    bool lIsBoundTypevar = true,
         rIsBoundTypevar = true;

    if (lIsBoundTypevar && rIsBoundTypevar) {
        /*Do they already have inferences?*/
        inference *lInf = infsLookup(infs, l),
                  *rInf = infsLookup(infs, r);

        if (lInf && rInf)
            return infsMerge(infs, lInf, rInf); //possible conflict

        else if (lInf)
            infAddTypevar(lInf, r);

        else if (rInf)
            infAddTypevar(rInf, l);

        else
            infAddTypevar(infsAdd(infs, l, 0), r);

    } else if (lIsBoundTypevar) {
        inference* inf = infsLookup(infs, l);

        if (inf)
            return infAddClosed(inf, r); //p. conflict

        else
            infsAdd(infs, l, r);

    } else {
        /*The right is the typevar.
          Switch them around and have the previous case handle them.*/
        return inferEqual(infs, r, l); //p. conflict
    }

    return false;
}


bool typeUnifies (typeSys* ts, inferences* infs, const type* l, const type* r) {
    if (l->kind == type_Var || r->kind == type_Var) {
        //todo wtf??!
        inferEqual(infs, l, r);
        return (type*) l;

    } else if (l->kind != r->kind)
        return 0;

    /*Kinds equal*/
    else {
        if (!typeKindIsntUnitary(l->kind))
            return l == r ? (type*) l : 0;

        switch (l->kind) {
        case type_Fn:
            return    typeUnifies(ts, infs, l->from, r->from)
                   && typeUnifies(ts, infs, l->to, r->to);

        case type_List:
            return typeUnifies(ts, infs, l->elements, r->elements);

        default:
            errprintf("Unhandled type, kind %d, %s\n", l->kind, typeGetStr(l));
            return false;
        }
    }
}

type* typeMakeSubs (typeSys* ts, const inferences* infs, const type* dt) {
    switch (dt->kind) {
    case type_Unit:
    case type_Integer:
    case type_Str:
    case type_Number:
    case type_File:
    case type_Invalid:
        return (type*) dt;

    case type_Fn: {
        type *from = typeMakeSubs(ts, infs, dt->from),
             *to = typeMakeSubs(ts, infs, dt->to);

        return typeFn(ts, from, to);

    } case type_List: {
        return typeList(ts, typeMakeSubs(ts, infs, dt->elements));

    } case type_Tuple: {
        vector(type*) types = vectorInit(dt->types.length, malloc);

        for_vector (type* tupledt, dt->types, {
            vectorPush(&types, typeMakeSubs(ts, infs, tupledt));
        })

        return typeTuple(ts, types);

    } case type_Var: {
        inference* inf = infsLookup(infs, dt);

        if (inf) {
            if (inf->closed)
                return (type*) inf->closed;

            else {
                assert(inf->typevars.length >= 1);
                return vectorGet(inf->typevars, 0);
            }

        } else
            return (type*) dt;

    } case type_Forall: {
        return typeMakeSubs(ts, infs, dt->dt);

    } case type_KindNo:
        errprintf("Dummy type kind 'KindNo' found in the wild\n");
        return (type*) dt;
    }

    errprintf("Unhandled type, kind %d, %s\n", dt->kind, typeGetStr(dt));
    return (type*) dt;
}

type* unifyArgWithFn (typeSys* ts, const type* arg, const type* fn) {
    assert(fn->kind == type_Forall);
    assert(fn->dt->kind == type_Fn);

    inferences infs = infsInit(malloc);

    /*Return the unified form of the function*/
    bool unifies = typeUnifies(ts, &infs, arg, fn->dt->from);

    type* specificFn = unifies ? typeMakeSubs(ts, &infs, fn) : 0;

    infsFree(&infs);

    return specificFn;
}
