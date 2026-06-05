/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-05 / 22:15:05
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-05 / 23:11:00
 */

#ifndef WEBSERV_CONFIG_CONFIG_NODES_HPP
#define WEBSERV_CONFIG_CONFIG_NODES_HPP

#include <string>
#include <vector>
#include <map>

namespace conf {
    struct LocationNode {
        std::string path;
        std::string root;
        std::string index;
        bool autoindex;
        std::vector<std::string> allowMethods;
        std::string redirect;
        bool upload_enable;
        std::string upload_store;

        LocationNode() : autoindex(false), upload_enable(false) {}
        
    };

    struct ServerNode {
        std::string listen;
        std::string clientMaxBodySize;
        std::map<std::size_t, std::string> errorPages;
        std::vector<LocationNode> locations;
    };
}

#endif // WEBSERV_CONFIG_CONFIG_NODES_HPP
