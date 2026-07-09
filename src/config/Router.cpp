/**
 * @ Author: yaycicek
 * @ Create Time: 2026-07-04 / 18:20:09
 * @ Modified by: yaycicek
 * @ Modified time: 2026-07-09 / 14:47:17
 */

#include "config/Router.hpp"

#include <string>
#include <vector>

#include "config/Exception.hpp"
#include "config/Parser.hpp"

#ifdef DEBUG
  #include "config/Debug.hpp"
#endif

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

        try {
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
        } catch (...) {
            #ifdef DEBUG
                config::Debug::printRouter((*this));
            #endif
            throw;
        }
        #ifdef DEBUG
            config::Debug::printRouter((*this));
        #endif
    }

    std::vector<std::string> Router::getListenAddresses() const {
        std::vector<std::string> addresses;

        for (std::map<std::string, const ServerBlock*>::const_iterator it = _routerMap.begin(); it != _routerMap.end(); it++) {
            addresses.push_back(it->first);
        }
        return (addresses);
    }

    bool Router::hasServerBlock(const std::string& listenAddress) const {
        return (_routerMap.find(listenAddress) != _routerMap.end());
    }

    const ServerBlock& Router::getServerBlock(const std::string& listenAddress) const {
        std::map<std::string, const ServerBlock*>::const_iterator it = _routerMap.find(listenAddress);

        if (it == _routerMap.end()) {
            throw ServerNotFoundError("No server block configured for listen address: '" + listenAddress + "'");
        }
        return (*(it->second));
    }

    const std::vector<ServerBlock>& Router::getServers() const {
        return (_servers);
    }

    Router::DuplicateListenError::DuplicateListenError(const std::string& message) : Exception(message) {}
    Router::DuplicateListenError::~DuplicateListenError() throw() {}
    Router::ServerNotFoundError::ServerNotFoundError(const std::string& message) : Exception(message) {}
    Router::ServerNotFoundError::~ServerNotFoundError() throw() {}
}
