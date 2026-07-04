/**
 * @ Author: yaycicek
 * @ Create Time: 2026-07-04 / 18:20:09
 * @ Modified by: yaycicek
 * @ Modified time: 2026-07-04 / 18:31:29
 */

#include "config/Router.hpp"

#include <string>
#include <vector>

namespace config {
    Router::Router() {}
    Router::Router(const std::vector<ServerBlock>& servers) : _servers(servers) {}
    Router::Router(const Router& other) : _servers(other._servers), _routerMap(other._routerMap) {}

    Router& Router::operator=(const Router& other) {
        if (this != &other) {
            _servers = other._servers;
            _routerMap = other._routerMap;
        }
        return ((*this));
    }

    Router::~Router() {}
}
