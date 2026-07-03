/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-03 / 17:08:14
 * @ Modified by: yaycicek
 * @ Modified time: 2026-07-03 / 15:50:21
 */

#ifndef WEBSERV_CONFIG_LEXER_HPP
#define WEBSERV_CONFIG_LEXER_HPP

#include <string>
#include <vector>

namespace config {
    enum StructuralChar {
        CHAR_OPENING_BRACE = '{',
        CHAR_CLOSING_BRACE = '}',
        CHAR_SEMICOLON = ';'        
    };

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

        Token(const TokenType t, const std::string& v) : type(t), value(v) {}
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

            void skipWhitespaceAndComments();
            bool isStructuralChar(char c) const;

            void addToken(std::vector<Token>& tokens, const TokenType type, const std::string& value);
            const std::string parseWord();
    };
}

#endif // WEBSERV_CONFIG_LEXER_HPP
