/**
 * @ Author: yaycicek
 * @ Create Time: 2026-05-27 / 23:29:02
 * @ Modified by: yaycicek
 * @ Modified time: 2026-05-28 / 00:29:37
 */

#include "utils/io.hpp"

#include <iostream>
#include <stdexcept>

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

std::string io::input(const std::string& prompt) {
    std::string input;

    io::print(prompt);
    if (!std::getline(std::cin, input)) {
        throw std::runtime_error("Input stream closed or failed (EOF).");
    }
    return (input);
}
