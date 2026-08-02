/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-02 / 14:05:15
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-02 / 23:23:55
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

    std::string Executor::buildError(http::status::Code code, const config::ServerBlock& sb) const {
        std::map<std::size_t, std::string>::const_iterator it = sb.errorPages.find(code);
        if (it != sb.errorPages.end()) {
            std::string path = it->second;
            if (!path.empty() && path[0] == '/') path = "." + path;
            std::string content = readFile(path);
            if (!content.empty()) return _responseBuilder.build(code, content, "text/html");
        }
        std::string body = "<html><body><h1>" + str::to_string(code) + " "
                         + http::status::getReasonPhrase(code) + "</h1></body></html>";
        return _responseBuilder.build(code, body, "text/html");
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
            return (buildError(http::status::NOT_FOUND, sb));
        }

        const std::vector<std::string>& methods = rp.location->allowMethods;
        for (std::vector<std::string>::const_iterator it = methods.begin(); it != methods.end(); ++it) {
            if (*it == "GET") {
                PathType type = getPathType(rp.fsPath);
                if (type == PATH_NONE) return (buildError(http::status::NOT_FOUND, sb));
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
                        if (!listing.empty()) return (_responseBuilder.build(http::status::OK, listing, "text/html"));
                    }
                    return (buildError(http::status::FORBIDDEN, sb));
                }
                std::string content = readFile(rp.fsPath);
                if (content.empty()) return (buildError(http::status::NOT_FOUND, sb));
                return (_responseBuilder.build(http::status::OK, content, getContentType(rp.fsPath)));
            }
        }
        return (buildError(http::status::METHOD_NOT_ALLOWED, sb));
    }

    std::string Executor::handlePost(const config::ServerBlock& sb, const http::Request& r) {
        exec::ResolvedPath rp = _resolver.resolve(sb, r.getUri());
        if (rp.location == NULL) return (buildError(http::status::NOT_FOUND, sb));
        const std::vector<std::string>& methods = rp.location->allowMethods;
        for (std::vector<std::string>::const_iterator it = methods.begin(); it != methods.end(); ++it) {
            if (*it == "POST") {
                if (r.getBody().size() > sb.clientMaxBodySize) return (buildError(http::status::CONTENT_TOO_LARGE, sb));
                if (!rp.location->uploadEnable) return (buildError(http::status::FORBIDDEN, sb));

                std::string uri = r.getUri();
                std::string fileName = uri.substr(uri.rfind('/') + 1);
                std::string us = rp.location->uploadStore;
                if (!us.empty() && us[us.size() - 1] != '/') us += '/';
                us += fileName;
                std::ofstream file(us.c_str());
                if (!file.is_open()) return (buildError(http::status::INTERNAL_SERVER_ERROR, sb));
                file << r.getBody();
                file.close();
                return _responseBuilder.build(http::status::CREATED, "<html><body>Dosya yuklendi</body></html>", "text/html");
            }
        }
        return (buildError(http::status::METHOD_NOT_ALLOWED, sb));
    }


    std::string Executor::execute(const config::ServerBlock& sb, const http::Request& r) {
        http::Method m = r.getMethod();
        if (m == http::GET) {
            return (handleGet(sb, r));
        }
        if (m == http::POST) {
            return (handlePost(sb, r));
        }
        return (buildError(http::status::METHOD_NOT_ALLOWED, sb));
    }
}