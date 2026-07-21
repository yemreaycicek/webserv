/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 00:36:18
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-22 / 00:38:37
 */

#ifndef WEBSERV_EXEC_EXCEPTION_HPP
#define WEBSERV_EXEC_EXCEPTION_HPP

#include <stdexcept>
#include <string>

namespace exec {
    class Exception {
        public:
            Exception(const std::string& message);
            ~Exception() throw();
    };
}

#endif // WEBSERV_EXEC_EXCEPTION_HPP 
