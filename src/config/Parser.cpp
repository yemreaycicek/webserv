/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-06 / 01:29:42
 * @ Modified by: yaycicek
 * @ Modified time: 2026-07-09 / 21:29:51
 */

#include "config/Parser.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>
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
        } catch (const Exception&) {
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
        validateServerBlock(server);
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
        
        location.path = consumeWord("location");
        expect(TOKEN_OPENING_BRACE, "Expected '{' after location path");

        while (!isAtEnd() && peek().type != TOKEN_CLOSING_BRACE) {
            parseLocationDirective(location);
        }
        expect(TOKEN_CLOSING_BRACE, "Expected '}' to close 'location' block");
        validateLocationBlock(location);
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
        server.listen.push_back(consumeWord("listen"));
    }

    void Parser::parseClientHeaderBufferSize(ServerBlock& server) {
        server.clientHeaderBufferSize = consumeWord("client_header_buffer_size");
    }

    void Parser::parseClientMaxBodySize(ServerBlock& server) {
        server.clientMaxBodySize = consumeWord("client_max_body_size");
    }

    void Parser::parseErrorPage(ServerBlock& server) {
        std::string line = consumeWord("error_page");
        long long statusCode = std::atoll(line.c_str());
        std::string errorPagePath = consumeWord("error_page");
        server.errorPages.insert(std::make_pair(statusCode, errorPagePath));
    }

    void Parser::parseRoot(LocationBlock& location) {
        location.root = consumeWord("root");
    }
    
    void Parser::parseIndex(LocationBlock& location) {
        location.index = consumeWord("index");
    }

    void Parser::parseAllowMethods(LocationBlock& location) {
        location.allowMethods.push_back(consumeWord("allow_methods"));
        while (!isAtEnd() && peek().type != TOKEN_SEMICOLON) {
            location.allowMethods.push_back(consumeWord("allow_methods"));
        }
    }

    void Parser::parseAutoindex(LocationBlock& location) {
        location.autoindex = (consumeWord("autoindex") == "on");
    }

    void Parser::parseReturn(LocationBlock& location) {
        std::string code = consumeWord("return");
        std::string url = consumeWord("return");
        location.redirect = code + " " + url;
    }

    void Parser::parseUploadEnable(LocationBlock& location) {
        location.uploadEnable = (consumeWord("upload_enable") == "on");
    }

    void Parser::parseUploadStore(LocationBlock& location) {
        location.uploadStore = consumeWord("upload_store");
    }

    void Parser::validateServerBlock(const ServerBlock& server) const {
        if (server.listen.empty()) {
            throw SyntaxError("Server block must contain at least one 'listen' directive!");
        }
    }

    void Parser::validateLocationBlock(const LocationBlock& location) const {
        if (location.path.empty()) {
            throw SyntaxError("Location block must have a valid path!");
        } else if (location.path.at(0) != '/') {
            throw SyntaxError("Location path '" + location.path + "' must begin with a '/' character!");
        }
        
        if (location.root.empty() && location.redirect.empty()) {
            throw SyntaxError("Location '" + location.path + "' must have either a 'root' or a 'return' directive!");
        }

        if (location.uploadEnable && location.uploadStore.empty()) {
            throw SyntaxError("Location '" + location.path + "' has upload enabled but no 'upload_store' directory defined!");
        }

        
    }

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
            throw SyntaxError(message + " (Found: '" + peek().value + "')");
        }
    }

    const std::string Parser::consumeWord(const std::string& directiveName) {
        if (peek().type != TOKEN_WORD) {
            throw SyntaxError("Expected argument for directive '" + directiveName + "', but found '" + peek().value + "'");
        }
        return (advance().value);
    }

    Parser::SyntaxError::SyntaxError (const std::string& message) : Exception(message) {}
    Parser::SyntaxError::~SyntaxError() throw() {}
}

