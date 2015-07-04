#pragma once

#include <locale>
#include <istream>

#include <sstream>
#include <fstream>

#include "token.hpp"

class Lexer {
private:
    using input_t = std::istream;
    /*TODO: unicode*/
    using char_t = char;

    input_t& input;
    std::locale loc;

public:
    Lexer (input_t& input): input(input) {};

    Token next ();
};

/*==== Inline implementations ====*/

inline Token Lexer::next () {
    string buffer;
    char_t current = input.peek();

    auto skip = [&]() {
        input.get();
        current = input.peek();
    };

    auto eat = [&]() {
        buffer += current;
        skip();
    };

    /*Skip whitespace*/
    while (std::isspace(current, loc))
        skip();

    if (input.eof())
        return Token::make_eof();

    auto kind = Token::normal;

    switch (current) {
    /*String or character literal*/
    case '"': case '\'': {
        kind = current == '"' ? Token::lit_str : Token::lit_char;

        char_t quote = current;
        eat();

        while (current != quote) {
            if (current == '\\')
                eat();

            eat();
        }

        eat();
        break;
    }

    /*Delimiter op*/
    case '(': case ')':
    case '[': case ']':
    case '{': case '}':
    case ',': case '`':
        kind = Token::op;
        eat();

    break;
    /*"Word"*/
    default:
        bool exit = false;

        do {
            eat();

            switch (current) {
            case '"': case '\'':
            case '(': case ')':
            case '[': case ']':
            case '{': case '}':
            case ',': case '`':
            case '\n': case '\r':
            case '\t': case ' ':
                exit = true;
            }
        } while (!exit && !input.eof());
    }

    return Token(kind, buffer);
};
