#include "runner.h"

#include <assert.h>
#include <gc/gc.h>

#include "sym.h"
#include "ast.h"
#include "value.h"

#include "builtins.h"

static value* runInvalid (envCtx* env, const ast* node) {
    (void) env, (void) node;
    return valueCreateInvalid();
}

static value* runUnitLit (envCtx* env, const ast* node) {
    (void) env, (void) node;
    return valueCreateUnit();
}

static value* runIntLit (envCtx* env, const ast* node) {
    (void) env;

    return valueCreateInt(node->literal.integer);
}

static value* runStrLit (envCtx* env, const ast* node) {
    (void) env;

    return valueCreateStr(node->literal.str);
}

static value* runFileLit (envCtx* env, const ast* node) {
    (void) env;

    return valueCreateFile(node->literal.str);
}

static value* runGlobLit (envCtx* env, const ast* node) {
    (void) env;

    return builtinExpandGlob(node->literal.str, valueCreateUnit());
}

static value* runListLit (envCtx* env, const ast* node) {
    vector(value*) result = vectorInit(node->children.length, GC_malloc);

    for_vector (ast* element, node->children, {
        vectorPush(&result, run(env, element));
    })

    return valueCreateVector(result);
}

static value* runTupleLit (envCtx* env, const ast* node) {
    vector(value*) result = vectorInit(node->children.length, GC_malloc);

    for_vector (ast* element, node->children, {
        vectorPush(&result, run(env, element));
    })

    return valueCreateVector(result);
}

static value* runSymbol (envCtx* env, const ast* node) {
    (void) env;
    value* val = node->symbol->val;
    return val ? val : valueCreateInvalid();
}

static value* runFnApp (envCtx* env, const ast* node) {
    value* result = run(env, node->r);

    for_vector (ast* argNode, node->children, {
        value* arg = run(env, argNode);
        result = valueCall(result, arg);
    })

    return result;
}

/*---- Binary operators ----*/

static value* runPipe (envCtx* env, const ast* node, const value* arg, const value* fn) {
    (void) env;

    /*Implicit map*/
    if (node->listApp) {
        valueIter iter;
        /*Fails if the arg isn't an iterable value*/
        assert(valueGetIterator(arg, &iter));

        vector(value*) results = vectorInit(valueGuessIterLength(iter), GC_malloc);

        /*Apply it to each element*/
        for (value* element; (element = valueIterRead(&iter));)
            vectorPush(&results, valueCall(fn, element));

        return valueCreateVector(results);

    } else
        return valueCall(fn, arg);
}

static value* runConcat (envCtx* env, const ast* node, const value* left, const value* right) {
    (void) env, (void) node;

    vector(const value*) lvec = valueGetVector(left),
                         rvec = valueGetVector(right);

    //todo opt.
    vector(value*) result = vectorInit(lvec.length + rvec.length, malloc);
    vectorPushFromVector(&result, lvec);
    vectorPushFromVector(&result, rvec);

    return valueCreateVector(result);
}

static value* runBOP (envCtx* env, const ast* node) {
    const value *left = run(env, node->l),
                *right = run(env, node->r);

    switch (node->op) {
    case opPipe: return runPipe(env, node, left, right);
    case opConcat: return runConcat(env, node, left, right);

    default:
        errprintf("Unhandled binary operator kind, %s\n", opKindGetStr(node->op));
        return valueCreateInvalid();
    }
}

/*---- End of binary operators ----*/

value* run (envCtx* env, const ast* node) {
    typedef value* (*handler_t)(envCtx*, const ast*);

    static handler_t table[astKindNo] = {
        [astInvalid] = runInvalid,
        [astBOP] = runBOP,
        [astFnApp] = runFnApp,
        [astSymbol] = runSymbol,
        [astUnitLit] = runUnitLit,
        [astIntLit] = runIntLit,
        [astStrLit] = runStrLit,
        [astFileLit] = runFileLit,
        [astGlobLit] = runGlobLit,
        [astListLit] = runListLit,
        [astTupleLit] = runTupleLit
    };

    handler_t handler;

    //todo check table for zeroes

    if (!node) {
        errprintf("The given AST node is a null pointer\n");
        return valueCreateInvalid();

    } else if ((handler = table[node->kind]))
        return handler(env, node);

    else {
        errprintf("Unhandled AST kind, %s\n", astKindGetStr(node->kind));
        return valueCreateInvalid();
    }
}
