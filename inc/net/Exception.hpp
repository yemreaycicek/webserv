/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-21 / 11:05:07
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-21 / 11:14:24
 */

#ifndef WEBSERV_NET_EXCEPTION_HPP
#define WEBSERV_NET_EXCEPTION_HPP

#include <stdexcept>
#include <string>

namespace net {
    class Exception : public std::runtime_error {
        public:
            Exception(const std::string& message);
            ~Exception() throw();
    };
}

#endif // WEBSERV_NET_EXCEPTION_HPP
