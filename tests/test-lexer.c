#include "test.h"

#include <vector.h>
#include "src/lexer.h"

typedef struct lexer_test {
    vector(const char*) tokens;
    const char* str;
} lexer_test;

void test_lexer (lexer_test test) {
    lexerCtx lexer = lexerInit(test.str);

    /*Match up all the tokens*/
    for_vector (const char* expected, test.tokens, {
        token t = lexerNext(&lexer);
        expect_str_equal(expected, t.buffer);
    })

    /*Confirm EOF*/
    token t = lexerNext(&lexer);
    expect_equal(tokenEOF, t.kind);
    expect(lexerEOF(&lexer));

    lexerDestroy(&lexer);
}

void all_tests (void) {
    lexer_test tests[] = {
        {vectorInitMarkedChain(malloc, "(", "x", "f", ",", "x", "|", "y", "f", ",", "x", "(", "y", "f", ")", ")", VTERM),
                                       "(x f, x | y f, x (y f))"},

        {vectorInitMarkedChain(malloc, "[", "*.[ch]", "]", VTERM),
                                       "[*.[ch]]"},

        {vectorInitMarkedChain(malloc, "{", "*.{cpp,h}", "}", VTERM),
                                       "{*.{cpp,h}}"}
    };

    int test_no = sizeof(tests) / sizeof(*tests);

    for (int i = 0; i < test_no; i++) {
        test_lexer(tests[i]);
        vectorFree(&tests[i].tokens);
    }

    //todo keywords, operators, token kinds, string literals, int literals
}

TEST_GLOBAL_SETUP(all_tests)
