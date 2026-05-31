/**
 * @ Author: yaycicek
 * @ Create Time: 2026-05-27 / 22:18:27
 * @ Modified by: yaycicek
 * @ Modified time: 2026-05-31 / 20:44:23
 */

#include <cstdlib>
#include <string>

#include "utils/io.hpp"

void printUsage() {
    io::errln("Usage: ./bin/webserv [CONFIGURATION_FILE]...");
    io::errln("Try './bin/webserv conf/default.conf' for a specific setup.");
}

std::string parseArguments(const int argc, const char* arg) {
    if (argc == 1) {
        io::println("No configuration file provided. Using default: conf/default.conf");
        return ("conf/default.conf");
    } else if (argc == 2) {
        return (arg);
    } else {
        printUsage();
        std::exit(2);
    }
}

int main(int argc, char **argv)
{
    std::string configFile = parseArguments(argc, argv[1]);
    return (0);
}
