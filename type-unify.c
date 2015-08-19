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
    //todo bound: intset or vector?
    vector(const type* typevar) bound;
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

inferences infsInit (vector(const type*) bound, stdalloc allocator) {
    return (inferences) {
        .bound = bound,
        .v = vectorInit(8, allocator)
    };
}

inferences* infsFree (inferences* infs) {
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
    /*Only bound typevars may be assigned to*/
    bool lIsBoundTypevar =    l->kind == type_Var
                           && vectorFind(infs->bound, (void*) l) != -1,
         rIsBoundTypevar =    r->kind == type_Var
                           && vectorFind(infs->bound, (void*) r) != -1;

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

    } else {
        /*Switch the bound typevar (if any) into the left slot.*/
        if (rIsBoundTypevar) {
            swap(l, r);
            swap(lIsBoundTypevar, rIsBoundTypevar);
        }

        if (lIsBoundTypevar) {
            inference* inf = infsLookup(infs, l);

            if (inf)
                return infAddClosed(inf, r); //p. conflict

            else
                infsAdd(infs, l, r);

        /*Two closed types, conflict
          (or free typevars, which for this purpose are closed)*/
        } else
            return true;
    }

    return false;
}


bool typeUnifies (typeSys* ts, inferences* infs, const type* l, const type* r) {
    if (l->kind == type_Var || r->kind == type_Var) {
        bool fail = inferEqual(infs, l, r);
        return !fail;

    } else if (l->kind == type_Forall) {
        /*The typevars of this quantifier cannot be assigned to*/
        return typeUnifies(ts, infs, l->dt, r);

    } else if (r->kind == type_Forall) {
        return typeUnifies(ts, infs, l, r->dt);

    } else if (l->kind != r->kind) {
        return false;

    /*Kinds equal*/
    } else {
        if (!typeKindIsntUnitary(l->kind))
            return l == r ? (type*) l : 0;

        switch (l->kind) {
        case type_Fn:
            return    typeUnifies(ts, infs, l->from, r->from)
                   && typeUnifies(ts, infs, l->to, r->to);

        case type_List:
            return typeUnifies(ts, infs, l->elements, r->elements);

        case type_Tuple:
            if (l->types.length != r->types.length)
                return false;

            for (int i = 0; i < l->types.length; i++) {
                type *ldt = vectorGet(l->types, i),
                     *rdt = vectorGet(r->types, i);

                if (!typeUnifies(ts, infs, ldt, rdt))
                    return false;
            }

            return true;

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
        //todo leave typevars with no closed
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

    /*Only the typevars bound to the two arguments can be assigned to
      in the unification process.*/

    vector(type*) boundTypevars = fn->typevars;

    if (arg->kind == type_Forall) {
        boundTypevars = vectorInit(boundTypevars.length + arg->typevars.length, malloc);
        vectorPushFromVector(&boundTypevars, fn->typevars);
        vectorPushFromVector(&boundTypevars, arg->typevars);
    }

    inferences infs = infsInit(boundTypevars, malloc);

    /*Return the unified form of the function*/
    bool unifies = typeUnifies(ts, &infs, arg, fn->dt->from);
    type* specificFn = unifies ? typeMakeSubs(ts, &infs, fn) : 0;

    infsFree(&infs);

    if (arg->kind == type_Forall)
        vectorFree(&boundTypevars);

    return specificFn;
}
