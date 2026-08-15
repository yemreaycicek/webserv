/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-26 / 00:10:30
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-15 / 13:11:52
 */


#include "exec/Resolver.hpp"
#include <sstream>
#include <vector>

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

        std::string Resolver::normalizePath(const std::string& path) const {
            std::vector<std::string> stack;
            std::stringstream ss(path);
            std::string segment;
            while (std::getline(ss, segment, '/')) {
                if (segment.empty() || segment == ".")
                    continue;
                if (segment == "..") {
                    if (!stack.empty())
                        stack.pop_back();
                    continue;
                }
                stack.push_back(segment);
            }

            std::string result = "/";
            for (size_t i = 0; i < stack.size(); ++i) {
                result += stack[i];
                if (i + 1 < stack.size())
                    result += "/";
            }
            return (result);
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
        std::string path = uri;
        std::string::size_type q = path.find('?');
        if (q != std::string::npos) path = path.substr(0, q);
        path = normalizePath(path);
        resolvePath.location = matchLocation(sb, path);
        if (resolvePath.location) resolvePath.fsPath= buildFsPath(*resolvePath.location, path);
        return (resolvePath);
    }
}