/**
 * @ Author: yaycicek
 * @ Create Time: 2026-05-27 / 23:15:18
 * @ Modified by: yaycicek
 * @ Modified time: 2026-09-07 / 18:51:12
 */

#ifndef WEBSERV_UTILS_IO_HPP
#define WEBSERV_UTILS_IO_HPP

#include <string>

namespace io {
    void println(const std::string& message);
    void errln(const std::string& message);

    #ifdef DEBUG
        std::string padRight(const std::string& str, std::size_t width);
    #endif
}

#endif // WEBSERV_UTILS_IO_HPP
