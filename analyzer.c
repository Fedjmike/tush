#include "analyzer.h"

#include <assert.h>

#include "type.h"
#include "sym.h"
#include "ast.h"

typedef struct analyzerCtx {
    typeSys* ts;
} analyzerCtx;

/*==== ====*/

static type* analyzer (analyzerCtx* ctx, ast* node);

static void analyzeFnApp (analyzerCtx* ctx, ast* node) {
    type* fn = analyzer(ctx, node->l);
    type* result = fn;

    //todo fix, this is backwards
    //or is it? still broken tho
    for (int i = 0; i < node->children.length; i++) {
        ast* argNode = vectorGet(node->children, i);
        type* arg = analyzer(ctx, argNode);

        //todo type equality

        type *elements;

        if (typeAppliesToFn(ctx->ts, arg, fn, &result))
            ;

        /*If the parameter is a list, attempt to apply the function instead to
          all of th elements individually.*/
        else if (   typeIsList(ctx->ts, arg, &elements)
                 && typeAppliesToFn(ctx->ts, elements, fn, &result)) {
            /*The result of this node is a list of the results of all the calls*/
            result = typeList(ctx->ts, result);
            //todo which arg is the list? lol
            node->listApp = true;

        } else
            assert(false);
            //todo analyzer errors
    }

    node->dt = result;
}

static void analyzeStrLit (analyzerCtx* ctx, ast* node) {
    node->dt = typeFile(ctx->ts);
}

static void analyzeListLit (analyzerCtx* ctx, ast* node) {
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

    node->dt = typeList(ctx->ts, elements);
}

static void analyzeSymbolLit (analyzerCtx* ctx, ast* node) {
    if (node->literal.symbol && node->literal.symbol->dt)
        node->dt = node->literal.symbol->dt;

    else {
        errprintf("Untyped symbol, %p %s\n", node->literal.symbol,
                  node->literal.symbol ? node->literal.symbol->name : "");
        node->dt = typeInvalid(ctx->ts);
    }
}

static type* analyzer (analyzerCtx* ctx, ast* node) {
    typedef void (*handler_t)(analyzerCtx*, ast*);

    static handler_t table[astKindNo] = {
        [astFnApp] = analyzeFnApp,
        [astStrLit] = analyzeStrLit,
        [astListLit] = analyzeListLit,
        [astSymbolLit] = analyzeSymbolLit
    };

    handler_t handler = table[node->kind];

    if (handler)
        handler(ctx, node);

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

void analyze (typeSys* ts, ast* node) {
    analyzerCtx ctx = analyzerInit(ts);
    analyzer(&ctx, node);
    analyzerFree(&ctx);
}
