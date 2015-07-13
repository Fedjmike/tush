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

    for (int i = 0; i < node->children.length; i++) {
        ast* argNode = vectorGet(node->children, i);
        type* arg = analyzer(ctx, argNode);

        //todo analyzer errors
        //todo type equality

        type* result;
        assert(typeAppliesToFn(ctx->ts, arg, fn, &result));
        fn = result;
    }

    node->dt = fn;
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
