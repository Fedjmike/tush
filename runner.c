#include "runner.h"

#include <assert.h>
#include <gc/gc.h>

#include "sym.h"
#include "ast.h"
#include "value.h"

/*---- Binary operators ----*/

static value* runPipe (envCtx* env, const ast* node, value* arg, value* fn) {
    (void) env;

    /*Implicit map*/
    if (node->listApp) {
        valueIter iter;
        /*Fails if the arg isn't an iterable value*/
        assert(valueGetIterator(arg, &iter));

        vector(value*) results = vectorInit(valueGuessIterSize(iter), GC_malloc);

        /*Apply it to each element*/
        for (value* element; (element = valueIterRead(&iter));)
            vectorPush(&results, valueCall(fn, element));

        return valueCreateVector(results);

    } else
        return valueCall(fn, arg);
}

static value* runBOP (envCtx* env, const ast* node) {
    value *left = run(env, node->l),
          *right = run(env, node->r);

    switch (node->op) {
    case opPipe: return runPipe(env, node, left, right); break;

    default:
        errprintf("Unhandled binary operator kind, %s", opKindGetStr(node->op));
        return 0;
    }
}

/*---- End of binary operators ----*/

static value* runFnApp (envCtx* env, const ast* node) {
    value* result = run(env, node->r);

    for (int i = 0; i < node->children.length; i++) {
        ast* argNode = vectorGet(node->children, i);
        value* arg = run(env, argNode);

        result = valueCall(result, arg);
    }

    return result;
}

static value* runSymbol (envCtx* env, const ast* node) {
    (void) env;
    value* val = node->symbol->val;
    return val ? val : valueCreateInvalid();
}

static value* runStrLit (envCtx* env, const ast* node) {
    (void) env;

    return valueCreateFile(node->literal.str);
}

static value* runFileLit (envCtx* env, const ast* node) {
    (void) env;

    return valueCreateFile(node->literal.str);
}

static value* runListLit (envCtx* env, const ast* node) {
    vector(value*) result = vectorInit(node->children.length, GC_malloc);

    for (int i = 0; i < node->children.length; i++) {
        ast* element = vectorGet(node->children, i);
        vectorPush(&result, run(env, element));
    }

    return valueCreateVector(result);
}

value* run (envCtx* env, const ast* node) {
    typedef value* (*handler_t)(envCtx*, const ast*);

    static handler_t table[astKindNo] = {
        [astBOP] = runBOP,
        [astFnApp] = runFnApp,
        [astSymbol] = runSymbol,
        [astStrLit] = runStrLit,
        [astFileLit] = runFileLit,
        [astListLit] = runListLit
    };

    handler_t handler = table[node->kind];

    if (handler)
        return handler(env, node);

    else {
        errprintf("Unhandled AST kind, %s\n", astKindGetStr(node->kind));
        return valueCreateInvalid();
    }
}
