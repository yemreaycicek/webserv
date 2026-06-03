/**
 * @ Author: yaycicek
 * @ Create Time: 2026-05-27 / 22:18:27
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-03 / 19:29:02
 */

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "config/Lexer.hpp"
#include "utils/io.hpp"

static void printUsage() {
    io::errln("Usage: ./bin/webserv [CONFIGURATION_FILE]...");
    io::errln("Try './bin/webserv conf/default.conf' for a specific setup.");
}

static std::string parseArguments(const int argc, const char* arg) {
    if (argc == 1) {
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

    std::ifstream file(configFile.c_str());
    if (!file.is_open()) {
        return (1);
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    std::string configFileContent = oss.str();
    file.close();

    conf::Lexer lexer;
    std::vector<conf::Token> tokens = lexer.tokenize(configFileContent);
    return (0);
}
