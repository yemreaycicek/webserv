/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-02 / 14:05:15
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-02 / 20:35:07
 */

#include "exec/Executor.hpp"
#include "http/RequestLine.hpp"
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <cctype>

namespace exec {
    Executor::Executor() {}
    Executor::~Executor() {}

    exec::PathType Executor::getPathType(const std::string& path) const {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return PATH_NONE;
        if (S_ISDIR(st.st_mode)) return PATH_DIR;
        if (S_ISREG(st.st_mode)) return PATH_FILE;
        return PATH_NONE;
    }
    
    std::string Executor::readFile(const std::string& path) const {
        std::ifstream file(path.c_str());
        if (!file.is_open()) return "";
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    std::string Executor::getContentType(const std::string& path) const {
        std::size_t pos = path.rfind('.');
        if (pos == std::string::npos) return "application/octet-stream";
        std::string cType = path.substr(pos+1);
        for (size_t i = 0; i < cType.size(); ++i) cType[i] = std::tolower(cType[i]);
        if (cType == "html" || cType == "htm")  return "text/html";
        if (cType == "txt")                     return "text/plain";
        if (cType == "css")                     return "text/css";
        if (cType == "js")                      return "application/javascript";
        if (cType == "jpg" || cType == "jpeg")  return "image/jpeg";
        if (cType == "png")                     return "image/png";
        if (cType == "gif")                     return "image/gif";
        return "application/octet-stream";
    }

    std::string Executor::generateAutoindex(const std::string& fsPath, const std::string& uri) const {
        DIR* dir = opendir(fsPath.c_str());
        if (dir == NULL) return ("");
        std::string base = uri;
        if (base.empty() || base[base.size() - 1] != '/') base += '/';
        std::stringstream body;
        body << "<html><head><title>Index of " << uri << "</title></head><body>";
        body << "<h1>Index of " << uri << "</h1>";
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            body << "<a href=\"" << base << name << "\">" << name << "</a><br>";
        }
        body << "</body></html>";
        closedir(dir);
        return (body.str());
    }

    std::string Executor::handleGet(const config::ServerBlock& sb, const http::Request& r) {
        exec::ResolvedPath rp = _resolver.resolve(sb, r.getUri());
        if (rp.location == NULL) {
            return (_responseBuilder.build(http::status::NOT_FOUND, "<html>404</html>", "text/html"));
        }

        const std::vector<std::string>& methods = rp.location->allowMethods;
        for (std::vector<std::string>::const_iterator it = methods.begin(); it != methods.end(); ++it) {
            if (*it == "GET") {
                PathType type = getPathType(rp.fsPath);
                if (type == PATH_NONE) return (_responseBuilder.build(http::status::NOT_FOUND, "<html>404</html>", "text/html"));
                if (type == PATH_DIR) {
                    std::string indexName = rp.location->index;
                    if (!indexName.empty()) {
                        std::string indexPath = rp.fsPath;
                        if (indexPath[indexPath.size() - 1] != '/') indexPath += '/';
                        indexPath += indexName;
                        if (getPathType(indexPath) == PATH_FILE) {
                            std::string content = readFile(indexPath);
                            return _responseBuilder.build(http::status::OK, content, getContentType(indexPath));
                        }
                    }
                    if (rp.location->autoindex) {
                        std::string listing = generateAutoindex(rp.fsPath, r.getUri());
                        if (!listing.empty()) return (_responseBuilder.build(http::status::OK, listing, getContentType(rp.fsPath)));
                    }
                    return (_responseBuilder.build(http::status::FORBIDDEN, "<html>403</html>", "text/html"));
                }
                std::string content = readFile(rp.fsPath);
                if (content.empty()) return (_responseBuilder.build(http::status::NOT_FOUND, "<html>404</html>", "text/html"));
                return (_responseBuilder.build(http::status::OK, content, getContentType(rp.fsPath)));
            }
        }
        return (_responseBuilder.build(http::status::METHOD_NOT_ALLOWED, "<html>405</html>", "text/html"));
    }
    
    std::string Executor::execute(const config::ServerBlock& sb, const http::Request& r) {
        http::Method m = r.getMethod();
        if (m == http::GET) {
            return (handleGet(sb, r));
        }
        return (_responseBuilder.build(http::status::METHOD_NOT_ALLOWED, "<html>405</html>", "text/html"));
    }
}