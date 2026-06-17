/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-17 / 14:02:31
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-17 / 17:50:36
 */

#include "config/Debug.hpp"

#include <vector>

#include "utils/io.hpp"

namespace config {
    void Debug::printLexer(const std::vector<config::Token>& tokens) {
        for (std::vector<config::Token>::const_iterator it = tokens.begin(); it != tokens.end(); it++) {
            io::println(io::padRight(getTokenType(it->type), 13) + " → '" + it->value + "'");
        }
    }
    
    std::string Debug::getTokenType(config::TokenType type) {
        switch (type) {
            case config::TOKEN_WORD:
                return ("WORD");
            case config::TOKEN_OPENING_BRACE:
                return ("OPENING_BRACE");
            case config::TOKEN_CLOSING_BRACE:
                return ("CLOSING_BRACE");
            case config::TOKEN_SEMICOLON:
                return ("SEMICOLON");
            case config::TOKEN_EOF:
                return ("EOF");
            default:
                return ("UNKNOWN");
        }
    }
}
