#pragma once

#include "common.hpp"

struct Token {
    enum Kind {normal, op, lit_str, lit_char, eof} kind;
    string buffer;

    Token (string buffer): kind(Token::normal), buffer(buffer) {}
    Token (Kind kind, string buffer): kind(kind), buffer(buffer) {}

    static Token make_eof () {
        return Token(Token::eof, "");
    }
};
