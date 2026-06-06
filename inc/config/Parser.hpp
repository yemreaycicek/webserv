/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-06 / 00:08:40
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-06 / 04:47:42
 */

#ifndef WEBSERV_CONFIG_PARSER_HPP
#define WEBSERV_CONFIG_PARSER_HPP

#include <stdexcept>
#include <vector>

#include "config/ConfigNodes.hpp"
#include "config/Lexer.hpp"

namespace conf {
    class Parser {
        public:
            Parser(const std::vector<Token>& tokens);
            Parser(const Parser& other);
            Parser& operator=(const Parser& other);
            ~Parser();

            std::vector<ServerNode> parse();
    
        private:
            std::vector<Token> _tokens;
            std::size_t _pos;

            Parser();

            bool isAtEnd() const;
            const Token& peek() const;
            const Token& advance();
            void expect(TokenType type, const std::string& errorMessage);

            ServerNode parseServerNode();

            class SyntaxError : public std::runtime_error {
                public:
                    SyntaxError(const std::string& errorMessage);
            };
    };
}

#endif // WEBSERV_CONFIG_PARSER_HPP
