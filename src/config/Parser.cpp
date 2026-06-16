/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-06 / 01:29:42
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-16 / 11:53:20
 */

#include "config/ConfigBlocks.hpp"
#include "config/Parser.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

#include "config/Lexer.hpp"

#ifdef DEBUG
    #include <iostream>
    #include <map>
#endif

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
    void Parser::parseLocationDirective(LocationBlock& location) {
        Token directiveName = advance();
        
        if (directiveName.value == "root") {
            location.root = advance().value;
        } else if (directiveName.value == "index") {
            location.index = advance().value;
        } else if (directiveName.value == "allow_methods") {
            while (!isAtEnd() && peek().type != TOKEN_SEMICOLON) {
                location.allowMethods.push_back(advance().value);
            }
        } else if (directiveName.value == "autoindex") {
            location.autoindex = advance().value == "on";
        } else if (directiveName.value == "return") {
            location.redirect = advance().value + " " + advance().value;
        } else if (directiveName.value == "upload_enable") {
            location.upload_enable = advance().value == "on";
        } else if (directiveName.value == "upload_store") {
            location.upload_store = advance().value;
        } else {
            while (!isAtEnd() && peek().type != TOKEN_SEMICOLON) {
                advance();
            }
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

    void Parser::parseServerDirective(ServerBlock& server) {
        Token directiveName = advance();

        if (directiveName.value == "listen") {
            server.listen.push_back(advance().value);
        } else if (directiveName.value == "client_header_buffer_size") {
            server.clientHeaderBufferSize = advance().value;
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
                parseServerDirective(server);
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
                #ifdef DEBUG
                    printServerBlocks(servers);
                #endif
                throw SyntaxError("Expected 'server' block but found '" + current.value + "'");
            }
        }
        #ifdef DEBUG
            printServerBlocks(servers);
        #endif
        return (servers);
    }

    Parser::SyntaxError::SyntaxError (const std::string& errorMessage) : std::runtime_error(errorMessage) {}

    #ifdef DEBUG
        std::string Parser::joinMethods(const std::vector<std::string>& methods) const {
            std::string res;
            for (size_t i = 0; i < methods.size(); ++i) {
                res += methods[i];
                if (i < methods.size() - 1) res += ", ";
            }
            return res;
        }

        void Parser::printLocationBlock(const LocationBlock& location, const bool isLastLocation) const {
            std::string prefix = isLastLocation ? "    " : "│   ";

            std::cout << (isLastLocation ? "└── " : "├── ") << "Location: '" << location.path << "'" << std::endl;
            
            std::vector<std::string> lines;
            if (!location.root.empty()) lines.push_back("root: " + location.root);
            if (!location.index.empty()) lines.push_back("index: " + location.index);
            lines.push_back(std::string("autoindex: ") + (location.autoindex ? "on" : "off"));
            if (!location.redirect.empty()) lines.push_back("return: " + location.redirect);
            
            if (location.upload_enable) {
                lines.push_back("upload_enable: on");
                if (!location.upload_store.empty()) lines.push_back("upload_store: " + location.upload_store);
            }
            if (!location.allowMethods.empty()) lines.push_back("allow_methods: " + joinMethods(location.allowMethods));

            for (size_t i = 0; i < lines.size(); ++i) {
                bool isLastLine = (i == lines.size() - 1);
                std::cout << prefix << (isLastLine ? "└── " : "├── ") << lines[i] << std::endl;
            }
        }

        void Parser::printServerBlocks(const std::vector<ServerBlock>& servers) const {
            std::cout << std::endl << "--- PARSER OUTPUT ---" << std::endl;

            for (std::size_t i = 0; i < servers.size(); ++i) {
                std::cout << "ServerBlock [" << i << "]" << std::endl;
                
                std::vector<std::string> serverLines;
                for (std::size_t j = 0; j < servers[i].listen.size(); ++j) {
                    serverLines.push_back("listen: " + servers[i].listen[j]);
                }
                if (!servers[i].clientHeaderBufferSize.empty()) serverLines.push_back("client_header_buffer_size: " + servers[i].clientHeaderBufferSize);
                if (!servers[i].clientMaxBodySize.empty()) serverLines.push_back("client_max_body_size: " + servers[i].clientMaxBodySize);

                for (std::size_t j = 0; j < serverLines.size(); ++j) {
                    bool isAbsolutelyLast = (servers[i].errorPages.empty() && servers[i].locations.empty() && j == serverLines.size() - 1);
                    std::cout << (isAbsolutelyLast ? "└── " : "├── ") << serverLines[j] << std::endl;
                }

                if (!servers[i].errorPages.empty()) {
                    bool noLocations = servers[i].locations.empty();
                    std::cout << (noLocations ? "└── " : "├── ") << "error_pages: " << std::endl;
                    std::string errPrefix = noLocations ? "    " : "│   ";
                    
                    std::size_t errCount = 0;
                    for (std::map<std::size_t, std::string>::const_iterator it = servers[i].errorPages.begin(); 
                            it != servers[i].errorPages.end(); ++it, ++errCount) {
                        bool isLastError = (errCount == servers[i].errorPages.size() - 1);
                        std::cout << errPrefix << (isLastError ? "└── " : "├── ") << it->first << " -> " << it->second << std::endl;
                    }
                }

                for (size_t j = 0; j < servers[i].locations.size(); j++) {
                    bool isLastLoc = (j == servers[i].locations.size() - 1);
                    printLocationBlock(servers[i].locations[j], isLastLoc);
                }
            }
            std::cout << "-----------------------";
            std::cout << std::endl << std::endl;
        }
    #endif
}

