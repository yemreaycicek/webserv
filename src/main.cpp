/**
 * @ Author: yaycicek
 * @ Create Time: 2026-05-27 / 22:18:27
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-15 / 20:21:44
 */

#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "config/Lexer.hpp"
#include "config/Parser.hpp"
#include "utils/io.hpp"

static void printUsage() {
    io::errln("Usage: ./bin/webserv [CONFIGURATION_FILE]...");
    io::errln("Try './bin/webserv conf/default.conf' for a specific setup.");
}

static void parseArguments(std::string& configFilePath, const int argc, const char* arg) {
    if (argc == 1) {
        configFilePath = "conf/default.conf"; 
    } else if (argc == 2) {
        configFilePath = arg;
    } else {
        printUsage();
        std::exit(2);
    }
}

static void checkFileExtension(const std::string& filepath) {
    std::string filename;
    std::size_t slashPos = filepath.find_last_of('/');

    if (slashPos == std::string::npos) {
        filename = filepath;
    } else {
        filename = filepath.substr(slashPos + 1);
    }

    if (filename.length() < 6 || filename.substr(filename.length() - 5) != ".conf") {
        io::errln("Configuration file must have a valid base name and a '.conf' extension.");
        std::exit(1);
    }
}

static void readFileContent(std::string& configFileContent, const std::string& configFilePath) {
    if (access(configFilePath.c_str(), F_OK) != 0) {
        io::errln("Configuration file does not exist: " + configFilePath);
        std::exit(1);
    }
    if (access(configFilePath.c_str(), R_OK) != 0) {
        io::errln("Permission denied to read configuration file: " + configFilePath);
        std::exit(1);
    }

    std::ifstream file(configFilePath.c_str());
    if (!file.is_open()) {
        io::errln("Could not open configuration file: " + configFilePath);
        std::exit(1);
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    configFileContent = oss.str();
}

int main(int argc, char **argv)
{
    std::string configFilePath;
    std::string configFileContent;

    parseArguments(configFilePath, argc, argv[1]);
    checkFileExtension(configFilePath);
    readFileContent(configFileContent, configFilePath);

    conf::Lexer lexer;
    std::vector<conf::Token> tokens = lexer.tokenize(configFileContent);

    try {
        conf::Parser parser(tokens);
        std::vector<conf::ServerBlock> servers = parser.parse();
    } catch (const std::runtime_error& e) {
        io::println(e.what());
    }
    return (0);
}
