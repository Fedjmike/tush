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

    handler_t handler = (handler_t[astKindNo]) {
        [astFnApp] = analyzeFnApp,
        [astStrLit] = analyzeStrLit,
        [astSymbolLit] = analyzeSymbolLit
    }[node->kind];

    if (handler)
        handler(ctx, node);

    else {
        errprintf("Unhandled AST kind, %d\n", astKindGetStr(node->kind));
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
