/**
 * @ Author: yaycicek
 * @ Create Time: 2026-05-31 / 20:51:28
 * @ Modified by: yaycicek
 * @ Modified time: 2026-05-31 / 23:24:29
 */

#ifndef WEBSERV_UTILS_LOGGER_HPP
#define WEBSERV_UTILS_LOGGER_HPP

#include <fstream>
#include <string>


enum LogLevel {
    INFO,
    WARNING,
    ERROR
};

class Logger {
    public:
        static Logger& getInstance();

        void log(const LogLevel level, const std::string& message);

    private:
        std::ofstream _logFile;

        Logger();
        Logger(const Logger& other);
        Logger& operator=(const Logger& other);
        ~Logger();

        std::string getTimestamp() const;
};

#ifndef DEBUG_MODE
# define DEBUG_MODE 0
#endif

#if DEBUG_MODE
    #define LOG_INFO(msg) Logger::getInstance().log(INFO, msg)
    #define LOG_WARN(msg) Logger::getInstance().log(WARNING, msg)
    #define LOG_ERR(msg) Logger::getInstance().log(ERROR, msg)
#else
    #define LOG_INFO(msg) ((void)0)
    #define LOG_WARN(msg) ((void)0)
    #define LOG_ERR(msg) ((void)0)
#endif

#endif // WEBSERV_UTILS_LOGGER_HPP
