#include "ast-printer.hpp"

#include <cstdio>

#include "fn-map.hpp"

namespace AST {

void print_literal (const AST::Literal* node) {
    printf("literal: %s\n", node->token.buffer.c_str());
}

void print_fn_app (const AST::FnApp* node) {
    puts("fn app");

    for (auto& param: node->params)
        AST::print(param.get());

    printf("fn: ");
    AST::print(node->fn.get());
}

}

void AST::print (const Ast* node) {
    fn_map(node, print_literal, print_fn_app);
}
