/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-06 / 21:21:09
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-11 / 21:00:40
 */


#include <unistd.h>
#include "exec/Cgi.hpp"

namespace exec {
    Cgi::Cgi() : _state(NOT_STARTED), _inWrFd(-1), _outRdFd(-1), _pid(-1), _inputOffset(0) {}
    Cgi::~Cgi() {}



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