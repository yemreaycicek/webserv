/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-02 / 13:20:15
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-14 / 17:25:28
 */



#ifndef WEBSERV_EXEC_EXECUTOR_HPP
#define WEBSERV_EXEC_EXECUTOR_HPP

#include "config/Parser.hpp"
#include "http/Request.hpp"
#include "exec/Resolver.hpp"
#include "exec/ResponseBuilder.hpp"
#include "exec/Cgi.hpp"
#include <string>

namespace exec {
    enum PathType {
        PATH_NONE,
        PATH_FILE,
        PATH_DIR
    };

    // Outcome of trying to route a request to a CGI location as soon as its
    // headers are known (before the, possibly huge, body has fully arrived).
    enum CgiDispatch {
        CGI_NONE,   // not a CGI location: caller should fall back to normal buffered handling
        CGI_ERROR,  // a CGI location, but the request is invalid; outErrorResponse is ready to send
        CGI_START   // valid CGI request; outCgi is ready (empty/partial body) to spawn and stream into
    };

    class Executor {
        public:
            Executor();
            ~Executor();

            std::string     execute(const config::ServerBlock& sb, const http::Request& r);
            CgiDispatch     prepareCgi(const config::ServerBlock& sb, const http::Request& r, CgiInfo& outCgi, std::string& outErrorResponse);

        private:
            Executor(const Executor& other);
            Executor&       operator=(const Executor& other);

            PathType        getPathType(const std::string& path) const;
            std::string     handleGet(const config::ServerBlock& sb, const http::Request& r);
            std::string     readFile(const std::string& path) const;
            std::string     generateAutoindex(const std::string& fsPath, const std::string& uri) const;
            std::string     getContentType(const std::string& path) const;
            std::string     buildError(http::status::Code code, const config::ServerBlock& sb) const;
            std::string     handlePost(const config::ServerBlock& sb, const http::Request& r);
            std::string     handleDelete(const config::ServerBlock& sb, const http::Request& r);
            void            buildRequestData(const http::Request& r, RequestData& out) const;
            std::size_t     getMaxBodySize(const config::ServerBlock& sb, const config::LocationBlock* loc) const;
            bool            isCgiRequest(const ResolvedPath& rp) const;
            bool            isMethodAllowed(const config::LocationBlock* loc, const std::string& method) const;

            Resolver        _resolver;
            ResponseBuilder _responseBuilder;
    };
}

#endif // WEBSERV_EXEC_EXECUTOR_HPP 
