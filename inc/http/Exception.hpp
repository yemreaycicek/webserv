/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-24 / 15:41:39
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-25 / 13:47:59
 */

#ifndef WEBSERV_HTTP_EXCEPTION_HPP
#define WEBSERV_HTTP_EXCEPTION_HPP

#include <stdexcept>
#include <string>

#include "http/Status.hpp"

namespace http {
    class Exception : public std::runtime_error {
        public:
            Exception(status::Code statusCode, const std::string& detail);
            ~Exception() throw();

            status::Code getStatusCode() const;

        private:
            status::Code _statusCode;
    };
}

#endif // WEBSERV_HTTP_EXCEPTION_HPP
