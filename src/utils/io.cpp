/**
 * @ Author: yaycicek
 * @ Create Time: 2026-05-27 / 23:29:02
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-17 / 14:51:51
 */

#include "utils/io.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

#ifdef DEBUG
  #include <iomanip>
  #include <sstream>
#endif

void io::print(const std::string& message) {
    std::cout << message;
}

void io::println(const std::string& message) {
    std::cout << message << std::endl;
}

void io::err(const std::string& message) {
    std::cerr << message;
}

void io::errln(const std::string& message) {
    std::cerr << message << std::endl;
}

void io::newline() {
    std::cout << std::endl;
}

std::string io::input(const std::string& prompt) {
    std::string input;

    io::print(prompt);
    if (!std::getline(std::cin, input)) {
        throw std::runtime_error("Input stream closed or failed (EOF).");
    }
    return (input);
}

#ifdef DEBUG
    std::string io::padRight(const std::string& str, std::size_t width) {
        std::ostringstream oss;
        oss << std::left << std::setw(width) << str;
        return (oss.str());
    }
#endif
