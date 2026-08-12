/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-06 / 21:21:09
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-12 / 15:08:57
 */


#include <unistd.h>
#include "exec/Cgi.hpp"
#include <sstream>

namespace exec {
    Cgi::Cgi() : _state(NOT_STARTED), _inWrFd(-1), _outRdFd(-1), _pid(-1), _inputOffset(0) {}
    Cgi::~Cgi() {}

    std::vector<std::string> Cgi::buildEnv(const RequestData& req) const {
        std::vector<std::string> env;
        env.push_back("GATEWAY_INTERFACE=CGI/1.1");
        env.push_back("SERVER_PROTOCOL=HTTP/1.1");

        env.push_back("REQUEST_METHOD=" + req.method);
        env.push_back("QUERY_STRING=" + req.query);
        env.push_back("SCRIPT_NAME=" + req.path);
        
        std::stringstream ss;
        ss << req.body.size();
        env.push_back("CONTENT_LENGTH=" + ss.str());

        return env;
    }

    void Cgi::run(const RequestData& req) {
        int inPipe[2];
        int outPipe[2];
        if (pipe(inPipe) == -1) {
            _state = FAILED;
            return ;
        }
        if (pipe(outPipe) == -1) {
            close(inPipe[0]);
            close(inPipe[1]);
            _state = FAILED;
            return ;
        }
        _pid = fork();
        if (_pid == -1) {
            close(inPipe[0]);
            close(inPipe[1]);
            close(outPipe[0]);
            close(outPipe[1]);
            _state = FAILED;
            return ;
        }
        if (_pid == 0) {
            if (dup2(inPipe[0], STDIN_FILENO) == -1) _exit(1);
            if (dup2(outPipe[1], STDOUT_FILENO) == -1) _exit(1);
            close(inPipe[0]);
            close(inPipe[1]);
            close(outPipe[0]);
            close(outPipe[1]);
            char* av[] = {(char *)"/usr/bin/python3", (char *)"/home/monster/code/webserv/src/exec/selam.py", NULL};
            char* envp[] = { NULL };
            execve(av[0], av, envp);
            _exit(1);
        } else {
            _inWrFd = inPipe[1];
            _outRdFd = outPipe[0];
            close(inPipe[0]);
            close(outPipe[1]);
            _state = READING;
        }
    }
}