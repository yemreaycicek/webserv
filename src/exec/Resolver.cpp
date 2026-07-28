/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-26 / 00:10:30
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-28 / 18:50:57
 */


#include "exec/Resolver.hpp"

namespace exec {
    Resolver::Resolver() {}

    Resolver::~Resolver() {}

    const config::LocationBlock* Resolver::matchLocation(const config::ServerBlock& sb, const std::string& uri) const {
        size_t max = 0;
        const config::LocationBlock* res = NULL;
        for (std::vector<config::LocationBlock>::const_iterator it = sb.locations.begin(); it != sb.locations.end(); ++it){
            if (uri.compare(0, it->path.length(), it->path) == 0) {
                if ((it->path == "/" || uri.length() == it->path.length() || uri[it->path.length()] == '/') && max < it->path.length()) {
                    max = it->path.length();
                    res = &(*it);
                }
            }
        }
        return res;
    }

    ResolvedPath Resolver::resolve(const config::ServerBlock& sb, const std::string& uri) {
        ResolvedPath resolvePath;
        resolvePath.location = matchLocation(sb, uri);
        
        
        
    }

}