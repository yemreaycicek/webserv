/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-03 / 17:29:34
 * @ Modified by: yaycicek
 * @ Modified time: 2026-07-02 / 14:20:32
 */

#include "config/Lexer.hpp"

#include <cctype>
#include <string>
#include <vector>

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

            if (c == CHAR_OPENING_BRACE) {
                addToken(tokens, TOKEN_OPENING_BRACE, "{");
            } else if (c == CHAR_CLOSING_BRACE) {
                addToken(tokens, TOKEN_CLOSING_BRACE, "}"); 
            } else if (c == CHAR_SEMICOLON) {
                addToken(tokens, TOKEN_SEMICOLON, ";");
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

    void Lexer::addToken(std::vector<Token>& tokens, const TokenType type, const std::string& value) {
        tokens.push_back(Token(type, value));
        _pos++;
    }
}
