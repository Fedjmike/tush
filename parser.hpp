#pragma once

#include "parser-base.hpp"

class Parser: ParserBase {
public:
    Parser (Lexer* lexer): ParserBase(lexer) {}

    Ast* s ();

    Ast* pipe ();
    Ast* fn_app ();
    Ast* atom ();
};
