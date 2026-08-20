/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-02 / 14:05:15
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-15 / 15:38:46
 */

#include "exec/Executor.hpp"
#include "http/RequestLine.hpp"
#include "exec/Cgi.hpp"
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <cctype>
#include <unistd.h>
#include <cstdio>

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
                    // No index was ever configured for this location and
                    // listing is off: the directory itself exists, we just
                    // refuse to show it — 403 Forbidden.
                    // An index *was* configured but this particular
                    // directory doesn't have that file: that's the more
                    // ordinary "the thing you asked for isn't here" — 404.
                    if (indexName.empty()) return (buildError(http::status::FORBIDDEN, sb));
                    return (buildError(http::status::NOT_FOUND, sb));
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
                if (r.getBody().size() > getMaxBodySize(sb, rp.location)) return (buildError(http::status::CONTENT_TOO_LARGE, sb));
                // POST itself is allowed here, but uploading specifically
                // isn't turned on for this location — that's 403 Forbidden,
                // not 404 (the location does exist and does handle POST).
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

    std::string Executor::handleDelete(const config::ServerBlock& sb, const http::Request& r) {
        exec::ResolvedPath rp = _resolver.resolve(sb, r.getUri());
        if (rp.location == NULL) return (buildError(http::status::NOT_FOUND, sb));
        const std::vector<std::string>& methods = rp.location->allowMethods;
        for (std::vector<std::string>::const_iterator it = methods.begin(); it != methods.end(); ++it) {
            if (*it == "DELETE") {
                if (!getPathType(rp.fsPath)) return (buildError(http::status::NOT_FOUND, sb));
                if (remove(rp.fsPath.c_str()) != 0) return buildError(http::status::INTERNAL_SERVER_ERROR, sb);
                return _responseBuilder.build(http::status::OK, "<html><body>Silindi</body></html>", "text/html");
            }
        }
        return (buildError(http::status::METHOD_NOT_ALLOWED, sb));
    }

    RequestData Executor::buildRequestData(const http::Request& r) const {
        RequestData rd;
        http::Method m = r.getMethod();
        switch(m) {
            case http::GET:
                rd.method = "GET";
                break;
            case http::POST:
                rd.method = "POST";
                break;
            case http::DELETE:
                rd.method = "DELETE";
                break;
            default:
                rd.method = "";
                break;
        }

        std::string uri = r.getUri();
        std::string::size_type pos = uri.find('?');
        if (pos == std::string::npos) {
            rd.path = uri;
            rd.query = "";
        }
        else {
            rd.path = uri.substr(0, pos);
            rd.query = uri.substr(pos + 1);
        }
        rd.body = r.getBody();
        rd.contentLength = r.getContentLength();
        rd.headers = r.getHeaders();
        return (rd);
    }

    bool Executor::isCgiRequest(const ResolvedPath& rp) const {
        if (rp.location == NULL) return (false);
        if (rp.location->cgiExtension.empty()) return (false);
        std::string cgiExtension = rp.location->cgiExtension;
        bool endsWith = rp.fsPath.size() >= cgiExtension.size() && rp.fsPath.compare(rp.fsPath.size() - cgiExtension.size(), cgiExtension.size(), cgiExtension) == 0;
        return (endsWith);
    }

    std::size_t Executor::getMaxBodySize(const config::ServerBlock& sb, const config::LocationBlock* loc) const {
        if (loc != NULL && loc->clientMaxBodySize != config::UNSET_BODY_SIZE) {
            return (loc->clientMaxBodySize);
        }
        return (sb.clientMaxBodySize);
    }

    bool Executor::isMethodAllowed(const config::LocationBlock* loc, const std::string& method) const {
        if (method.empty()) return (false);
        const std::vector<std::string>& methods = loc->allowMethods;
        for (std::vector<std::string>::const_iterator it = methods.begin(); it != methods.end(); ++it) {
            if (*it == method) return (true);
        }
        return (false);
    }

    // Called as soon as a request's headers are ready, before its (possibly
    // large) body has fully arrived, so a CGI location can start receiving the
    // body as it streams in rather than after the whole thing is buffered.
    CgiDispatch Executor::prepareCgi(const config::ServerBlock& sb, const http::Request& r, CgiInfo& outCgi, std::string& outErrorResponse) {
        outCgi.isCgi = false;
        std::string uri = r.getUri();
        std::string::size_type qpos = uri.find('?');
        std::string pathOnly = (qpos == std::string::npos) ? uri : uri.substr(0, qpos);
        exec::ResolvedPath rp = _resolver.resolve(sb, pathOnly);
        if (rp.location != NULL && rp.location->redirect.isSet()) return (CGI_NONE);
        if (!isCgiRequest(rp)) return (CGI_NONE);

        RequestData reqData = buildRequestData(r);
        if (!isMethodAllowed(rp.location, reqData.method)) {
            outErrorResponse = buildError(http::status::METHOD_NOT_ALLOWED, sb);
            return (CGI_ERROR);
        }
        // Deliberately NOT checking whether rp.fsPath actually exists here:
        // that would mean deciding on (and closing out) the request before
        // its body has necessarily started arriving, which is exactly the
        // early-response-then-close-early-with-body-still-incoming situation
        // that risks the OS sending RST instead of a clean FIN once we do
        // close — some HTTP clients then report a connection error instead
        // of the response we actually sent. Letting a missing script run the
        // normal course (spawn attempted, execve() fails in the child, the
        // parent's write to its now-dead stdin pipe gets EPIPE) produces a
        // 502 through the ordinary Cgi FAILED path instead, which naturally
        // only resolves once the CGI dispatch (and so the response) is under
        // way in lockstep with the body, not decided upfront.
        if (r.getMethod() == http::POST && r.getContentLength() > getMaxBodySize(sb, rp.location)) {
            outErrorResponse = buildError(http::status::CONTENT_TOO_LARGE, sb);
            return (CGI_ERROR);
        }
        outCgi.isCgi = true;
        outCgi.interpreter = rp.location->cgiPass;
        outCgi.scriptPath  = rp.fsPath;
        outCgi.reqData = reqData;
        return (CGI_START);
    }

    std::string Executor::execute(const config::ServerBlock& sb, const http::Request& r) {
        std::string uri = r.getUri();
        std::string::size_type qpos = uri.find('?');
        std::string pathOnly = (qpos == std::string::npos) ? uri : uri.substr(0, qpos);
        exec::ResolvedPath rp = _resolver.resolve(sb, pathOnly);
        if (rp.location != NULL && rp.location->redirect.isSet()) {
            return _responseBuilder.buildRedirect(static_cast<http::status::Code>(rp.location->redirect.code), rp.location->redirect.target);
        }
        // CGI locations are dispatched earlier via prepareCgi() (see Server),
        // as soon as headers are ready, so by the time execute() runs, the
        // request is already known not to be one.

        http::Method m = r.getMethod();
        if (m == http::GET) {
            return (handleGet(sb, r));
        }
        if (m == http::HEAD) {
            if (rp.location == NULL || !isMethodAllowed(rp.location, "HEAD"))
                return (buildError(http::status::METHOD_NOT_ALLOWED, sb));
            std::string res = handleGet(sb, r);
            std::string::size_type pos = res.find("\r\n\r\n");
            if (pos != std::string::npos) res.erase(pos + 4);
            return (res);
        }
        if (m == http::POST) {
            return (handlePost(sb, r));
        }
        if (m == http::DELETE) {
            return (handleDelete(sb, r));
        }
        return (buildError(http::status::METHOD_NOT_ALLOWED, sb));
    }
}
