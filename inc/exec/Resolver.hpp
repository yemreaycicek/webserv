/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-26 / 00:10:42
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-15 / 12:45:49
 */

#ifndef WEBSERV_EXEC_RESOLVER_HPP
#define WEBSERV_EXEC_RESOLVER_HPP

#include "config/Parser.hpp"
#include <string>


namespace exec {
    struct ResolvedPath {
        const config::LocationBlock*    location;
        std::string                     fsPath;
    };

    class Resolver {
        public:
            Resolver();
            ~Resolver();

            ResolvedPath                    resolve(const config::ServerBlock& sb, const std::string& uri);
        private:
            const config::LocationBlock*    matchLocation(const config::ServerBlock& sb, const std::string& uri) const;
            std::string                     buildFsPath(const config::LocationBlock& loc, const std::string& uri) const;
            std::string                     normalizePath(const std::string& path) const;
    };
}

#endif // WEBSERV_EXEC_RESOLVER_HPP 
