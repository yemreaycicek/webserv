/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-03 / 17:29:34
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-05 / 23:39:06
 */

#include "config/Lexer.hpp"

#include <cctype>
#include <string>
#include <vector>
#include <iostream>

#ifdef DEBUG
    #include <iomanip>
#endif

namespace conf {
    Lexer::Lexer() : _pos(0) {}
    Lexer::Lexer(const Lexer& other) : _input(other._input), _pos(other._pos) {}
    
    Lexer& Lexer::operator=(const Lexer& other) {
        if (this != &other) {
            _input = other._input;
            _pos = other._pos;
            _tokens = other._tokens;
        }
        return (*this);
    }
    
    Lexer::~Lexer() {}

    std::vector<Token> Lexer::tokenize(const std::string& input) {
        _input = input;

        while (_pos < _input.length()) {
            skipWhitespaceAndComments();

            if (_pos >= _input.length()) {
                break;
            }

            char c = _input[_pos];

            if (c == '{') {
                addToken(TOKEN_OPENING_BRACE, "{");
                _pos++;
            } else if (c == '}') {
                addToken(TOKEN_CLOSING_BRACE, "}");
                _pos++;
            } else if (c == ';') {
                addToken(TOKEN_SEMICOLON, ";");
                _pos++;
            } else {
                std::size_t start = _pos;

                while (_pos < _input.length() && !isspace(_input[_pos]) && !isStructuralChar(_input[_pos])) {
                    _pos++;
                }

                std::string word = _input.substr(start, (_pos - start));
                addToken(TOKEN_WORD, word);
            }
        }
        addToken(TOKEN_EOF, "EOF");
        #ifdef DEBUG
            printTokens();
        #endif
        return (_tokens);
    }

    void Lexer::addToken(TokenType type, const std::string& value) {
        _tokens.push_back(Token(type, value));
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

    #ifdef DEBUG
        std::string Lexer::getTokenTypeString(conf::TokenType type) const {
            switch(type) {
                case conf::TOKEN_WORD:          return ("WORD");
                case conf::TOKEN_OPENING_BRACE: return ("OPENING_BRACE");
                case conf::TOKEN_CLOSING_BRACE: return ("CLOSING_BRACE");
                case conf::TOKEN_SEMICOLON:     return ("SEMICOLON");
                case conf::TOKEN_EOF:           return ("EOF");
                default:                        return ("UNKNOWN");
            }
        }
    
        void Lexer::printTokens() const {
            std::cout << "--- LEXER OUTPUT ---" << std::endl;
            for (std::vector<Token>::const_iterator it = _tokens.begin(); it != _tokens.end(); ++it) {
                std::cout << std::left << std::setw(13) << getTokenTypeString(it->type) << " → '" << it->value << "'" << std::endl;
            }
            std::cout << "--------------------" << std::endl;
        }
    #endif
}
