/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-17 / 14:02:31
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-20 / 15:39:00
 */

#include "config/Debug.hpp"

#include <vector>

#include "utils/io.hpp"
#include "utils/str.hpp"

namespace config {
    void Debug::printLexer(const std::vector<config::Token>& tokens) {
        for (std::vector<config::Token>::const_iterator it = tokens.begin(); it != tokens.end(); it++) {
            io::println(io::padRight(getTokenType(it->type), 13) + " → '" + it->value + "'");
        }
    }

    void Debug::printParser(const std::vector<config::ServerBlock>& servers) {
        for (std::size_t i = 0; i < servers.size(); i++) {
            printServerBlock(servers.at(i), i);
        }
        
    }

    void Debug::printRouter(const config::Router& router) {
        std::vector<std::string> addresses = router.getListenAddresses();
        const std::vector<config::ServerBlock>& servers = router.getServers();

        io::println("Router Table [" + str::to_string(addresses.size()) + " Socket(s)]");

        for (std::size_t i = 0; i < addresses.size(); ++i) {
            bool isLastSocket = (i == addresses.size() - 1);
            std::string branch = isLastSocket ? "└── " : "├── ";
            std::string indent = isLastSocket ? "    " : "│   ";

            const std::string& addr = addresses[i];
            const config::ServerBlock& target = router.getServerBlock(addr);

            std::size_t serverIndex = 0;
            for (std::size_t s = 0; s < servers.size(); ++s) {
                if (&target == &servers[s]) {
                    serverIndex = s + 1;
                    break;
                }
            }

            io::println(branch + "Socket: " + addr);
            io::println(indent + "├── Routed to: Server [" + str::to_string(serverIndex) + "]");

            std::string locSummary = "";
            for (std::size_t l = 0; l < target.locations.size(); ++l) {
                locSummary += "'" + target.locations[l].path + "'";
                if (l + 1 < target.locations.size()) {
                    locSummary += ", ";
                }
            }
            io::println(indent + "└── Locations (" + str::to_string(target.locations.size()) + "): " + locSummary);
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

    std::string Debug::joinMethods(const std::vector<std::string>& methods) {
        std::string ret;
        for (std::size_t i = 0; i < methods.size(); i++) {
            ret += methods.at(i);
            if (i < methods.size() - 1) {
                ret += ", ";
            }
        }
        return (ret);
    }

    void Debug::printServerBlock(const config::ServerBlock& server, std::size_t index) {
        io::println("Server [" + str::to_string(index + 1) + "]");

        std::vector<std::string> serverLines;
        for (std::size_t i = 0; i < server.listen.size(); i++) {
            serverLines.push_back("listen: " + server.listen.at(i));
        }
        if (server.clientMaxBodySize != 0) {
            serverLines.push_back("client_max_body_size: " + str::to_string(server.clientMaxBodySize));
        }

        for (size_t i = 0; i < serverLines.size(); i++) {
            bool isAbsolutelyLast = (server.errorPages.empty() && server.locations.empty() && i == serverLines.size() - 1);
            io::println((isAbsolutelyLast ? "└── " : "├── ") + serverLines.at(i));
        }

        if (!server.errorPages.empty()) {
            bool hasLocations = server.locations.empty();
            io::println((hasLocations ? "└── " : "├── ") + std::string("error_pages: "));
            std::string errorPrefix = (hasLocations ? "    " : "│   ");

            std::size_t errorCount = 0;
            for (std::map<std::size_t, std::string>::const_iterator it = server.errorPages.begin(); it != server.errorPages.end(); it++, errorCount++) {
                bool isLastError = (errorCount == server.errorPages.size() - 1);
                io::println(errorPrefix + (isLastError ? "└── " : "├── ") + str::to_string(it->first) + " → " + it->second);   
            }
        }
        
        for (std::size_t i = 0; i < server.locations.size(); i++) {
            bool isLastLocation = (i == server.locations.size() - 1);
            printLocationBlock(server.locations.at(i), isLastLocation);
        }
    }

    void Debug::printLocationBlock(const config::LocationBlock& location, bool isLastLocation) {
        std::string prefix = (isLastLocation ? "    " : "│   ");

        io::println((isLastLocation ? "└── " : "├── ") + std::string("Location: '") + location.path + std::string("'"));

        std::vector<std::string> locationLines;
        if (!location.root.empty()) {
            locationLines.push_back("root: " + location.root);
        }
        if (!location.index.empty()) {
            locationLines.push_back("index: " + location.index);
        }
        if (location.autoindex) {
            locationLines.push_back(std::string("autoindex: ") + (location.autoindex ? "on" : "off"));
        }
        if (location.redirect.isSet()) {
            locationLines.push_back("return: " + str::to_string(location.redirect.code) + " " + location.redirect.target);
        }
        if (location.uploadEnable) {
            locationLines.push_back("upload_enable: on");
            if (!location.uploadStore.empty()) {
                locationLines.push_back("upload_store: " + location.uploadStore);
            }
        }
        if (!location.allowMethods.empty()) {
            locationLines.push_back("allow_methods: " + joinMethods(location.allowMethods));
        }
        if (location.clientMaxBodySize != config::UNSET_BODY_SIZE) {
            locationLines.push_back("client_max_body_size: " + str::to_string(location.clientMaxBodySize));
        }

        for (std::size_t i = 0; i < locationLines.size(); i++) {
            bool isLastLine = (i == locationLines.size() - 1);
            io::println(prefix + (isLastLine ? "└── " : "├── ") + locationLines.at(i));
        }
    }
}
