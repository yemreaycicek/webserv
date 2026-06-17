/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-16 / 22:39:01
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-17 / 13:39:50
 */

#ifndef WEBSERV_UTILS_ARG_HPP
#define WEBSERV_UTILS_ARG_HPP

#include <string>

namespace arg {
    class Parser {
        public:
            Parser(int argc, char** argv);
            Parser(const Parser& other);
            Parser& operator=(const Parser& other);
            ~Parser();

            const std::string& getConfigFileContent() const;

        private:
            std::string _configFilePath;
            std::string _configFileContent;

            Parser();

            void parseArguments(int argc, char** argv);
            void checkFileExtension();
            void readFileContent();
            void printUsage() const;
    };
}

#endif // WEBSERV_UTILS_ARG_HPP
