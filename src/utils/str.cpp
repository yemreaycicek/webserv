/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-02 / 14:08:32
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-23 / 17:00:47
 */

#include "utils/str.hpp"

#include <string> 

std::string str::trim(const std::string& value) {
    static const char* whitespace = " \t\n\r\v\f";
    std::size_t start = value.find_first_not_of(whitespace);
    if (start == std::string::npos) return ("");
    std::size_t end = value.find_last_not_of(whitespace);
    return (value.substr(start, (end - start + 1)));
}

std::string str::tolower(const std::string& value) {
    std::string ret = value;
    for (std::size_t i = 0; i < value.size(); i++) {
        ret.at(i) = std::tolower(static_cast<unsigned char>(value.at(i))); 
    }
    return (ret);
}
