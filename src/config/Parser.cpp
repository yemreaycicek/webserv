#include "config/Parser.hpp"

#include <vector>

#include "config/Lexer.hpp"

namespace conf {
    Parser::Parser() {}
    Parser::Parser(const std::vector<Token>& tokens) {
        (void)tokens;
    }
    Parser::Parser(const Parser& other) {
        (void)other;
    }
    
    Parser& Parser::operator=(const Parser& other) {
        (void)other;
        return (*this);
    }
    
    Parser::~Parser() {}
}

