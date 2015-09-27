#include <vector.h>

#include "type.h"

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
        /*Forall*/
        struct {
            type* typevar;
            type* dt;
        };
    };

    /*Not used by all types
      Allocated in typeGetStr, if at all*/
    char* str;
} type;

static inline bool typeKindIsntUnitary (typeKind kind) {
    return    kind == type_Fn || kind == type_List || kind == type_Tuple
           || kind == type_Var || kind == type_Forall;
}

/*==== Unification ====*/

type* unifyArgWithFn (typeSys* ts, const type* arg, const type* fn);
type* unifyMatching (typeSys* ts, const type* l, const type* r);
