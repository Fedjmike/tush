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

static type* analyzeFnLit (analyzerCtx* ctx, ast* node) {
    /*Type the arg patterns
      (this must occur before typing the body because the args will
       be used in it and thus need types)*/

    for_vector (ast* pattern, node->children, {
        type* dt =   pattern->kind == astTypeHint
                   ? pattern->dt
                   : typeVar(ctx->ts);

        pattern->symbol->dt = dt;
        pattern->dt = pattern->symbol->dt;
    })

    /*Body*/

    type* result = analyzer(ctx, node->r);

    /*Build up the fn type*/

    for_vector_reverse (ast* pattern, node->children, {
        result = typeFn(ctx->ts, pattern->dt, result);

        if (typeIsKind(type_Var, pattern->dt))
            result = typeForall(ctx->ts, pattern->dt, result);
    })

    return result;
}

static type* analyzeTupleLit (analyzerCtx* ctx, ast* node) {
    precond(node->children.length >= 2);

    vector(type*) elements = vectorInit(node->children.length, malloc);

    for_vector (ast* element, node->children, {
        vectorPush(&elements, analyzer(ctx, element));
    })

    return typeTuple(ctx->ts, elements);
}

static void errorListLitMismatch (analyzerCtx* ctx, type* elements, type* dt) {
    if (typeIsInvalid(elements) || typeIsInvalid(dt))
        return;

    error(ctx)("list element mismatch: given %s while others are %s\n",
               typeGetStr(dt), typeGetStr(elements));
}

static type* analyzeListLit (analyzerCtx* ctx, ast* node) {
    /*No elements => type is ['a] */
    if (node->children.length == 0) {
        type* A = typeVar(ctx->ts);
        return typeForall(ctx->ts, A, typeList(ctx->ts, A));

    } else {
        type* elements = 0;

        for_vector_indexed (i, ast* element, node->children, {
            type* dt = analyzer(ctx, element);

            if (!elements)
                elements = dt;

            else {
                type* unified;
                typeCanUnify(ctx->ts, elements, dt, &unified);

                if (unified)
                    elements = unified;

                else
                    errorListLitMismatch(ctx, elements, dt);
            }

            //todo mode average if they differ
        })

        return typeList(ctx->ts, elements);
    }
}

static type* analyzeLit (analyzerCtx* ctx, ast* node) {
    switch (node->kind) {
    case astUnitLit: return typeUnitary(ctx->ts, type_Unit);
    case astIntLit: return typeUnitary(ctx->ts, type_Int);
    case astFloatLit: return typeUnitary(ctx->ts, type_Float);
    case astBoolLit: return typeUnitary(ctx->ts, type_Bool);
    case astStrLit: return typeUnitary(ctx->ts, type_Str);
    case astFileLit: return typeUnitary(ctx->ts, type_File);
    /*This one thrown in too because it's similarly simple*/
    case astInvalid: return typeInvalid(ctx->ts);

    default:
        errprintf("Unhandled literal kind, %s", astKindGetStr(node->kind));
        return typeInvalid(ctx->ts);
    }
}

static type* analyzeGlobLit (analyzerCtx* ctx, ast* node) {
    (void) node;
    return typeList(ctx->ts, typeUnitary(ctx->ts, type_File));
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
        return;

    error(ctx)("%s given to string stream program\n", typeGetStr(arg));
}

static void errorFnApp (analyzerCtx* ctx, type* arg, type* fn) {
    if (typeIsInvalid(fn))
        ;

    else if (!typeIsFn(fn))
        error(ctx)("%s is not a function\n", typeGetStr(fn));

    else if (!typeIsInvalid(arg))
        error(ctx)("%s given to function %s\n", typeGetStr(arg), typeGetStr(fn));
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
        node->flags |= flagUnixInvocation;

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
    /*Work out what the result of the call is*/
    type* callResult; {
        type *elements;

        if (typeAppliesToFn(ctx->ts, arg, fn, &callResult)) {
            ;

        /*If the parameter is a list, instead try to apply the function to
          each of the elements individually.*/
        } else if (   typeIsListOf(arg, &elements)
                 && typeAppliesToFn(ctx->ts, elements, fn, &callResult)) {
            arg = elements;
            node->flags |= flagListApplication;

        } else {
            errorFnApp(ctx, arg, fn);
            callResult = typeInvalid(ctx->ts);
        }
    }

    if (node->op == opPipeZip)
        /*Zip up the arg with the result of (each/the) call*/
        callResult = typeTuple(ctx->ts, vectorInitChain(2, malloc, callResult, arg));

    if (node->flags & flagListApplication)
        return typeList(ctx->ts, callResult);

    else
        return callResult;
}

static void errorConcat (analyzerCtx* ctx, type* left, type* right) {
    if (typeIsInvalid(left) || typeIsInvalid(right))
        return;

    bool leftNotList = !typeIsList(left),
         rightNotList = !typeIsList(right);

    if (leftNotList || rightNotList) {
        if (leftNotList && rightNotList) {
            error(ctx)(  typeIsEqual(left, right)
                       ? "concat operator (++): operands, %s, are not lists\n"
                       : "concat operator (++): operands, %s and %s, are not lists\n",
                       typeGetStr(left), typeGetStr(right));

        } else
            error(ctx)("concat operator (++): %s operand, %s, is not a list\n",
                       leftNotList ? "left" : "right",
                       typeGetStr(leftNotList ? left : right));

    } else
        error(ctx)("concat operator (++): operand mismatch: %s and %s\n",
                   typeGetStr(left), typeGetStr(right));
}

static type* analyzeConcat (analyzerCtx* ctx, ast* node, type* left, type* right) {
    (void) node;

    type* result;
    bool unifies = typeCanUnify(ctx->ts, left, right, &result);

    if (unifies && typeIsList(result))
        return result;

    else {
        errorConcat(ctx, left, right);
        return typeInvalid(ctx->ts);
    }
}

static type* analyzeBOP (analyzerCtx* ctx, ast* node) {
    type *left = analyzer(ctx, node->l),
         *right = analyzer(ctx, node->r);

    switch (node->op) {
    case opPipe:
    case opPipeZip:
        return analyzePipe(ctx, node, left, right);

    case opConcat: return analyzeConcat(ctx, node, left, right);

    default:
        errprintf("Unhandled binary operator kind, %s\n", opKindGetStr(node->op));
        return typeInvalid(ctx->ts);
    }
}

/*---- ----*/

static type* analyzeLet (analyzerCtx* ctx, ast* node) {
    type* init = analyzer(ctx, node->r);

    if (node->symbol)
        node->symbol->dt = init;

    return typeUnitary(ctx->ts, type_Unit);
}

/*---- ----*/

static type* analyzer (analyzerCtx* ctx, ast* node) {
    typedef type* (*handler_t)(analyzerCtx*, ast*);

    static handler_t table[astKindNo] = {
        [astFnLit] = analyzeFnLit,
        [astTupleLit] = analyzeTupleLit,
        [astListLit] = analyzeListLit,
        /*Common handler*/
        [astInvalid] = analyzeLit,
        [astUnitLit] = analyzeLit,
        [astIntLit] = analyzeLit,
        [astFloatLit] = analyzeLit,
        [astBoolLit] = analyzeLit,
        [astStrLit] = analyzeLit,
        [astFileLit] = analyzeLit,
        /*---*/
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

    /*A top level fn app gets to execute synchronously.
      That is, take control of the terminal and return only an error code.*/
    if (node->kind == astFnApp && (node->flags & flagUnixInvocation)) {
        node->flags |= flagUnixSynchronous;
        node->dt = typeUnitary(ts, type_Int);
    }

    analyzerResult result = {
        .errors = ctx.errors
    };

    analyzerFree(&ctx);

    return result;
}
