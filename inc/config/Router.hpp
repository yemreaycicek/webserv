/**
 * @ Author: yaycicek
 * @ Create Time: 2026-07-04 / 18:04:40
 * @ Modified by: yaycicek
 * @ Modified time: 2026-07-04 / 19:01:09
 */

#ifndef WEBSERV_CONFIG_ROUTER_HPP
#define WEBSERV_CONFIG_ROUTER_HPP

#include <stdexcept>
#include <string>
#include <vector>

#include "config/Parser.hpp"

namespace config {
    class Router {
        public:
            Router(const std::vector<ServerBlock>& servers);
            Router(const Router& other);
            Router& operator=(const Router& other);
            ~Router();

            class DuplicateListenError : public std::runtime_error {
                public:
                    DuplicateListenError(const std::string& errorMessage);
            };

            class ServerNotFoundError : public std::runtime_error {
                public:
                    ServerNotFoundError(const std::string& errorMessage);
            };

            std::vector<std::string> getListenAddresses() const;
            bool hasServerBlock(const std::string& listenAddress) const;
            const ServerBlock& getServerBlock(const std::string& listenAddress) const;
            const std::vector<ServerBlock>& getServers() const;

        private:
            std::vector<ServerBlock> _servers;
            std::map<std::string, const ServerBlock*> _routerMap;

            Router();

            void buildMap();
    };
}

#endif // WEBSERV_CONFIG_ROUTER_HPP
