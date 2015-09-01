#include "runner.h"

#include <gc/gc.h>

#include "sym.h"
#include "ast.h"
#include "type.h"
#include "value.h"

#include "invoke.h"
#include "builtins.h"

static value* getSymbolValue (envCtx* env, sym* symbol) {
    /*Look it up in the symbol environment
      if running with one (because we're in a lambda)*/
    if (!vectorNull(env->symbols)) {
        int index = vectorFind(env->symbols, symbol);

        if (!precond(index != -1))
            return 0;

        value* result = vectorGet(env->values, index);

        precond(result);
        return result;

    /*Otherwise we can just access the global value*/
    } else {
        value* val = symbol->val;

        if (!val) {
            errprintf("%s has no value\n", symGetName(symbol));
            return valueCreateInvalid();
        }

        return val;
    }

}

static value* runFnLit (envCtx* env, const ast* node) {
    /*Actual args = captures + explicit args*/
    int argNo = node->captured->length + node->children.length;

    /*These two vectors form a simple map from symbol to value, hence
      their elements must correspond.*/
    vector(sym*) argSymbols = vectorInit(argNo, GC_malloc);
    vector(value*) argValues = vectorInit(argNo, GC_malloc);

    /*Fill the symbols vector with the captured symbols and arg symbols*/

    vectorPushFromVector(&argSymbols, *node->captured);

    for_vector (ast* arg, node->children, {
        if (!precond(arg->symbol))
            continue;

        vectorPush(&argSymbols, arg->symbol);
    })

    /*Capture the necessary symbols*/

    for_vector (sym* capture, *node->captured, {
        vectorPush(&argValues, getSymbolValue(env, capture));
    })

    /*Duplicate the body*/
    ast* body = astDup(node->r, GC_malloc);

    return valueCreateASTClosure(argSymbols, argValues, body);
}

static value* runTupleLit (envCtx* env, const ast* node) {
    /*Note: VLA*/
    value* results[node->children.length];

    for_vector_indexed (i, ast* element, node->children, {
        results[i] = run(env, element);
    })

    return valueStoreArray(node->children.length, results);
}

static value* runListLit (envCtx* env, const ast* node) {
    /*Note: VLA*/
    value* results[node->children.length];

    for_vector_indexed (i, ast* element, node->children, {
        results[i] = run(env, element);
    })

    /*Use StoreArray, not StoreVector, because a literal is quite likely
      to be small and not need an allocation.*/
    return valueStoreArray(node->children.length, results);
}

static value* runFileLit (envCtx* env, const ast* node) {
    (void) env;

    return valueCreateFile(node->literal.str);
}

static value* runGlobLit (envCtx* env, const ast* node) {
    (void) env;

    return builtinExpandGlob(node->literal.str, valueCreateUnit());
}

static value* runLit (envCtx* env, const ast* node) {
    (void) env;

    switch (node->kind) {
    case astUnitLit: return valueCreateUnit();
    case astIntLit: return valueCreateInt(node->literal.integer);
    case astBoolLit: return valueCreateInt(node->literal.truth);
    case astStrLit: return valueCreateStr(node->literal.str);
    /*This one thrown in too because it's similarly simple*/
    case astInvalid: return valueCreateUnit();

    default:
        errprintf("Unhandled literal kind, %s", astKindGetStr(node->kind));
        return valueCreateInvalid();
    }
}

static value* runSymbol (envCtx* env, const ast* node) {
    return getSymbolValue(env, node->symbol);
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

    if (!precond(programOutput))
        return valueCreateInvalid();

    /*Read the pipe*/
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

static value* pipeCall (const ast* node, const value* fn, const value* arg) {
    value* result = valueCall(fn, arg);

    if (node->op == opPipeZip)
        result = valueStoreTuple(2, result, arg);

    return result;
}

static value* runPipe (envCtx* env, const ast* node, const value* arg, const value* fn) {
    (void) env;

    /*Implicit map*/
    if (node->flags & astListApplication) {
        valueIter iter;
        bool fail = valueGetIterator(arg, &iter);

        if (fail)
            return valueCreateInvalid();

        vector(value*) results = vectorInit(valueGuessIterableLength(arg), GC_malloc);

        /*Apply it to each element*/
        for (const value* element; (element = valueIterRead(&iter));)
            vectorPush(&results, pipeCall(node, fn, element));

        return valueStoreVector(results);

    } else
        return pipeCall(node, fn, arg);
}

static value* runConcat (envCtx* env, const ast* node, const value* left, const value* right) {
    (void) env, (void) node;

    vector(const value*) lvec = valueGetVector(left),
                         rvec = valueGetVector(right);

    //todo opt.
    vector(value*) result = vectorInit(lvec.length + rvec.length, malloc);
    vectorPushFromVector(&result, lvec);
    vectorPushFromVector(&result, rvec);

    return valueStoreVector(result);
}

static value* runBOP (envCtx* env, const ast* node) {
    const value *left = run(env, node->l),
                *right = run(env, node->r);

    switch (node->op) {
    case opPipe:
    case opPipeZip:
        return runPipe(env, node, left, right);

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
        [astFnLit] = runFnLit,
        [astTupleLit] = runTupleLit,
        [astListLit] = runListLit,
        [astFileLit] = runFileLit,
        [astGlobLit] = runGlobLit,
        /*Common handler*/
        [astInvalid] = runLit,
        [astUnitLit] = runLit,
        [astIntLit] = runLit,
        [astBoolLit] = runLit,
        [astStrLit] = runLit,
        /*---*/
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
