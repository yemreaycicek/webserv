/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-02 / 13:20:15
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-02 / 13:58:28
 */



#ifndef WEBSERV_EXEC_EXECUTOR_HPP
#define WEBSERV_EXEC_EXECUTOR_HPP

#include "config/Parser.hpp"
#include "http/Request.hpp"
#include "exec/Resolver.hpp"
#include "exec/ResponseBuilder.hpp"
#include <string>

namespace exec {
    class Executor {
        public:
            Executor();
            ~Executor();

            std::string     execute(const config::ServerBlock& sb, const http::Request& r);
            
        private:
            Executor(const Executor& other);
            Executor&       operator=(const Executor& other);

            std::string     handleGet(const config::ServerBlock& sb, const http::Request& r);
            std::string     readFile(const std::string& path) const;

            Resolver        _resolver;
            ResponseBuilder _responseBuilder;
    };
}

#endif // WEBSERV_EXEC_EXECUTOR_HPP 
