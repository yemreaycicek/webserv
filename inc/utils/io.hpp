/**
 * @ Author: yaycicek
 * @ Create Time: 2026-05-27 / 23:15:18
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-17 / 14:46:17
 */

#ifndef WEBSERV_UTILS_IO_HPP
#define WEBSERV_UTILS_IO_HPP

#include <string>

namespace io {
    void print(const std::string& message);
    void println(const std::string& message);
    void err(const std::string& message);
    void errln(const std::string& message);
    void newline();
    std::string input(const std::string& prompt);

    #ifdef DEBUG
        std::string padRight(const std::string& str, std::size_t width);
    #endif
}

#endif // WEBSERV_UTILS_IO_HPP
