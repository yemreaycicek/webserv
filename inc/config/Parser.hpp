/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-06 / 00:08:40
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-17 / 19:47:46
 */

#ifndef WEBSERV_CONFIG_PARSER_HPP
#define WEBSERV_CONFIG_PARSER_HPP

#include <map>
#include <stdexcept>
#include <vector>

#include "config/Lexer.hpp"

namespace config {
    struct LocationBlock {
        std::string path;
        std::string root;
        std::string index;
        bool autoindex;
        std::vector<std::string> allowMethods;
        std::string redirect;
        bool uploadEnable;
        std::string uploadStore;

        LocationBlock() : autoindex(false), uploadEnable(false) {}
    };

    struct ServerBlock {
        std::vector<std::string> listen;
        std::string clientHeaderBufferSize;
        std::string clientMaxBodySize;
        std::map<std::size_t, std::string> errorPages;
        std::vector<LocationBlock> locations;
    };

    class Parser {
        public:
            Parser(const std::vector<Token>& tokens);
            Parser(const Parser& other);
            Parser& operator=(const Parser& other);
            ~Parser();

            std::vector<ServerBlock> parse();

            class SyntaxError : public std::runtime_error {
                public:
                    SyntaxError(const std::string& errorMessage);
            };
    
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
    };
}

#endif // WEBSERV_CONFIG_PARSER_HPP
