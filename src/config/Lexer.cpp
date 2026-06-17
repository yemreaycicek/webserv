/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-03 / 17:29:34
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-17 / 20:46:58
 */

#include "config/Lexer.hpp"

#include <cctype>
#include <string>
#include <vector>
#include <iostream>

#ifdef DEBUG
  #include "config/Debug.hpp"
#endif

namespace config {
    Lexer::Lexer() : _pos(0) {}
    Lexer::Lexer(const Lexer& other) : _input(other._input), _pos(other._pos) {}
    
    Lexer& Lexer::operator=(const Lexer& other) {
        if (this != &other) {
            _input = other._input;
            _pos = other._pos;
        }
        return (*this);
    }
    
    Lexer::~Lexer() {}

    std::vector<Token> Lexer::tokenize(const std::string& input) {
        std::vector<Token> tokens;

        _input = input;
        while (_pos < _input.length()) {
            skipWhitespaceAndComments();

            if (_pos >= _input.length()) {
                break;
            }

            char c = _input[_pos];

            if (c == '{') {
                tokens.push_back(Token(TOKEN_OPENING_BRACE, "{"));
                _pos++;
            } else if (c == '}') {
                tokens.push_back(Token(TOKEN_CLOSING_BRACE, "}"));
                _pos++;
            } else if (c == ';') {
                tokens.push_back(Token(TOKEN_SEMICOLON, ";"));
                _pos++;
            } else {
                std::size_t start = _pos;

                while (_pos < _input.length() && !isspace(_input[_pos]) && !isStructuralChar(_input[_pos])) {
                    _pos++;
                }

                std::string word = _input.substr(start, (_pos - start));
                tokens.push_back(Token(TOKEN_WORD, word));
            }
        }
        tokens.push_back(Token(TOKEN_EOF, "EOF"));
        #ifdef DEBUG
            config::Debug::printLexer(tokens);
        #endif
        return (tokens);
    }

    void Lexer::skipWhitespaceAndComments() {
        while (_pos < _input.length()) {
            char c = _input[_pos];

            if (c == '\n') {
                _pos++;
            } else if (c == ' ' || c == '\t' ||  c == '\r' || c == '\v' || c == '\f') {
                _pos++;
            } else if (c == '#') {
                while (_pos < _input.length() && _input[_pos] != '\n') {
                    _pos++;
                }
            } else {
                break;
            }
        }
    }

    bool Lexer::isStructuralChar(char c) const {
        return (c == '{' || c == '}' || c == ';');
    }
}
