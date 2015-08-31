#include "value.h"

#include <stdio.h>
#include <gc/gc.h>
#include <common.h>

#include "sym.h"
#include "runner.h"

typedef enum valueKind {
    valueInvalid, valueUnit, valueInt, valueStr,
    valueFn, valueSimpleClosure, valueASTClosure,
    valueFile, valueVector
} valueKind;

typedef struct value {
    valueKind kind;

    union {
        /*Int*/
        int64_t integer;
        /*Str*/
        struct {
            char* str;
            size_t strlen;
        };
        /*Fn*/
        value* (*fnptr)(const value*);
        /*SimpleClosure*/
        struct {
            value* (*simpleClosure)(const void* env, const value*);
            const void* simpleEnv;
        };
        /*ASTClosure*/
        struct {
            const vector(sym*)* argSymbols;
            const vector(value*)* argValues;
            ast* body;
        };
        /*File*/
        char* filename;
        /*Vector*/
        vector(value*) vec; //todo array(value*)
    };
} value;

static const char* valueKindGetStr (valueKind kind);

/*==== Value creators ====*/

static value* valueCreate (valueKind kind, value init) {
    value* v = GC_MALLOC(sizeof(value));
    *v = init;
    v->kind = kind;
    return v;
}

value* valueCreateUnit (void) {
    return valueCreate(valueUnit, (value) {});
}

value* valueCreateInt (int integer) {
    return valueCreate(valueInt, (value) {
        .integer = integer
    });
}

value* valueCreateStr (char* str) {
    size_t length = strlen(str);

    return valueCreate(valueStr, (value) {
        .str = strcpy(GC_MALLOC(length+1), str), .strlen = length
    });
}

value* valueCreateFn (value* (*fnptr)(const value*)) {
    return valueCreate(valueFn, (value) {
        .fnptr = fnptr
    });
}

value* valueCreateSimpleClosure (const void* env, simpleClosureFn fnptr) {
    return valueCreate(valueSimpleClosure, (value) {
        .simpleClosure = fnptr, .simpleEnv = env
    });
}

value* valueCreateASTClosure (vector(sym*) argSymbols, vector(value*) argValues, ast* body) {
    return valueCreate(valueASTClosure, (value) {
        .argSymbols = malloci(sizeof(vector), &argSymbols),
        .argValues = malloci(sizeof(vector), &argValues),
        .body = body
    });
}

value* valueCreateFile (const char* filename) {
    return valueCreate(valueFile, (value) {
        .filename = GC_STRDUP(filename)
    });
}

value* valueCreateVector (vector(value*) elements) {
    return valueCreate(valueVector, (value) {
        .vec = elements
    });
}

value* valueCreateInvalid (void) {
    static value* invalid;

    if (!invalid)
        invalid = valueCreate(valueInvalid, (value) {});

    return invalid;
}

/*==== ====*/

const char* valueKindGetStr (valueKind kind) {
    switch (kind) {
    case valueUnit: return "Unit";
    case valueStr: return "Str";
    case valueFn: return "Fn";
    case valueSimpleClosure: return "SimpleClosure";
    case valueASTClosure: return "ASTClosure";
    case valueFile: return "File";
    case valueInt: return "Int";
    case valueVector: return "Vector";
    case valueInvalid: return "<Invalid value>";
    }

    return "<unhandled value kind>";
}

/*==== (Kind generic) Operations ====*/

bool valueIsInvalid (const value* v) {
    return v->kind == valueInvalid;
}

int valuePrintImpl (const value* v, printf_t printf) {
    //todo TYPE
    switch (v->kind) {
    case valueUnit:
        return printf("()");

    case valueInt:
        return printf("%ld", v->integer);

    case valueStr:
        //todo escape
        return printf("\"%s\"", v->str);

    case valueFn:
        return printf("<fn at %p>", v->fnptr);

    case valueSimpleClosure:
        return printf("<fn at %p with env. %p>", v->simpleClosure, v->simpleEnv);

    case valueASTClosure:
        return printf("<AST of fn at %p with %p>", v->body, v->argValues);

    case valueFile:
        return printf("%s", v->filename);

    case valueVector: {
        int length = printf("[");

        for_vector_indexed (i, value* element, v->vec, {
            if (i != 0)
                length += printf(", ");

            length += valuePrintImpl(element, printf);
        })

        return length += printf("]");
    }

    case valueInvalid:
        return printf("<invalid>");
    }

    errprintf("Unhandled value kind, %s\n", valueKindGetStr(v->kind));
    return 0;
}

int valueGetWidthOfStr (const value* v) {
    return valuePrintImpl(v, dryprintf);
}

int valuePrint (const value* v) {
    return valuePrintImpl(v, printf);
}

/*==== Kind specific operations ====*/

int64_t valueGetInt (const value* num) {
    if (!precond(num->kind == valueInt))
        /*This interface gives no way to inform of an error. todo?*/
        return 0;

    return num->integer;
}

static const char* valueGetStrImpl (const value* str, size_t* length) {
    if (!precond(str->kind == valueStr)) {
        if (length)
            *length = 0;

        return "";
    }

    if (length)
        *length = str->strlen;

    return str->str;
}

const char* valueGetStr (const value* str) {
    return valueGetStrImpl(str, 0);
}

const char* valueGetStrWithLength (const value* str, size_t* length) {
    return valueGetStrImpl(str, length);
}

value* valueCall (const value* fn, const value* arg) {
    switch (fn->kind) {
    case valueFn:
        return fn->fnptr(arg);

    case valueSimpleClosure:
        return fn->simpleClosure(fn->simpleEnv, arg);

    case valueASTClosure: {
        /*Create a copy of the values vector with the new arg*/
        vector(value*) argValues = vectorInit(fn->argSymbols->length, GC_malloc);
        vectorPushFromVector(&argValues, *fn->argValues);
        vectorPush(&argValues, arg);

        /*More args to come, store in another closure*/
        if (argValues.length < fn->argSymbols->length)
            return valueCreateASTClosure(*fn->argSymbols, argValues, fn->body);

        /*Enough, run the body with this environment*/
        else {
            if (argValues.length > fn->argSymbols->length)
                errprintf("ASTClosure given too many args\n");

            envCtx env = {
                .symbols = *fn->argSymbols,
                .values = argValues
            };

            return run(&env, fn->body);
        }

    } default:
        errprintf("Unhandled value kind, %s\n", valueKindGetStr(fn->kind));
        return valueCreateInvalid();
    }
}

const char* valueGetFilename (const value* value) {
    if (value->kind == valueInvalid)
        return 0;

    if (!precond(   value->kind == valueFile
                 || value->kind == valueStr))
        return 0;

    if (value->kind == valueFile)
        return value->filename;

    else
        return value->str;
}

/*---- Iterables ----*/

static bool isIterable (const value* iterable) {
    return iterable->kind == valueVector;
}

bool valueGetIterator (const value* iterable, valueIter* iter) {
    if (!precond(isIterable(iterable)))
        return true;

    else if (iterable->kind == valueVector) {
        *iter = (valueIter) {
            .iterable = iterable, .n = 0
        };
        return false;

    } else {
        errprintf("Unhandled iterable kind, %s\n", valueKindGetStr(iterable->kind));
        return true;
    }
}

int valueGuessIterLength (valueIter iterator) {
    if (!precond(iterator.iterable->kind == valueVector))
        /*After extensive research, scientists have discovered
          that iterators are all three items long.*/
        return 3;

    return iterator.iterable->vec.length;
}

const value* valueIterRead (valueIter* iterator) {
    if (!precond(iterator->iterable->kind == valueVector))
        return 0;

    return vectorGet(iterator->iterable->vec, iterator->n++);
}

vector(const value*) valueGetVector (const value* iterable) {
    if (   !precond(isIterable(iterable))
        || !precond(iterable->kind == valueVector))
        /*Dummy vector*/
        return vectorInit(1, GC_malloc);

    return iterable->vec;
}

/*---- ----*/

const value* valueGetTupleNth (const value* tuple, int n) {
    if (!precond(tuple->kind == valueVector))
        return valueCreateInvalid();

    return vectorGet(tuple->vec, n);
}
