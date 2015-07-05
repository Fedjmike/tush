#include "ast-printer.hpp"

#include <cstdio>

#include "fn-map.hpp"

static void print_literal (const Ast::Literal* node) {
    printf("literal: %s\n", node->token.buffer.c_str());
}

static void print_fn_app (const Ast::FnApp* node) {
    puts("fn app");

    for (auto& param: node->params)
        print_ast(param.get());

    printf("fn: ");
    print_ast(node->fn.get());
}

void print_ast (const Ast* node) {
    fn_map(node, print_literal, print_fn_app);
}
