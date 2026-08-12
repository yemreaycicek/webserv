/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-06 / 21:21:09
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-12 / 16:37:32
 */


#include <unistd.h>
#include "exec/Cgi.hpp"
#include <sstream>
#include <algorithm>

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
        std::map<std::string, std::string>::const_iterator it = req.headers.find("Content-Type");
        if (it != req.headers.end()) env.push_back("CONTENT_TYPE=" + it->second);
        std::stringstream ss;
        ss << req.body.size();
        env.push_back("CONTENT_LENGTH=" + ss.str());

        for (std::map<std::string, std::string>::const_iterator it = req.headers.begin(); it != req.headers.end(); ++it) {
            std::string envName;
            for (size_t i = 0; i < it->first.size(); ++i) {
                if (it->first[i] == '-') envName += '_';
                else envName += std::toupper(static_cast<unsigned char>(it->first[i]));
            }
            if (envName == "CONTENT_TYPE" || envName == "CONTENT_LENGTH") continue;
            env.push_back("HTTP_" + envName + "=" + it->second);
        }
        return env;
    }

    void Cgi::onWritable() {
        ssize_t n;
        n = write(_inWrFd, (_input.c_str() + _inputOffset), (_input.size() - _inputOffset));
        if (n > 0) {
            _inputOffset += n;
        }
        else if (n < 0) {
            _state = FAILED;
            return ;
        }
        if (_inputOffset == _input.size()) {
            close (_inWrFd);
            _inWrFd = -1;
            _state = READING;
        }
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
            std::vector<std::string> envStr = buildEnv(req);
            std::vector<char*> envp;
            for (std::vector<std::string>::iterator it = envStr.begin(); it != envStr.end(); ++it) {
                envp.push_back(const_cast<char*>(it->c_str()));
            }
            envp.push_back(NULL);
            execve(av[0], av, &envp[0]);
            _exit(1);
        }
        else {
            _input = req.body;
            _inWrFd = inPipe[1];
            _outRdFd = outPipe[0];
            close(inPipe[0]);
            close(outPipe[1]);
            _state = READING;
        }
    }
}