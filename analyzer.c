#include "analyzer.h"

#include <assert.h>

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

/*---- Binary operators ----*/

static type* analyzePipe (analyzerCtx* ctx, ast* node, type* arg, type* fn) {
    type* result;

    if (typeAppliesToFn(arg, fn))
        result = typeGetFnResult(fn);

    /*If the parameter is a list, attempt to apply the function instead to
      all of the elements individually.*/
    else if (   typeIsList(arg)
             && typeAppliesToFn(typeGetListElements(arg), fn)) {
        /*The result is a list of the results of all the calls*/
        result = typeList(ctx->ts, typeGetFnResult(fn));
        node->listApp = true;

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

    if (!typeIsList(left))
        errorConcatIsntList(ctx, true, left);

    if (!typeIsList(right))
        errorConcatIsntList(ctx, false, right);

    if (typeIsEqual(left, right))
        return left; //arbitrary

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

/*---- End of binary operators ----*/

static type* analyzeFnApp (analyzerCtx* ctx, ast* node) {
    type* result = analyzer(ctx, node->r);

    for (int i = 0; i < node->children.length; i++) {
        ast* argNode = vectorGet(node->children, i);
        type* arg = analyzer(ctx, argNode);

        if (typeAppliesToFn(arg, result))
            result = typeGetFnResult(result);

        else {
            errorFnApp(ctx, arg, result);
            result = typeInvalid(ctx->ts);
        }
    }

    return result;
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

static type* analyzeUnitLit (analyzerCtx* ctx, ast* node) {
    (void) node;
    return typeUnitary(ctx->ts, type_Unit);
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

static type* analyzeListLit (analyzerCtx* ctx, ast* node) {
    //todo 'a List
    assert(node->children.length != 0);

    type* elements;

    for (int i = 0; i < node->children.length; i++) {
        ast* element = vectorGet(node->children, i);
        elements = analyzer(ctx, element);

        //todo check equality
        //mode average if they differ
        //todo lowest common interface ?
    }

    return typeList(ctx->ts, elements);
}

static type* analyzer (analyzerCtx* ctx, ast* node) {
    typedef type* (*handler_t)(analyzerCtx*, ast*);

    static handler_t table[astKindNo] = {
        [astInvalid] = analyzeInvalid,
        [astBOP] = analyzeBOP,
        [astFnApp] = analyzeFnApp,
        [astSymbol] = analyzeSymbol,
        [astUnitLit] = analyzeUnitLit,
        [astStrLit] = analyzeStrLit,
        [astFileLit] = analyzeFileLit,
        [astGlobLit] = analyzeGlobLit,
        [astListLit] = analyzeListLit
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
