#include "analyzer.h"

#include "common.h"
#include "type.h"
#include "sym.h"
#include "ast.h"

typedef struct analyzerCtx {
    typeSys* ts;
    int errors;
} analyzerCtx;

static type* analyzer (analyzerCtx* ctx, ast* node);

/*==== Internals ====*/

inline static printf_t* error (analyzerCtx* ctx) {
    ctx->errors++;
    printf("error: ");
    return printf;
}

/*==== ====*/

static type* analyzeInvalid (analyzerCtx* ctx, ast* node) {
    (void) node;
    return typeInvalid(ctx->ts);
}

static type* analyzeFnLit (analyzerCtx* ctx, ast* node) {
    /*Type the arg patterns
      (this must occur before typing the body because the args will
       be used in it and thus need types)*/

    vector(type*) typevars = vectorInit(2, malloc);

    for_vector (ast* pattern, node->children, {
        /*Nothing we can say about the arg for now*/
        type* typevar = typeVar(ctx->ts);
        vectorPush(&typevars, typevar);

        pattern->symbol->dt = typevar;
        pattern->dt = pattern->symbol->dt;
    })

    /*Body*/

    type* result = analyzer(ctx, node->r);

    /*Build up the fn type*/

    for_vector_reverse (ast* pattern, node->children, {
        result = typeFn(ctx->ts, pattern->dt, result);
    })

    if (typevars.length != 0)
        return typeForall(ctx->ts, typevars, result);

    else {
        vectorFree(&typevars);
        return result;
    }
}

static type* analyzeTupleLit (analyzerCtx* ctx, ast* node) {
    precond(node->children.length >= 2);

    vector(type*) elements = vectorInit(node->children.length, malloc);

    for_vector (ast* element, node->children, {
        vectorPush(&elements, analyzer(ctx, element));
    })

    return typeTuple(ctx->ts, elements);
}

static type* analyzeListLit (analyzerCtx* ctx, ast* node) {
    /*No elements => type is ['a] */
    if (node->children.length == 0) {
        type* A = typeVar(ctx->ts);
        vector(type*) typevars = vectorInitChain(1, malloc, A);

        return typeForall(ctx->ts, typevars, typeList(ctx->ts, A));

    } else {
        type* elements;

        for_vector (ast* element, node->children, {
            elements = analyzer(ctx, element);

            //todo check equality
            //mode average if they differ
            //todo lowest common interface ?
        })

        return typeList(ctx->ts, elements);
    }
}

static type* analyzeUnitLit (analyzerCtx* ctx, ast* node) {
    (void) node;
    return typeUnitary(ctx->ts, type_Unit);
}

static type* analyzeIntLit (analyzerCtx* ctx, ast* node) {
    (void) node;
    return typeUnitary(ctx->ts, type_Int);
}

static type* analyzeBoolLit (analyzerCtx* ctx, ast* node) {
    (void) node;
    return typeUnitary(ctx->ts, type_Bool);
}

static type* analyzeStrLit (analyzerCtx* ctx, ast* node) {
    (void) node;
    return typeUnitary(ctx->ts, type_Str);
}

static type* analyzeFileLit (analyzerCtx* ctx, ast* node) {
    (void) node;
    return typeFile(ctx->ts);
}

static type* analyzeGlobLit (analyzerCtx* ctx, ast* node) {
    (void) node;
    return typeList(ctx->ts, typeFile(ctx->ts));
}

static type* analyzeSymbol (analyzerCtx* ctx, ast* node) {
    if (node->symbol && node->symbol->dt)
        return node->symbol->dt;

    else {
        errprintf("Untyped symbol, %p %s\n", node->symbol,
                  node->symbol ? node->symbol->name : "");
        return typeInvalid(ctx->ts);
    }
}

static void errorClassicUnixApp (analyzerCtx* ctx, type* arg) {
    if (typeIsInvalid(arg))
        ;

    else
        error(ctx)("Unix invocation (stdin -> stdout) requires string arguments, given %s\n",
                   typeGetStr(arg));
}

static void errorFnApp (analyzerCtx* ctx, type* arg, type* fn) {
    if (typeIsInvalid(fn))
        ;

    else if (!typeIsFn(fn))
        error(ctx)("type %s is not a function\n", typeGetStr(fn));

    else if (typeIsInvalid(arg))
        ;

    else
        error(ctx)("type %s does not apply to function %s\n", typeGetStr(arg), typeGetStr(fn));
}

static bool isUnixBasicSerializable (type* dt) {
    return    typeIsKind(type_Str, dt)
           || typeIsKind(type_File, dt);
}

static bool isUnixSerializable (type* dt) {
    type* elements;

    if (isUnixBasicSerializable(dt))
        return true;

    else if (   typeIsListOf(dt, &elements)
             && isUnixBasicSerializable(elements))
        return true;

    else
        return false;
}

static type* analyzeFnApp (analyzerCtx* ctx, ast* node) {
    type* result = analyzer(ctx, node->r);

    /*A classic Unix program invocation*/
    if (typeIsKind(type_File, result)) {
        node->flags |= astUnixInvocation;

        for_vector (ast* argNode, node->children, {
            type* arg = analyzer(ctx, argNode);

            if (!isUnixSerializable(arg))
                errorClassicUnixApp(ctx, arg);

            //todo is str
            //or serializable?
        })

        return typeUnitary(ctx->ts, type_Str);

    } else {
        /*Successively apply each argument to the previous result*/
        for_vector (ast* argNode, node->children, {
            type* arg = analyzer(ctx, argNode);

            type* callResult;

            if (typeAppliesToFn(ctx->ts, arg, result, &callResult))
                result = callResult;

            else {
                errorFnApp(ctx, arg, result);
                result = typeInvalid(ctx->ts);
            }
        })
    }

    return result;
}

/*---- Binary operators ----*/

static type* analyzePipe (analyzerCtx* ctx, ast* node, type* arg, type* fn) {
    type *result, *callResult, *elements;

    if (typeAppliesToFn(ctx->ts, arg, fn, &callResult))
        result = callResult;

    /*If the parameter is a list, attempt to apply the function instead to
      all of the elements individually.*/
    else if (   typeIsListOf(arg, &elements)
             && typeAppliesToFn(ctx->ts, elements, fn, &callResult)) {
        /*The result is a list of the results of all the calls*/
        result = typeList(ctx->ts, callResult);
        node->flags |= astListApplication;

    } else {
        errorFnApp(ctx, arg, fn);
        result = typeInvalid(ctx->ts);
    }

    return result;
}

static void errorConcatIsntList (analyzerCtx* ctx, bool left, type* operand) {
    if (typeIsInvalid(operand))
        return;

    error(ctx)("%s operand of concat operator (++), %s, is not a list\n",
               left ? "Left" : "Right",
               typeGetStr(operand));
}

static void errorConcatMismatch (analyzerCtx* ctx, type* left, type* right) {
    if (typeIsInvalid(left) || typeIsInvalid(right))
        return;

    error(ctx)("Operands of concat operator (++), %s and %s, do not match\n",
               typeGetStr(left), typeGetStr(right));
}

static type* analyzeConcat (analyzerCtx* ctx, ast* node, type* left, type* right) {
    (void) node;

    type* result;

    if (!typeIsList(left))
        errorConcatIsntList(ctx, true, left);

    if (!typeIsList(right))
        errorConcatIsntList(ctx, false, right);

    if (typeCanUnify(ctx->ts, left, right, &result))
        return result;

    else {
        errorConcatMismatch(ctx, left, right);
        return typeInvalid(ctx->ts);
    }
}

static type* analyzeBOP (analyzerCtx* ctx, ast* node) {
    type *left = analyzer(ctx, node->l),
         *right = analyzer(ctx, node->r);

    switch (node->op) {
    case opPipe: return analyzePipe(ctx, node, left, right);
    case opConcat: return analyzeConcat(ctx, node, left, right);

    default:
        errprintf("Unhandled binary operator kind, %s\n", opKindGetStr(node->op));
        return typeInvalid(ctx->ts);
    }
}

/*---- ----*/

static type* analyzeLet (analyzerCtx* ctx, ast* node) {
    type* init = analyzer(ctx, node->r);

    node->symbol->dt = init;

    return typeUnitary(ctx->ts, type_Unit);
}

/*---- ----*/

static type* analyzer (analyzerCtx* ctx, ast* node) {
    typedef type* (*handler_t)(analyzerCtx*, ast*);

    static handler_t table[astKindNo] = {
        [astInvalid] = analyzeInvalid,
        [astFnLit] = analyzeFnLit,
        [astTupleLit] = analyzeTupleLit,
        [astListLit] = analyzeListLit,
        [astUnitLit] = analyzeUnitLit,
        [astIntLit] = analyzeIntLit,
        [astBoolLit] = analyzeBoolLit,
        [astStrLit] = analyzeStrLit,
        [astFileLit] = analyzeFileLit,
        [astGlobLit] = analyzeGlobLit,
        [astSymbol] = analyzeSymbol,
        [astFnApp] = analyzeFnApp,
        [astBOP] = analyzeBOP,
        [astLet] = analyzeLet
    };

    if (!node) {
        errprintf("The given AST node is a null pointer\n");
        return typeInvalid(ctx->ts);
    }

    handler_t handler = table[node->kind];

    if (handler)
        node->dt = handler(ctx, node);

    else {
        errprintf("Unhandled AST kind, %s\n", astKindGetStr(node->kind));
        node->dt = typeInvalid(ctx->ts);
    }

    return node->dt;
}

/*==== ====*/

static analyzerCtx analyzerInit (typeSys* ts) {
    return (analyzerCtx) {
        .ts = ts
    };
}

static analyzerCtx* analyzerFree (analyzerCtx* ctx) {
    return ctx;
}

analyzerResult analyze (typeSys* ts, ast* node) {
    analyzerCtx ctx = analyzerInit(ts);
    analyzer(&ctx, node);

    analyzerResult result = {
        .errors = ctx.errors
    };

    analyzerFree(&ctx);

    return result;
}
