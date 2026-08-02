/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-02 / 14:05:15
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-02 / 15:32:43
 */

#include "exec/Executor.hpp"
#include "http/RequestLine.hpp"
#include <vector>
#include <fstream>
#include <sstream>

namespace exec {
    Executor::Executor() {}
    Executor::~Executor() {}

    std::string Executor::readFile(const std::string& path) const {
        std::ifstream file(path.c_str());
        if (!file.is_open()) return "";
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    std::string Executor::handleGet(const config::ServerBlock& sb, const http::Request& r) {
        exec::ResolvedPath rp = _resolver.resolve(sb, r.getUri());
        if (rp.location == NULL) {
            return (_responseBuilder.build(http::status::NOT_FOUND, "<html>404</html>", "text/html"));
        }
        const std::vector<std::string>& methods = rp.location->allowMethods;
        for (std::vector<std::string>::const_iterator it = methods.begin(); it != methods.end(); ++it) {
            if (*it == "GET") {
                std::string content = readFile(rp.fsPath);
                if (content.empty()) return (_responseBuilder.build(http::status::NOT_FOUND, "<html>404</html>", "text/html"));
                return (_responseBuilder.build(http::status::OK, content, "text/html"));
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