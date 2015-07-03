#pragma once

#include <memory>
#include <vector>

#include "token.hpp"

namespace AST {
    struct Node {

    };

    struct Expr: public Node {

    };

    struct FnApp: public Expr {
        /*TODO: review use of std::vector*/
        std::unique_ptr<Node> fn;
        std::vector<std::unique_ptr<Node>> params;

        FnApp (Node* fn, std::vector<Node*> params)
            : fn(std::unique_ptr<Node>(fn)),
              params{} {
            for (Node* param: params)
                this->params.push_back(std::unique_ptr<Node>(param));
        }
    };

    struct Literal: public Expr {
        Token token;

        Literal (Token token): token(token) {}
    };
};

using Ast = AST::Node;
