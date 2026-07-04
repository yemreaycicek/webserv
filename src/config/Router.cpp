/**
 * @ Author: yaycicek
 * @ Create Time: 2026-07-04 / 18:20:09
 * @ Modified by: yaycicek
 * @ Modified time: 2026-07-04 / 18:55:35
 */

#include "config/Router.hpp"

#include <string>
#include <vector>

namespace config {
    Router::Router() {
        buildMap();
    }
    Router::Router(const std::vector<ServerBlock>& servers) : _servers(servers) {
        buildMap();
    }
    Router::Router(const Router& other) : _servers(other._servers) {
        buildMap();
    }

    Router& Router::operator=(const Router& other) {
        if (this != &other) {
            _servers = other._servers;
            buildMap();
        }
        return ((*this));
    }

    Router::~Router() {}

    void Router::buildMap() {
        for (std::size_t i = 0; i < _servers.size(); i++) {
            for (std::size_t j = 0; j < _servers.at(i).listen.size(); j++) {
                const std::string& addr = _servers.at(i).listen.at(j);

                if (_routerMap.find(addr) != _routerMap.end()) {
                    throw DuplicateListenError("Virtual Hosting is disabled! Duplicate listen address found: '" + addr + "'");
                } else {
                    _routerMap[addr] = &(_servers.at(i));
                }
            }
            
        }
        
    }

    Router::DuplicateListenError::DuplicateListenError(const std::string& errorMessage) : std::runtime_error(errorMessage) {}
}
