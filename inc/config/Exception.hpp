#ifndef WEBSERV_CONFIG_EXCEPTION_HPP
#define WEBSERV_CONFIG_EXCEPTION_HPP

#include <stdexcept>
#include <string>

namespace config {
    class Exception : public std::runtime_error {
        public:
            Exception(const std::string& message);
            ~Exception() throw();
    };
}

#endif // WEBSERV_CONFIG_EXCEPTION_HPP
