#include "value.h"

#include <stdio.h>
#include <assert.h>
#include <gc/gc.h>
#include <common.h>

typedef enum valueKind {
    valueInvalid, valueUnit, valueInt, valueStr,
    valueFn, valueSimpleClosure, valueFile, valueVector
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
    assert(num->kind == valueInt);
    return num->integer;
}

static const char* valueGetStrImpl (const value* str, size_t* length) {
    assert(str->kind == valueStr);

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

    default:
        errprintf("Unhandled value kind, %s\n", valueKindGetStr(fn->kind));
        return valueCreateInvalid();
    }
}

const char* valueGetFilename (const value* value) {
    if (value->kind == valueInvalid)
        return 0;

    assert(   value->kind == valueFile
           || value->kind == valueStr);

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
    if (!isIterable(iterable))
        return false;

    else if (iterable->kind == valueVector) {
        *iter = (valueIter) {
            .iterable = iterable, .n = 0
        };
        return true;

    } else {
        errprintf("Unhandled iterable kind, %s\n", valueKindGetStr(iterable->kind));
        return false;
    }
}

int valueGuessIterLength (valueIter iterator) {
    assert(iterator.iterable->kind == valueVector);
    return iterator.iterable->vec.length;
}

const value* valueIterRead (valueIter* iterator) {
    assert(iterator->iterable->kind == valueVector);
    return vectorGet(iterator->iterable->vec, iterator->n++);
}

vector(const value*) valueGetVector (const value* iterable) {
    assert(isIterable(iterable));
    return iterable->vec;
}

/*---- ----*/

const value* valueGetTupleNth (const value* tuple, int n) {
    assert(tuple->kind == valueVector);
    return vectorGet(tuple->vec, n);
}
