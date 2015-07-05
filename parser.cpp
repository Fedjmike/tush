#include "parser.hpp"

#include <vector>
#include <memory>

#include "ast.hpp"

template<typename T>
using vector = std::vector<T>;

Ast* Parser::s () {
    return pipe();
}

Ast* Parser::pipe () {
    auto node = fn_app();

    while (try_match("|")) {
        auto fn = fn_app();
        node = new AST::FnApp(fn, vector<Ast*> {node});
    }

    return node;
}

Ast* Parser::fn_app () {
    vector<Ast*> nodes;
    int exprs = 0;
    /*Filled iff there is a backtick function*/
    Ast* fn = 0;

    while (waiting_for(")") && waiting_for("|")) {
        if (try_match("`")) {
            if (fn)
                error("Multiple explicit functions in backticks: '" + current.buffer + "'");

            fn = atom();
            match("`");

        } else
            nodes.push_back(atom());

        exprs++;
    }

    if (exprs == 1)
        return nodes[0];

    else {
        /*If not explicitly marked in backticks, the last expr was the fn*/
        if (!fn) {
            fn = nodes.back();
            nodes.pop_back();
        }

        return new AST::FnApp(fn, nodes);
    }
}

Ast* Parser::atom () {
    auto node = new AST::Literal(current);

    if (see_kind(Token::normal))
        accept();

    else
        expected("expression");

    return node;
}
