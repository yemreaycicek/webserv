/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-06 / 20:22:57
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-20 / 16:53:52
 */


#ifndef WEBSERV_EXEC_CGI_HPP
#define WEBSERV_EXEC_CGI_HPP

#include <string>
#include <map>
#include <vector>
#include <sys/types.h> 
#include <ctime>

namespace exec {
    enum State {
        NOT_STARTED,
        WRITING,
        READING,
        DONE,
        FAILED
    };
    struct RequestData {
        std::string method;
        std::string path;
        std::string query;
        std::map<std::string, std::string> headers;
        std::string body;
        std::size_t contentLength;
        RequestData() : contentLength(0) {}
    };
    struct CgiInfo {
        bool        isCgi;
        std::string interpreter;
        std::string scriptPath;
        RequestData reqData;
    };
    class Cgi {
        public:
            Cgi(int clientFd);
            ~Cgi();

            void    run(const RequestData& req, const std::string& interpreter, const std::string& scriptPath);
            void    onWritable();
            void    onReadable();
            void    feed(const std::string& chunk);
            void    finishInput();
            std::size_t pendingInputBytes() const;
            State   getState() const;
            int     getInFd()  const;
            int     getOutFd() const;
            void    cleanup();
            int getClientFd() const;
            const std::string& rawOutput() const;
            bool isTimedOut(int limitSec) const;
            pid_t getPid() const;
            void setTimedOut();

            const std::string& peekOutput() const;
            void        dropOutputPrefix(std::size_t n);
            std::string takeOutput();
            bool        headersRelayed() const;
            void        setHeadersRelayed();

        private:
            Cgi(const Cgi&);
            Cgi& operator=(const Cgi&);

            std::vector<std::string> buildEnv(const RequestData& req, const std::string& scriptPath) const;
            int _clientFd;
            State   _state;
            int     _inWrFd;
            int     _outRdFd;
            pid_t   _pid;
            std::string _input;
            std::size_t _inputOffset;
            bool        _inputDone;
            std::string _output;
            bool        _hadAnyOutput;
            bool        _headersRelayed;
            time_t _lastActivity;
    };
}

#endif // WEBSERV_EXEC_CGI_HPP 
