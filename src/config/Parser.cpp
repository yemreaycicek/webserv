/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-06 / 01:29:42
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-06 / 01:48:21
 */

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

    bool Parser::isAtEnd() const {
        return (_pos >= _tokens.size() || _tokens.at(_pos).type == TOKEN_EOF);
    }

    const Token& Parser::peek() const {
        if (isAtEnd()) return(_tokens.back());
        return (_tokens.at(_pos));
    }

    const Token& Parser::advance() {
        if (!isAtEnd()) _pos++;
        return (_tokens.at(_pos - 1));
    }

    void Parser::expect(TokenType type, const std::string& message) {
        if (peek().type == type) {
            advance();
        } else {
            std::string errorMessage = "Syntax Error: " + message + " (Found: '" + peek().value + "')";
            throw SyntaxError(errorMessage);
        }
    }

    Parser::SyntaxError::SyntaxError (const std::string& errorMessage) : std::runtime_error(errorMessage) {}
}

