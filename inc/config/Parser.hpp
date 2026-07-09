/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-06 / 00:08:40
 * @ Modified by: yaycicek
 * @ Modified time: 2026-07-09 / 22:17:20
 */

#ifndef WEBSERV_CONFIG_PARSER_HPP
#define WEBSERV_CONFIG_PARSER_HPP

#include <map>
#include <vector>

#include "config/Exception.hpp"
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

        private:
            typedef void(Parser::*ServerDirectiveHandler)(ServerBlock&);
            typedef void(Parser::*LocationDirectiveHandler)(LocationBlock&);

            std::vector<Token> _tokens;
            std::size_t _pos;
            std::map<std::string, ServerDirectiveHandler> _serverHandler;
            std::map<std::string, LocationDirectiveHandler> _locationHandler;

            Parser();

            void initHandlers();

            LocationBlock parseLocationBlock();
            ServerBlock parseServerBlock();

            void parseServerDirective(ServerBlock& server);
            void parseLocationDirective(LocationBlock& location);

            void parseListen(ServerBlock& server);
            void parseClientHeaderBufferSize(ServerBlock& server);
            void parseClientMaxBodySize(ServerBlock& server);
            void parseErrorPage(ServerBlock& server);

            void parseRoot(LocationBlock& location);
            void parseIndex(LocationBlock& location);
            void parseAllowMethods(LocationBlock& location);
            void parseAutoindex(LocationBlock& location);
            void parseReturn(LocationBlock& location);
            void parseUploadEnable(LocationBlock& location);
            void parseUploadStore(LocationBlock& location);
            
            void validateServerBlock(const ServerBlock& server) const;
            void validateLocationBlock(const LocationBlock& location) const;

            bool isAtEnd() const;
            const Token& peek() const;
            const Token& advance();
            void expect(TokenType type, const std::string& errorMessage);

            const std::string consumeWord(const std::string& directiveName);

            void parseListenAddress(const std::string& value, ServerBlock& server) const;
            std::size_t parseSize(const std::string& value, const std::string& directiveName) const;
            int parseStatusCode(const std::string& value, const std::string& directiveName) const;
            void validatePath(const std::string& path, const std::string& directiveName, const bool mustBeURI) const;
            bool parseBoolean(const std::string& value, const std::string& directiveName) const;

            class SyntaxError : public Exception {
                public:
                    SyntaxError(const std::string& message);
                    ~SyntaxError() throw();
            };
    };
}

#endif // WEBSERV_CONFIG_PARSER_HPP
