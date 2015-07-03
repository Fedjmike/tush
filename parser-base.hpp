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
    void error () {
        printf("ERROR\n");
        errors++;
    }

    bool see (string look) {
        return look == current.buffer;
    }

    bool waiting_for (string look) {
        return !see(look) && current.kind != Token::eof;
    }

    void accept () {
        print(current.buffer);
        current = lexer->next();
    }

    void expect (string look) {
        if (!see(look))
            error();

        accept();
    }

    bool match_if (string look) {
        if (see(look)) {
            accept();
            return true;

        } else
            return false;
    }
};
