/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-03 / 17:29:34
 * @ Modified by: yaycicek
 * @ Modified time: 2026-07-02 / 17:35:10
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

            switch (c) {
                case CHAR_OPENING_BRACE:
                    addToken(tokens, TOKEN_OPENING_BRACE, "{");
                    break;
                case CHAR_CLOSING_BRACE:
                    addToken(tokens, TOKEN_CLOSING_BRACE, "}");
                    break;
                case CHAR_SEMICOLON:
                    addToken(tokens, TOKEN_SEMICOLON, ";");
                    break;
                default:
                    parseWord(tokens);
                    break;
            }
        }
        addToken(tokens, TOKEN_EOF, "EOF");
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
        return (c == CHAR_OPENING_BRACE || c == CHAR_CLOSING_BRACE || c == CHAR_SEMICOLON);
    }

    void Lexer::addToken(std::vector<Token>& tokens, const TokenType type, const std::string& value) {
        tokens.push_back(Token(type, value));
        _pos += value.length();
    }

    void Lexer::parseWord(std::vector<Token>& tokens) {
        std::size_t len = 0;

        while ((_pos + len) < _input.length() && !std::isspace(_input.at(_pos + len)) && !isStructuralChar(_input.at(_pos + len))) {
            len++;
        }
        std::string word = _input.substr(_pos, len);
        addToken(tokens, TOKEN_WORD, word);
    }
}
