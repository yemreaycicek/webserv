/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-02 / 13:56:00
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-17 / 19:51:51
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
}

#endif // WEBSERV_UTILS_STR_HPP
