/**
 * @ Author: yaycicek
 * @ Create Time: 2026-05-27 / 23:29:02
 * @ Modified by: yaycicek
 * @ Modified time: 2026-09-07 / 18:51:01
 */

#include "utils/io.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

#ifdef DEBUG
  #include <iomanip>
  #include <sstream>
#endif

void io::println(const std::string& message) {
    std::cout << message << std::endl;
}

void io::errln(const std::string& message) {
    std::cerr << message << std::endl;
}

#ifdef DEBUG
    std::string io::padRight(const std::string& str, std::size_t width) {
        std::ostringstream oss;
        oss << std::left << std::setw(width) << str;
        return (oss.str());
    }
#endif
