/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-06 / 01:29:42
 * @ Modified by: yaycicek
 * @ Modified time: 2026-07-09 / 14:47:40
 */

#include "config/Parser.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef DEBUG
  #include "config/Debug.hpp"
#endif

namespace config {
    Parser::Parser() : _pos(0) {
        initHandlers();
    }
    Parser::Parser(const std::vector<Token>& tokens) : _tokens(tokens), _pos(0) {
        initHandlers();
    }
    Parser::Parser(const Parser& other) : _tokens(other._tokens), _pos(other._pos), _serverHandler(other._serverHandler), _locationHandler(other._locationHandler) {}
    
    Parser& Parser::operator=(const Parser& other) {
        if (this != &other) {
            _tokens = other._tokens;
            _pos = other._pos;
            _serverHandler = other._serverHandler;
            _locationHandler = other._locationHandler;
        }
        return (*this);
    }
    
    Parser::~Parser() {}

    void Parser::initHandlers() {
        _serverHandler["listen"] = &Parser::parseListen;
        _serverHandler["client_header_buffer_size"] = &Parser::parseClientHeaderBufferSize;
        _serverHandler["client_max_body_size"] = &Parser::parseClientMaxBodySize;
        _serverHandler["error_page"] = &Parser::parseErrorPage;

        _locationHandler["root"] = &Parser::parseRoot;
        _locationHandler["index"] = &Parser::parseIndex;
        _locationHandler["allow_methods"] = &Parser::parseAllowMethods;
        _locationHandler["autoindex"] = &Parser::parseAutoindex;
        _locationHandler["return"] = &Parser::parseReturn;
        _locationHandler["upload_enable"] = &Parser::parseUploadEnable;
        _locationHandler["upload_store"] = &Parser::parseUploadStore;
    }

    std::vector<ServerBlock> Parser::parse() {
        std::vector<ServerBlock> servers;

        try {
            while (!isAtEnd()) {
                Token current = advance();
    
                if (current.type == TOKEN_WORD && current.value == "server") {
                    expect(TOKEN_OPENING_BRACE, "Expected '{' after 'server' declaration");
                    servers.push_back(parseServerBlock());
                } else {
                    throw SyntaxError("Expected 'server' block but found '" + current.value + "'");
                }
            }
        } catch (const SyntaxError& e) {
            #ifdef DEBUG
                config::Debug::printParser(servers);
            #endif
            throw;
        }
        #ifdef DEBUG
            config::Debug::printParser(servers);
        #endif
        return (servers);
    }

    ServerBlock Parser::parseServerBlock() {
        ServerBlock server;

        while (!isAtEnd() && peek().type != TOKEN_CLOSING_BRACE) {
            Token current = peek();

            if (current.type == TOKEN_WORD && current.value == "location") {
                advance();
                server.locations.push_back(parseLocationBlock());
            } else if (current.type == TOKEN_WORD) {
                parseServerDirective(server);
            } else {
                throw SyntaxError("Unexpected token in server block: '" + current.value + "'");
            }
        }
        expect(TOKEN_CLOSING_BRACE, "Expected '}' to close 'server' block");
        return (server);
    }

    void Parser::parseServerDirective(ServerBlock& server) {
        Token directiveName = advance();

        std::map<std::string, ServerDirectiveHandler>::iterator it = _serverHandler.find(directiveName.value);
        if (it != _serverHandler.end()) {
            (this->*(it->second))(server);
        } else {
            throw SyntaxError("Unknown server directive: '" + directiveName.value + "'");
        }
        expect(TOKEN_SEMICOLON, "Expected ';' after directive '" + directiveName.value + "'");
    }

    LocationBlock Parser::parseLocationBlock() {
        LocationBlock location;
        
        location.path = advance().value;
        expect(TOKEN_OPENING_BRACE, "Expected '{' after location path");

        while (!isAtEnd() && peek().type != TOKEN_CLOSING_BRACE) {
            parseLocationDirective(location);
        }
        expect(TOKEN_CLOSING_BRACE, "Expected '}' to close 'location' block");
        return (location);
    }

    void Parser::parseLocationDirective(LocationBlock& location) {
        Token directiveName = advance();

        std::map<std::string, LocationDirectiveHandler>::iterator it = _locationHandler.find(directiveName.value);
        if (it != _locationHandler.end()) {
            (this->*(it->second))(location);
        } else {
            throw SyntaxError("Unknown location directive: '" + directiveName.value + "'");
        }
        expect(TOKEN_SEMICOLON, "Expected ';' after directive '" + directiveName.value + "'");
    }

    void Parser::parseListen(ServerBlock& server) {
        server.listen.push_back(advance().value);
    }

    void Parser::parseClientHeaderBufferSize(ServerBlock& server) {
        server.clientHeaderBufferSize = advance().value;
    }

    void Parser::parseClientMaxBodySize(ServerBlock& server) {
        server.clientMaxBodySize = advance().value;
    }

    void Parser::parseErrorPage(ServerBlock& server) {
        long long statusCode = std::atoll(advance().value.c_str());
        std::string errorPagePath = advance().value;
        server.errorPages[statusCode] = errorPagePath;
    }

    void Parser::parseRoot(LocationBlock& location) {
        location.root = advance().value;
    }
    
    void Parser::parseIndex(LocationBlock& location) {
        location.index = advance().value;
    }

    void Parser::parseAllowMethods(LocationBlock& location) {
        while (!isAtEnd() && peek().type != TOKEN_SEMICOLON) {
            location.allowMethods.push_back(advance().value);
        }
    }

    void Parser::parseAutoindex(LocationBlock& location) {
        location.autoindex = (advance().value == "on");
    }

    void Parser::parseReturn(LocationBlock& location) {
        location.redirect = advance().value + " " + advance().value;
    }

    void Parser::parseUploadEnable(LocationBlock& location) {
        location.uploadEnable = (advance().value == "on");
    }

    void Parser::parseUploadStore(LocationBlock& location) {
        location.uploadStore = advance().value;
    }

    bool Parser::isAtEnd() const {
        return (_pos >= _tokens.size() || _tokens.at(_pos).type == TOKEN_EOF || _tokens.at(_pos).value.empty());
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

    Parser::SyntaxError::SyntaxError (const std::string& message) : Exception(message) {}
    Parser::SyntaxError::~SyntaxError() throw() {}
}

