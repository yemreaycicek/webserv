/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-26 / 00:10:30
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-14 / 21:32:20
 */


#include "exec/Resolver.hpp"

namespace exec {
    Resolver::Resolver() {}

    Resolver::~Resolver() {}

    const config::LocationBlock* Resolver::matchLocation(const config::ServerBlock& sb, const std::string& uri) const {
        size_t max = 0;
        const config::LocationBlock* res = NULL;
        for (std::vector<config::LocationBlock>::const_iterator it = sb.locations.begin(); it != sb.locations.end(); ++it){
            std::string normPath = it->path;
            if (normPath.length() > 1 && normPath[normPath.length() - 1] == '/') normPath = normPath.substr(0, normPath.length() - 1);
            if (uri.compare(0, it->path.length(), it->path) == 0) {
                if ((it->path == "/" || uri.length() == it->path.length() || uri[it->path.length()] == '/') && max < it->path.length()) {
                    max = it->path.length();
                    res = &(*it);
                }
            }
        }
        return res;
    }

    std::string Resolver::buildFsPath(const config::LocationBlock& loc, const std::string& uri) const {
        std::string remainder = uri.substr(loc.path.size());
        std::string root = loc.root;
        if (!remainder.empty() && !root.empty()) {
            if (remainder[0] != '/' && root[root.size() - 1] != '/') {
                remainder = "/" + remainder;
            } else if (remainder[0] == '/' && root[root.size() - 1] == '/') {
                root.erase(root.size() - 1);
            }
        }
        return (root + remainder);
    }

    ResolvedPath Resolver::resolve(const config::ServerBlock& sb, const std::string& uri) {
        ResolvedPath resolvePath;
        resolvePath.location = matchLocation(sb, uri);
        if (resolvePath.location) resolvePath.fsPath= buildFsPath(*resolvePath.location, uri);
        return (resolvePath);
    }
}