#pragma once

#include <string>
#include <cstdio>

#include "common.hpp"

#include "ast.hpp"
#include "lexer.hpp"

class ParserBase {
protected:
    Lexer* lexer;
    Token current;

    int errors = 0;

public:
    ParserBase (Lexer* nlexer) : lexer(nlexer), current(nlexer->next()) {}

protected:
    void error (string msg) {
        printf("error: %s\n", msg.c_str());
        errors++;
    }

    bool see (string look) {
        return look == current.buffer;
    }

    bool see_kind (Token::Kind look) {
        return look == current.kind;
    }

    bool waiting_for (string look) {
        return !see(look) && current.kind != Token::eof;
    }

    void accept () {
        print(current.buffer);
        current = lexer->next();
    }

    void expected (string expected) {
        error("Expected " + expected + ", found '" + current.buffer + "'");
        accept();
    }

    void match (string look) {
        if (!see(look))
            expected(look);

        else
            accept();
    }

    bool try_match (string look) {
        if (see(look)) {
            accept();
            return true;

        } else
            return false;
    }
};
