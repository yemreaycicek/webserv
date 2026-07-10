/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-02 / 13:56:00
 * @ Modified by: yaycicek
 * @ Modified time: 2026-07-10 / 21:49:50
 */

#ifndef WEBSERV_UTILS_STR_HPP
#define WEBSERV_UTILS_STR_HPP

#include <sstream>
#include <string>

namespace str {
    std::string trim(const std::string& value);

    template <typename T>
    std::string to_string(T value) {
        std::ostringstream oss;
        oss << value;
        return (oss.str());
    }

    template <typename T>
    bool to_numeric(const std::string& value, T& result) {
        std::istringstream iss(value);
        iss >> result;

        return (!iss.fail() && iss.eof());
    }

    std::string tolower(const std::string& value);
}

#endif // WEBSERV_UTILS_STR_HPP
