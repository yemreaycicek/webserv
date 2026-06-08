/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-06 / 01:29:42
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-08 / 11:48:03
 */

#include "config/ConfigBlocks.hpp"
#include "config/Parser.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

#include "config/Lexer.hpp"

namespace conf {
    Parser::Parser() : _pos(0) {}
    Parser::Parser(const std::vector<Token>& tokens) : _tokens(tokens) {}
    Parser::Parser(const Parser& other) : _tokens(other._tokens), _pos(other._pos) {}
    
    Parser& Parser::operator=(const Parser& other) {
        if (this != &other) {
            _tokens = other._tokens;
            _pos = other._pos;
        }
        return (*this);
    }
    
    Parser::~Parser() {}

    bool Parser::isAtEnd() const {
        return (_pos >= _tokens.size() || _tokens.at(_pos).type == TOKEN_EOF);
    }

    const Token& Parser::peek() const {
        if (isAtEnd()) return(_tokens.back());
        return (_tokens.at(_pos));
    }

    const Token& Parser::advance() {
        if (!isAtEnd()) _pos++;
        return (_tokens.at(_pos - 1));
    }

    void Parser::expect(TokenType type, const std::string& message) {
        if (peek().type == type) {
            advance();
        } else {
            std::string errorMessage = "Syntax Error: " + message + " (Found: '" + peek().value + "')";
            throw SyntaxError(errorMessage);
        }
    }

    LocationBlock Parser::parseLocationBlock() {
        LocationBlock location;
        return (location);
    }

    void Parser::parseDirective(ServerBlock& server) {
        Token directiveName = advance();

        if (directiveName.value == "listen") {
            server.listen = advance().value;
        } else if (directiveName.value == "client_max_body_size") {
            server.clientMaxBodySize = advance().value;
        } else if (directiveName.value == "error_page") {
            long long statusCode = std::atoll(advance().value.c_str());
            std::string errorPagePath = advance().value;
            server.errorPages[statusCode] = errorPagePath;
        } else {
            while (!isAtEnd() && peek().type != TOKEN_SEMICOLON) {
                advance();
            }
        }
        expect(TOKEN_SEMICOLON, "Expected ';' after directive '" + directiveName.value + "'");
    }

    ServerBlock Parser::parseServerBlock() {
        ServerBlock server;

        while (!isAtEnd() && peek().type != TOKEN_CLOSING_BRACE) {
            Token current = peek();

            if (current.type == TOKEN_WORD && current.value == "location") {
                advance();
                server.locations.push_back(parseLocationBlock());
            } else if (current.type == TOKEN_WORD) {
                parseDirective(server);
            } else {
                throw SyntaxError("Unexpected token in server block: '" + current.value + "'");
            }
        }
        expect(TOKEN_CLOSING_BRACE, "Expected '}' to close 'server' block");
        return (server);
    }

    std::vector<ServerBlock> Parser::parse() {
        std::vector<ServerBlock> servers;
        _pos = 0;

        while (!isAtEnd()) {
            Token current = advance();

            if (current.type == TOKEN_WORD && current.value == "server") {
                expect(TOKEN_OPENING_BRACE, "Expected '{' after 'server' declaration");
                servers.push_back(parseServerBlock());
            } else {
                throw SyntaxError("Expected 'server' block but found '" + current.value + "'");
            }
        }
        return (servers);
    }

    Parser::SyntaxError::SyntaxError (const std::string& errorMessage) : std::runtime_error(errorMessage) {}
}

