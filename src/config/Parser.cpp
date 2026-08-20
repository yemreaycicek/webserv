/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-06 / 01:29:42
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-20 / 15:36:38
 */

#include "config/Parser.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "utils/str.hpp"

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
        _serverHandler["client_max_body_size"] = &Parser::parseClientMaxBodySize;
        _serverHandler["error_page"] = &Parser::parseErrorPage;

        _locationHandler["root"] = &Parser::parseRoot;
        _locationHandler["index"] = &Parser::parseIndex;
        _locationHandler["allow_methods"] = &Parser::parseAllowMethods;
        _locationHandler["autoindex"] = &Parser::parseAutoindex;
        _locationHandler["return"] = &Parser::parseReturn;
        _locationHandler["upload_enable"] = &Parser::parseUploadEnable;
        _locationHandler["upload_store"] = &Parser::parseUploadStore;
        _locationHandler["cgi_extension"] = &Parser::parseCgiExtension; //!check
        _locationHandler["cgi_pass"] = &Parser::parseCgiPass;           //!check
        _locationHandler["client_max_body_size"] = &Parser::parseClientMaxBodySize;

    }

    void Parser::parseCgiExtension(LocationBlock& location) {
        location.cgiExtension = consumeWord("cgi_extension");
    }
    
    void Parser::parseCgiPass(LocationBlock& location) {
        location.cgiPass = consumeWord("cgi_pass");
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
        parseListenAddress(consumeWord("listen"), server);
    }

    void Parser::parseClientMaxBodySize(ServerBlock& server) {
        server.clientMaxBodySize = parseSize(consumeWord("client_max_body_size"), "client_max_body_size");
    }

    void Parser::parseErrorPage(ServerBlock& server) {
        std::string line = consumeWord("error_page");
        int statusCode = parseStatusCode(line, "error_page");
        std::string path = consumeWord("error_page");
        validatePath(path, "error_page");
        server.errorPages[statusCode] = path;
    }

    void Parser::parseRoot(LocationBlock& location) {
        location.root = consumeWord("root");
    }
    
    void Parser::parseIndex(LocationBlock& location) {
        location.index = consumeWord("index");
    }

    void Parser::parseAllowMethods(LocationBlock& location) {
        std::string method = consumeWord("allow_methods");
        do {
            if (method != "GET" && method != "POST" && method != "DELETE") {
                throw SyntaxError("Unsupported HTTP method '" + method + "' in allow_methods (Only GET, POST, DELETE allowed)");
            }
            location.allowMethods.push_back(method);
            if (isAtEnd() || peek().type == TOKEN_SEMICOLON) {
                break;
            }
            method = consumeWord("allow_methods");
        } while (true);
    }

    void Parser::parseAutoindex(LocationBlock& location) {
        location.autoindex = parseBoolean(consumeWord("autoindex"), "autoindex");
    }

    void Parser::parseReturn(LocationBlock& location) {
        location.redirect.code = parseStatusCode(consumeWord("return"), "return");

        std::string target = consumeWord("return");
        if (target.at(0) != '/' && target.find("://") == std::string::npos) {
            throw SyntaxError("Redirect target '" + target + "' in directive 'return' must be a path or an absolute URL");
        }
        location.redirect.target = target;
    }

    void Parser::parseUploadEnable(LocationBlock& location) {
        location.uploadEnable = parseBoolean(consumeWord("upload_enable"), "upload_enable");
    }

    void Parser::parseUploadStore(LocationBlock& location) {
        location.uploadStore = consumeWord("upload_store");
    }
    void Parser::parseClientMaxBodySize(LocationBlock& location) {
        location.clientMaxBodySize = parseSize(consumeWord("client_max_body_size"), "client_max_body_size");
    }

    void Parser::validateServerBlock(const ServerBlock& server) const {
        if (server.listen.empty()) {
            throw SyntaxError("Server block must contain at least one 'listen' directive!");
        }

        for (std::size_t i = 0; i < server.locations.size(); i++) {
            for (std::size_t j = i + 1; j < server.locations.size(); j++) {
                if (server.locations.at(i).path == server.locations.at(j).path) {
                    throw SyntaxError("Duplicate location path '" + server.locations.at(i).path + "' defined in the same server block!");
                }
            }
            
        }
        
    }

    void Parser::validateLocationBlock(const LocationBlock& location) const {
        if (location.path.empty()) {
            throw SyntaxError("Location block must have a valid path!");
        } else if (location.path.at(0) != '/') {
            throw SyntaxError("Location path '" + location.path + "' must begin with a '/' character!");
        }
        
        if (location.root.empty() && !location.redirect.isSet()) {
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

    void Parser::parseListenAddress(const std::string& value, ServerBlock& server) const {
        std::size_t colonPos = value.find(':');
        if (colonPos == std::string::npos) {
            throw SyntaxError("Invalid listen format '" + value + "' (Expected format: IP:PORT e.g., 127.0.0.1:8080)");
        }

        std::string ip = value.substr(0, colonPos);
        std::string portString = value.substr(colonPos + 1);
        int port;

        if (!str::to_numeric(portString, port)) {
            throw SyntaxError("Invalid numeric format or integer overflow in port '" + portString + "'");
        } else if (port < 1 || port > 65535) {
            throw SyntaxError("Port '" + portString + "' is out of valid range (1 - 65535)");
        }

        server.listen.push_back(value);        
    }

    std::size_t Parser::parseSize(const std::string& value, const std::string& directiveName) const {
        std::size_t len = value.length();
        char unit = std::tolower(value.at(len - 1));
        std::string numberString = value;
        std::size_t multiplier = 1;

        if (unit == 'k') {
            multiplier = 1024;
            numberString = value.substr(0, len - 1);
        } else if (unit == 'm') {
            multiplier = 1024 * 1024;
            numberString = value.substr(0, len - 1);
        }

        std::size_t rawValue = 0;

        if (!str::to_numeric(numberString, rawValue)) {
            throw SyntaxError("Invalid size format or integer overflow in '" + value + "' for directive '" + directiveName + "'");
        } else if (rawValue > std::numeric_limits<std::size_t>::max() / multiplier) {
            throw SyntaxError("Size calculation overflow! The value '" + value + "' exceeds system limits.");
        }

        return (rawValue * multiplier);
    }

    int Parser::parseStatusCode(const std::string& value, const std::string& directiveName) const {
        int code;

        if (!str::to_numeric(value, code)) {
            throw SyntaxError("Invalid number or integer overflow in status code '" + value + "' for directive '" + directiveName + "'");
        } else if (code < 100 || code > 599) {
            throw SyntaxError("Status code '" + value + "' out of RFC 9112 bounds (100-599) in directive '" + directiveName + "'");
        }
        return (code);
    }

    void Parser::validatePath(const std::string& path, const std::string& directiveName) const {
        if (path.at(0) != '/') {
            throw SyntaxError("Path '" + path + "' in directive '" + directiveName + "' must begin with '/'");
        }   
    }

    bool Parser::parseBoolean(const std::string& value, const std::string& directiveName) const {
        if (value == "on") {
            return (true);
        } else if (value == "off") {
            return (false);
        }
        throw SyntaxError("Invalid value '" + value + "' for directive '" + directiveName + "' (Expected 'on' or 'off')");
    }


    Parser::SyntaxError::SyntaxError (const std::string& message) : Exception(message) {}
    Parser::SyntaxError::~SyntaxError() throw() {}
}

