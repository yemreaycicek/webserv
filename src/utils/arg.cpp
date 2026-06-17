/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-16 / 22:38:38
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-17 / 13:39:45
 */

#include "utils/arg.hpp"

#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <sstream>

#include "utils/io.hpp"

namespace arg {
    Parser::Parser() {}
    Parser::Parser(int argc, char** argv) {
        parseArguments(argc, argv);
        checkFileExtension();
        readFileContent();
    }
    Parser::Parser(const Parser& other) : _configFilePath(other._configFilePath), _configFileContent(other._configFileContent) {}

    Parser& Parser::operator=(const Parser& other) {
        if (this != &other) {
            _configFilePath = other._configFilePath;
            _configFileContent = other._configFileContent;
        }
        return (*this);
    }

    Parser::~Parser() {}

    const std::string& Parser::getConfigFileContent() const {
        return (_configFileContent);
    }

    void Parser::printUsage() const {
        io::errln("Usage: ./bin/webserv [CONFIGURATION_FILE]...");
        io::errln("Try './bin/webserv conf/default.conf' for a specific setup.");
    }

    void Parser::parseArguments(int argc, char** argv) {
        if (argc == 1) {
            _configFilePath = "conf/default.conf";
        } else if (argc == 2) {
            std::string arg = argv[1];

            if (arg == "-h" || arg == "--help") {
                // ...
                std::exit(1);
            } else {
                _configFilePath = arg;
            }
        } else {
            printUsage();
            std::exit(2);
        }
    }

    void Parser::checkFileExtension() {
        std::string filename;
        std::size_t slashPos = _configFilePath.find_last_of('/');

        if (slashPos == std::string::npos) {
            filename = _configFilePath;
        } else {
            filename = _configFilePath.substr(slashPos + 1);
        }

        if (filename.length() < 6 || filename.substr(filename.length() - 5) != ".conf") {
            throw std::runtime_error("Configuration file must have a valid base name and a '.conf' extension.");
        }
    }

    void Parser::readFileContent() {
        if (access(_configFilePath.c_str(), F_OK) != 0) {
            throw std::runtime_error("Configuration file does not exist: " + _configFilePath);
        }
        if (access(_configFilePath.c_str(), R_OK) != 0) {
            throw std::runtime_error("Permission denied to read configuration file: " + _configFilePath);
        }

        std::ifstream file(_configFilePath.c_str());
        if (!file.is_open()) {
            throw std::runtime_error("Could not open configuration file: " + _configFilePath);
        }

        std::ostringstream oss;
        oss << file.rdbuf();
        _configFileContent = oss.str();
    }
}
