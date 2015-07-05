#include <sstream>

#include "common.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include "ast-printer.hpp"

int main (int argc, char** argv) {
    (void) argc, (void) argv;

    std::string str = "*.cpp wc | sort";
    std::istringstream input(str);

    Lexer lexer(input);
    Parser parser(&lexer);
    
    auto tree = parser.s();

    print_ast(tree);

    delete tree;
}
