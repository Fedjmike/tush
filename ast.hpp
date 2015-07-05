#pragma once

#include <memory>
#include <vector>

#include "token.hpp"

struct Ast {
    virtual ~Ast () {};

    struct Expr;
    struct FnApp;
    struct Literal;
};

struct Ast::Expr: public Ast {

};

struct Ast::FnApp: public Expr {
    /*TODO: review use of std::vector*/
    std::unique_ptr<Ast> fn;
    std::vector<std::unique_ptr<Ast>> params;

    FnApp (Ast* fn, std::vector<Ast*> params)
        : fn(std::unique_ptr<Ast>(fn)),
          params {} {
        for (Ast* param: params)
            this->params.push_back(std::unique_ptr<Ast>(param));
    }
};

struct Ast::Literal: public Expr {
    Token token;

    Literal (Token token): token(token) {}
};
