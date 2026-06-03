/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-03 / 17:08:14
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-03 / 17:56:10
 */

#ifndef WEBSERV_CONFIG_LEXER_HPP
#define WEBSERV_CONFIG_LEXER_HPP

#include <string>
#include <vector>

namespace conf {
    enum TokenType {
        TOKEN_WORD,
        TOKEN_OPENING_BRACE,
        TOKEN_CLOSING_BRACE,
        TOKEN_SEMICOLON,
        TOKEN_EOF
    };

    struct Token {
        TokenType type;
        std::string value;
        std::size_t line;

        Token(const TokenType t, const std::string& v, const std::size_t l) : type(t), value(v), line(l) {}
    };

    class Lexer {
        public:
            Lexer();
            Lexer(const Lexer& other);
            Lexer& operator=(const Lexer& other);
            ~Lexer();

            std::vector<Token> tokenize(const std::string& input);
    
        private:
            std::string _input;
            std::size_t _pos;
            std::size_t _line;
            std::vector<Token> _tokens;

            void addToken(TokenType type, const std::string& value);
            void skipWhitespaceAndComments();
            bool isStructuralChar(char c) const;
    };
}

#endif // WEBSERV_CONFIG_LEXER_HPP
