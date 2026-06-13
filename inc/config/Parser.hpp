/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-06 / 00:08:40
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-13 / 12:11:19
 */

#ifndef WEBSERV_CONFIG_PARSER_HPP
#define WEBSERV_CONFIG_PARSER_HPP

#include <stdexcept>
#include <vector>

#include "config/ConfigBlocks.hpp"
#include "config/Lexer.hpp"

namespace conf {
    class Parser {
        public:
            Parser(const std::vector<Token>& tokens);
            Parser(const Parser& other);
            Parser& operator=(const Parser& other);
            ~Parser();

            std::vector<ServerBlock> parse();
    
        private:
            std::vector<Token> _tokens;
            std::size_t _pos;

            Parser();

            bool isAtEnd() const;
            const Token& peek() const;
            const Token& advance();
            void expect(TokenType type, const std::string& errorMessage);

            LocationBlock parseLocationBlock();
            ServerBlock parseServerBlock();
            void parseServerDirective(ServerBlock& server);
            void parseLocationDirective(LocationBlock& location);

            class SyntaxError : public std::runtime_error {
                public:
                    SyntaxError(const std::string& errorMessage);
            };

        #ifdef DEBUG
            void printServerBlocks(const std::vector<ServerBlock>& servers) const;
            void printLocationBlock(const LocationBlock& location, const bool isLastLocation) const;
            std::string joinMethods(const std::vector<std::string>& methods) const;
        #endif
    };
}

#endif // WEBSERV_CONFIG_PARSER_HPP
