#include "value.h"

#include <stdio.h>
#include <gc.h>
#include <common.h>

#include "sym.h"
#include "runner.h"

typedef enum valueKind {
    valueInvalid, valueUnit, valueInt, valueFloat, valueStr, valueFile,
    valueFn, valueSimpleClosure, valueASTClosure,
    valuePair, valueTriple, valueVector,
} valueKind;

typedef struct value {
    valueKind kind;

    union {
        /*Int*/
        int64_t integer;
        /*Float*/
        double number;
        /*Str*/
        struct {
            const char* str;
            size_t strlen;
        };
        /*File*/
        struct {
            const char* filename;
            const char* relativeTo;
            /*The absolute form of the filename. May not have been
              computed yet and therefore null.*/
            const char* absolute;
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
        /*Vector*/
        vector(value*) vec; //todo array(value*)
        /*Pair Triple*/
        struct {
            value *first, *second, *third;
        };
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

value* valueCreateFloat (double number) {
    return valueCreate(valueFloat, (value) {
        .number = number
    });
}

value* valueCreateStr (char* str) {
    size_t length = strlen(str);

    return valueCreate(valueStr, (value) {
        .str = strcpy(GC_MALLOC(length+1), str), .strlen = length
    });
}

value* valueCreateFile (const char* filename, const char* relativeTo) {
    return valueCreate(valueFile, (value) {
        .filename = GC_STRDUP(filename),
        .relativeTo = relativeTo,
        .absolute = 0
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
        .argSymbols = alloci(sizeof(vector), &argSymbols, GC_malloc),
        .argValues = alloci(sizeof(vector), &argValues, GC_malloc),
        .body = body
    });
}

value* valueCreatePair (value* first, value* second) {
    return valueCreate(valuePair, (value) {
        .first = first, .second = second
    });
}

value* valueCreateTriple (value* first, value* second, value* third) {
    return valueCreate(valueTriple, (value) {
        .first = first, .second = second, .third = third
    });
}

static value* valueCreateVector (vector(value*) elements) {
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

value* valueStoreTuple (int n, ...) {
    switch (n) {
    case 2:
    case 3: {
        va_list args;
        va_start(args, n);

        value *first = va_arg(args, value*),
              *second = va_arg(args, value*),
              *third = n == 3 ? va_arg(args, value*) : 0;

        va_end(args);

        if (n == 2)
            return valueCreatePair(first, second);

        else
            return valueCreateTriple(first, second, third);
    }

    default: {
        vector(value*) v = vectorInit(n, GC_malloc);

        for_n_args (n, value* element, n, {
            vectorPush(&v, element);
        })

        return valueCreateVector(v);
    }}
}

value* valueStoreArray (int n, value** const array) {
    switch (n) {
    case 2: return valueCreatePair(array[0], array[1]);
    case 3: return valueCreateTriple(array[0], array[1], array[2]);
    default: {
        vector(value*) v = vectorInit(n, GC_malloc);
        vectorPushFromArray(&v, (void**) array, n, sizeof(value*));
        return valueCreateVector(v);
    }}
}

value* valueStoreVector (vector(value*) v) {
    return valueCreateVector(v);
}

/*==== ====*/

const char* valueKindGetStr (valueKind kind) {
    switch (kind) {
    case valueUnit: return "Unit";
    case valueInt: return "Int";
    case valueFloat: return "Float";
    case valueStr: return "Str";
    case valueFn: return "Fn";
    case valueSimpleClosure: return "SimpleClosure";
    case valueASTClosure: return "ASTClosure";
    case valueFile: return "File";
    case valuePair: return "Pair";
    case valueTriple: return "Triple";
    case valueVector: return "Vector";
    case valueInvalid: return "<Invalid value>";
    }

    return "<unhandled value kind>";
}

/*==== (Kind generic) Operations ====*/

bool valueIsInvalid (const value* v) {
    return !v || v->kind == valueInvalid;
}

int valuePrintImpl (const value* v, printf_t printf) {
    if (!precond(v))
        return printf("<null>");

    switch (v->kind) {
    case valueUnit:
        return printf("()");

    case valueInt:
        return printf("%ld", v->integer);

    case valueFloat:
        return printf("%f", v->number);

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

    case valuePair:
        return printf("<pair>");

    case valueTriple:
        return printf("<triple>");

    case valueVector:
        return printf("<vector of %d>", v->vec.length);

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

static bool precond_valueKind (const value* v, valueKind kind) {
    return precond(v) && !valueIsInvalid(v) && precond(v->kind == kind);
}

static bool precond_value (const value* v, bool (*predicate)(const value*)) {
    return precond(v) && !valueIsInvalid(v) && precond(predicate(v));
}

int64_t valueGetInt (const value* num) {
    if (!precond_valueKind(num, valueInt))
        /*This interface gives no way to inform of an error. todo?*/
        return 0;

    return num->integer;
}

static const char* valueGetStrImpl (const value* str, size_t* length) {
    if (!precond_valueKind(str, valueStr)) {
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
    if (!precond(fn) || !precond(arg))
        return valueCreateInvalid();

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

static bool isFileish (const value* v) {
    return    v->kind == valueFile
           || v->kind == valueStr;
}

const char* valueGetFilename (const value* v) {
    if (!precond_value(v, isFileish))
        return "";

    if (v->kind == valueFile) {
        /*Non-relative path, return directly*/
        if (!v->relativeTo)
            return v->filename;

        /*Construct the absolute*/
        if (!v->absolute) {
            size_t length = strlen(v->relativeTo) + strlen(v->filename) + 2;
            ((value*) v)->absolute = GC_MALLOC_ATOMIC(length);
            snprintf((char*) v->absolute, length, "%s/%s", v->relativeTo, v->filename);
        }

        return v->absolute;

    } else
        return v->str;
}

const char* valueGetDisplayFilename (const value* v) {
    if (!precond_value(v, isFileish))
        return "";

    if (v->kind == valueFile)
        return v->filename;

    else
        return v->str;
}

/*---- Iterables ----*/

static bool isIterable (const value* iterable) {
    switch (iterable->kind) {
    case valuePair:
    case valueTriple:
    case valueVector:
        return true;
    default:
        return false;
    }
}

int valueGuessIterableLength (const value* iterable) {
    if (!precond_value(iterable, isIterable)) {
        /*After extensive research, scientists have discovered
          that all iterators are three items long.*/
        return 3;
    }

    switch (iterable->kind) {
    case valuePair: return 2;
    case valueTriple: return 3;
    case valueVector:
        return iterable->vec.length;

    default:
        errprintf("Unhandled iterable kind, %s\n", valueKindGetStr(iterable->kind));
        return 3;
    }
}

bool valueGetIterator (const value* iterable, valueIter* iter) {
    if (!precond_value(iterable, isIterable)) {
        *iter = (valueIter) {.kind = iterInvalid};
        return true;
    }

    switch (iterable->kind) {
    case valuePair:
    case valueTriple:
    case valueVector: {
        *iter = (valueIter) {
            .iterable = iterable, .index = -1
        };

        iter->kind =   iterable->kind == valuePair ? iterPair
                     : iterable->kind == valueTriple ? iterTriple : iterVector;

        return false;
    }

    default:
        errprintf("Unhandled iterable kind, %s\n", valueKindGetStr(iterable->kind));
        return true;
    }
}

const value* valueIterRead (valueIter* iterator) {
    if (   !precond(iterator)
        || iterator->kind == iterInvalid)
        return 0;

    return valueGetTupleNth(iterator->iterable, ++iterator->index);
}

vector(const value*) valueGetVector (const value* iterable) {
    if (   !precond_value(iterable, isIterable)
        || !precond(iterable->kind == valueVector))
        /*Dummy vector*/
        return vectorInit(1, GC_malloc);

    return iterable->vec;
}

/*---- ----*/

const value* valueGetTupleNth (const value* tuple, int n) {
    if (!precond_value(tuple, isIterable))
        return valueCreateInvalid();

    switch (tuple->kind) {
    case valuePair:
    case valueTriple:
        switch (n) {
        case 0: return tuple->first;
        case 1: return tuple->second;
        /*Will be null if the tuple was a pair, the desired output*/
        case 2: return tuple->third;
        default: return 0;
        }

    case valueVector:
        return vectorGet(tuple->vec, n);

    default:
        errprintf("Unhandled iterable kind, %s\n", valueKindGetStr(tuple->kind));
        return valueCreateInvalid();
    }
}
