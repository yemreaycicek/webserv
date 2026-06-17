/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-17 / 14:02:20
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-17 / 17:51:05
 */

#ifndef WEBSERV_CONFIG_DEBUG_HPP
#define WEBSERV_CONFIG_DEBUG_HPP

#include <string>
#include <vector>

#include "config/Lexer.hpp"

namespace config {
    class Debug {
        public:
            static void printLexer(const std::vector<config::Token>& tokens);
    
        private:
            static std::string getTokenType(config::TokenType type);
    };
}

#endif // WEBSERV_CONFIG_DEBUG_HPP
