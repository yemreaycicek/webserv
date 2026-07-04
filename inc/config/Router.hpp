/**
 * @ Author: yaycicek
 * @ Create Time: 2026-07-04 / 18:04:40
 * @ Modified by: yaycicek
 * @ Modified time: 2026-07-04 / 18:31:19
 */

#ifndef WEBSERV_CONFIG_ROUTER_HPP
#define WEBSERV_CONFIG_ROUTER_HPP

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

        private:
            std::vector<ServerBlock> _servers;
            std::map<std::string, const ServerBlock*> _routerMap;

            Router();
    };
}

#endif // WEBSERV_CONFIG_ROUTER_HPP
