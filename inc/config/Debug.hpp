/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-17 / 14:02:20
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-17 / 19:14:47
 */

#ifndef WEBSERV_CONFIG_DEBUG_HPP
#define WEBSERV_CONFIG_DEBUG_HPP

#include <string>
#include <vector>

#include "config/Lexer.hpp"
#include "config/Parser.hpp"

namespace config {
    class Debug {
        public:
            static void printLexer(const std::vector<config::Token>& tokens);
            static void printParser(const std::vector<config::ServerBlock>& servers);
    
        private:
            static std::string getTokenType(config::TokenType type);

            static std::string joinMethods(const std::vector<std::string>& methods);
            static void printServerBlock(const config::ServerBlock& server, std::size_t index);
            static void printLocationBlock(const config::LocationBlock& location, bool isLastLocation);
    };
}

#endif // WEBSERV_CONFIG_DEBUG_HPP
