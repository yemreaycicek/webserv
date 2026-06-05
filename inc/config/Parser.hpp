/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-06 / 00:08:40
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-06 / 00:11:08
 */

#ifndef WEBSERV_CONFIG_PARSER_HPP
#define WEBSERV_CONFIG_PARSER_HPP

#include <vector>

#include "config/Lexer.hpp"

namespace conf {
    class Parser {
        public:
            Parser(const std::vector<Token>& tokens);
            Parser(const Parser& other);
            Parser& operator=(const Parser& other);
            ~Parser();
    
        private:
            Parser();
    };
}

#endif // WEBSERV_CONFIG_PARSER_HPP
