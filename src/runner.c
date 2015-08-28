#include "runner.h"

#include <assert.h>
#include <gc/gc.h>

#include "sym.h"
#include "ast.h"
#include "type.h"
#include "value.h"

#include "invoke.h"
#include "builtins.h"

static value* runInvalid (envCtx* env, const ast* node) {
    (void) env, (void) node;
    return valueCreateInvalid();
}

static value* runTupleLit (envCtx* env, const ast* node) {
    vector(value*) result = vectorInit(node->children.length, GC_malloc);

    for_vector (ast* element, node->children, {
        vectorPush(&result, run(env, element));
    })

    return valueCreateVector(result);
}

static value* runListLit (envCtx* env, const ast* node) {
    vector(value*) result = vectorInit(node->children.length, GC_malloc);

    for_vector (ast* element, node->children, {
        vectorPush(&result, run(env, element));
    })

    return valueCreateVector(result);
}

static value* runUnitLit (envCtx* env, const ast* node) {
    (void) env, (void) node;
    return valueCreateUnit();
}

static value* runIntLit (envCtx* env, const ast* node) {
    (void) env;

    return valueCreateInt(node->literal.integer);
}

static value* runBoolLit (envCtx* env, const ast* node) {
    (void) env;

    return valueCreateInt(node->literal.truth);
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

static value* runSymbol (envCtx* env, const ast* node) {
    (void) env;
    value* val = node->symbol->val;
    return val ? val : valueCreateInvalid();
}

bool unixSerialize (vector(const char*)* args, value* v, type* dt) {
    const char* str;

    type* elements;

    //todo quoting

    if (typeIsKind(type_File, dt))
        str = valueGetFilename(v);

    else if (typeIsKind(type_Str, dt))
        str = valueGetStr(v);

    /*A list appears as a sequence of args*/
    else if (typeIsListOf(dt, &elements)) {
        vector(value*) vec = valueGetVector(v);

        /*Allocate more space for the elements early since we know how
          many there will be.*/
        vectorResize(args, args->capacity + vec.length);

        const char* (*getter)(const value*) =   typeIsKind(type_Str, elements)
                                              ? valueGetStr : valueGetFilename;

        for_vector (value* element, vec, {
            vectorPush(args, getter(element));
        })

        /*todo: Currently a zero size array serializes to no new args,
                which means the program doesn't notice it. This can cause
                programs to read indefinitely from stdin. Bad! Give empty
                string?
                Try !wc *.nonexistent */

        return false;

    } else {
        errprintf("Unhandled type kind, %s\n", typeGetStr(dt));
        return true;
    }

    /*Add the serialized string
      (for args of a single string, lists return early)*/
    vectorPush(args, str);
    return false;
}
static value* runClassicUnixApp (envCtx* env, const ast* node, const char* program) {
    /*Create a vector of the (string) args,
      bookended by the program name and a null-terminator.*/

    vector(const char*) args = vectorInit(node->children.length + 2, malloc);

    vectorPush(&args, program);

    for_vector (ast* argNode, node->children, {
        value* arg = run(env, argNode);

        /*Structured data must be lowered to strings*/
        bool fail = unixSerialize(&args, arg, argNode->dt);

        if (fail)
            return valueCreateInvalid();
    })

    vectorPush(&args, 0);

    /*Run the program*/
    FILE* programOutput = invokePiped((char**) args.buffer);
    vectorFree(&args);

    /*Read the pipe*/
    assert(programOutput);
    char* output = readall(programOutput, GC_malloc, GC_realloc);

    return valueCreateStr(output);
}

static value* runFnApp (envCtx* env, const ast* node) {
    value* result = run(env, node->r);

    if (node->flags & astUnixInvocation)
        result = runClassicUnixApp(env, node, valueGetFilename(result));

    else {
        for_vector (ast* argNode, node->children, {
            value* arg = run(env, argNode);
            result = valueCall(result, arg);
        })
    }

    return result;
}

/*---- Binary operators ----*/

static value* runPipe (envCtx* env, const ast* node, const value* arg, const value* fn) {
    (void) env;

    /*Implicit map*/
    if (node->flags & astListApplication) {
        valueIter iter;
        /*Fails if the arg isn't an iterable value*/
        assert(valueGetIterator(arg, &iter));

        vector(value*) results = vectorInit(valueGuessIterLength(iter), GC_malloc);

        /*Apply it to each element*/
        for (const value* element; (element = valueIterRead(&iter));)
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

/*---- ----*/

static value* runLet (envCtx* env, const ast* node) {
    const value* init = run(env, node->r);

    node->symbol->val = (value*) init;

    return valueCreateUnit();
}

/*---- ----*/

value* run (envCtx* env, const ast* node) {
    typedef value* (*handler_t)(envCtx*, const ast*);

    static handler_t table[astKindNo] = {
        [astInvalid] = runInvalid,
        [astTupleLit] = runTupleLit,
        [astListLit] = runListLit,
        [astUnitLit] = runUnitLit,
        [astIntLit] = runIntLit,
        [astBoolLit] = runBoolLit,
        [astStrLit] = runStrLit,
        [astFileLit] = runFileLit,
        [astGlobLit] = runGlobLit,
        [astSymbol] = runSymbol,
        [astFnApp] = runFnApp,
        [astBOP] = runBOP,
        [astLet] = runLet
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
