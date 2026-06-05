#include "config/Parser.hpp"

#include <stdexcept>
#include <vector>

#include "config/Lexer.hpp"

namespace conf {
    Parser::Parser() : _pos(0) {}
    Parser::Parser(const std::vector<Token>& tokens) : _tokens(tokens) {}
    Parser::Parser(const Parser& other) : _tokens(other._tokens), _pos(other._pos) {}
    
    Parser& Parser::operator=(const Parser& other) {
        if (this != &other) {
            _tokens = other._tokens;
            _pos = other._pos;
        }
        return (*this);
    }
    
    Parser::~Parser() {}

    Parser::SyntaxError::SyntaxError (const std::string& errorMessage) : std::runtime_error(errorMessage) {}
}

