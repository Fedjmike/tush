#include "test.h"

#include <gc.h>

#include "src/sym.h"

void test_sym (void) {
    /*Symbols interact with the GC because they can own GC objects*/
    GC_INIT();

    sym* scope = symInit();
    require(scope);

    /*Simple scope lookup*/

    sym *sym1 = symAdd(scope, "sym1"),
        *sym2 = symAdd(scope, "sym2");

    expect(symIsInside(sym1, scope));
    expect(symIsInside(sym2, scope));

    expect_equal(sym1, symLookup(scope, "sym1"));
    expect_equal(sym2, symLookup(scope, "sym2"));
    expect_null(symLookup(scope, "non-sym"));

    /*Lookup from an inner scope*/

    sym* innerscope = symAddScope(scope);
    sym* sym3 = symAdd(innerscope, "sym3");

    expect(symIsInside(sym3, scope));
    expect(symIsInside(sym3, innerscope));

    expect_null(symLookup(scope, "sym3"));
    expect_equal(sym1, symLookup(innerscope, "sym1"));
    expect_equal(sym3, symLookup(innerscope, "sym3"));

    /*Shadowed symbols (same scope)*/

    sym* new_sym1 = symAdd(scope, "sym1");

    expect(symIsInside(sym1, scope));
    expect(symIsInside(new_sym1, scope));

    expect_equal(new_sym1, symLookup(scope, "sym1"));
    expect_equal(new_sym1, symLookup(innerscope, "sym1"));

    /*Shadowed symbols (nested scopes)*/

    sym* inner_sym1 = symAdd(innerscope, "sym1");

    expect_equal(inner_sym1, symLookup(innerscope, "sym1"));
    expect_equal(new_sym1, symLookup(scope, "sym1"));

    /*symGetName*/

    expect_str_equal("sym1", symGetName(symLookup(scope, "sym1")));

    /*Teardown*/

    symEnd(scope);
}

TEST_GLOBAL_SETUP(test_sym)
