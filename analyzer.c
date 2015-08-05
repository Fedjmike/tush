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

/*---- Binary operators ----*/

static void errorFnApp (analyzerCtx* ctx, type* arg, type* fn) {
    error(ctx)("type %s does not apply to function %s\n", typeGetStr(arg), typeGetStr(fn));
}

static type* analyzePipe (analyzerCtx* ctx, ast* node) {
    type *arg = node->l->dt,
         *fn = node->r->dt;

    type* result; {
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
            if (!typeIsInvalid(arg) && !typeIsInvalid(fn))
                errorFnApp(ctx, arg, fn);

            result = typeInvalid(ctx->ts);
        }
    }

    return result;
}

static type* analyzeBOP (analyzerCtx* ctx, ast* node) {
    analyzer(ctx, node->l);
    analyzer(ctx, node->r);

    switch (node->op) {
    case opPipe: return analyzePipe(ctx, node);

    default:
        errprintf("Unhandled binary operator kind, %s", opKindGetStr(node->op));
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
            if (!typeIsInvalid(arg) && !typeIsInvalid(result))
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

static type* analyzeStrLit (analyzerCtx* ctx, ast* node) {
    (void) node;
    return typeInvalid(ctx->ts);
}

static type* analyzeFileLit (analyzerCtx* ctx, ast* node) {
    (void) node;
    return typeFile(ctx->ts);
}

static type* analyzeGlobLit (analyzerCtx* ctx, ast* node) {
    (void) node;
    return typeFnChain(2, ctx->ts, type_Unit, type_File);
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
        [astStrLit] = analyzeStrLit,
        [astFileLit] = analyzeFileLit,
        [astGlobLit] = analyzeGlobLit,
        [astListLit] = analyzeListLit
    };

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
