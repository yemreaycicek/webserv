/**
 * @ Author: yaycicek
 * @ Create Time: 2026-05-31 / 20:51:21
 * @ Modified by: yaycicek
 * @ Modified time: 2026-05-31 / 23:34:56
 */

#include "utils/Logger.hpp"

#include <sys/stat.h>

#include <cerrno>
#include <cstring>
#include <ctime>
#include <iostream>

Logger::Logger() {
    if (mkdir("log", 0777) == -1) {
        if (errno != EEXIST) {
            std::cerr << "Failed to create 'log' directory: " << std::strerror(errno) << std::endl;
        }
    }

    _logFile.open("log/webserv.log", std::ios::out | std::ios::trunc);
    if (!_logFile.is_open()) {
        std::cerr << "Could not open log file!" << std::endl;
    } else {
        std::cout << "Logging system activated. All events are being recorded to log/webserv.log" << std::endl; 
    }
}

Logger::~Logger() {
    if (_logFile.is_open()) {
        _logFile.close();
    }
}

Logger& Logger::getInstance() {
    static Logger logger;
    return (logger);
}

void Logger::log(const LogLevel level, const std::string& message) {
    if (!_logFile.is_open()) {
        return;
    }

    std::string prefix;
    switch (level) {
        case INFO:
            prefix = "[ INFO ]    ";
            break;
        case WARNING:
            prefix = "[ WARNING ] ";
            break;
        case ERROR:
            prefix = "[ ERROR ]   ";
            break;
    }
    _logFile << "[ " << getTimestamp() << " ] " << prefix << message << std::endl;
}

std::string Logger::getTimestamp() const {
    std::time_t now = std::time(NULL);
    std::tm* localTime = std::localtime(&now);

    char buffer[24];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime);

    return (std::string(buffer));
}
