/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-06 / 21:21:09
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-15 / 16:10:49
 */


#include <unistd.h>
#include "exec/Cgi.hpp"
#include <sstream>
#include <sys/wait.h>
#include <fcntl.h>
#include <cerrno>

namespace exec {
    Cgi::Cgi(int clientFd) : _clientFd(clientFd), _state(NOT_STARTED), _inWrFd(-1), _outRdFd(-1), _pid(-1), _inputOffset(0), _inputDone(false), _totalFed(0), _maxInputBytes(0), _hadAnyOutput(false), _headersRelayed(false), _lastActivity(0) {}
    Cgi::~Cgi() {}

    std::vector<std::string> Cgi::buildEnv(const RequestData& req, const std::string& scriptPath) const {
        std::vector<std::string> env;
        env.push_back("GATEWAY_INTERFACE=CGI/1.1");
        env.push_back("SERVER_PROTOCOL=HTTP/1.1");

        env.push_back("REQUEST_METHOD=" + req.method);
        env.push_back("QUERY_STRING=" + req.query);
        env.push_back("SCRIPT_NAME=");
        env.push_back("PATH_INFO=" + req.path);
        env.push_back("SCRIPT_FILENAME=" + scriptPath);
        std::map<std::string, std::string>::const_iterator it = req.headers.find("Content-Type");
        if (it != req.headers.end()) env.push_back("CONTENT_TYPE=" + it->second);
        std::stringstream ss;
        ss << req.contentLength;
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

    int Cgi::getClientFd() const {
        return _clientFd;
    }

    void Cgi::onWritable() {
        if (_inputOffset < _input.size()) {
            ssize_t n = write(_inWrFd, (_input.c_str() + _inputOffset), (_input.size() - _inputOffset));
            if (n > 0) {
                _inputOffset += n;
                _lastActivity = time(NULL);
            } else if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return ;
                _state = FAILED;
                return ;
            }
        }
        if (_inputOffset >= _input.size()) {
            // Everything queued so far has been written to the child. Reclaim the
            // buffer instead of letting it grow forever as more feed() calls keep
            // appending past an ever-increasing offset.
            if (!_input.empty()) {
                _input.clear();
                _inputOffset = 0;
            }
            if (_inputDone) {
                close (_inWrFd);
                _inWrFd = -1;
                _state = READING;
            }
        }
    }

    void Cgi::feed(const std::string& chunk) {
        if (chunk.empty()) return;
        _input += chunk;
        _totalFed += chunk.size();
        _lastActivity = time(NULL);
    }

    void Cgi::setMaxInputBytes(std::size_t n) {
        _maxInputBytes = n;
    }

    bool Cgi::inputOverLimit() const {
        return (_maxInputBytes != 0 && _totalFed > _maxInputBytes);
    }

    void Cgi::finishInput() {
        _inputDone = true;
        // Deliberately does NOT close _inWrFd here: this can be called from
        // outside the poll-driven path (Server::feedCgiStream, as soon as the
        // client's request is complete), and closing the fd without going
        // through onWritable()/handleCgi() would leave Server's _cgi map and
        // poller registration stale for that fd number — a dangling entry that
        // a later accept()/pipe() reusing the same fd number would collide with.
        // The pipe is already registered for POLLOUT, which will fire on the
        // next poll cycle regardless (room in the pipe buffer), so onWritable()
        // closes it properly and Server cleans up its bookkeeping in lockstep.
    }

    std::size_t Cgi::pendingInputBytes() const {
        return (_input.size() - _inputOffset);
    }

    void Cgi::onReadable() {
        char buf[65536];
        ssize_t n;
        n = read(_outRdFd, buf, sizeof(buf));
        if (n > 0) {
            _output.append(buf, n);
            _hadAnyOutput = true;
            _lastActivity = time(NULL);
        }
        else if (n == 0) {
            close(_outRdFd);
            _outRdFd = -1;
            waitpid(_pid, NULL, 0);
            _pid = -1;
            // _output may already have been drained via takeOutput() while
            // streaming, so "did we ever produce output" has to be tracked
            // separately from "is _output non-empty right now".
            if (_hadAnyOutput) _state = DONE;
            else _state = FAILED;
        }
        else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return ;
            _state = FAILED;
        }
    }

    const std::string& Cgi::peekOutput() const {
        return (_output);
    }

    void Cgi::dropOutputPrefix(std::size_t n) {
        _output.erase(0, n);
    }

    std::string Cgi::takeOutput() {
        std::string out;
        out.swap(_output);
        return (out);
    }

    bool Cgi::headersRelayed() const {
        return (_headersRelayed);
    }

    void Cgi::setHeadersRelayed() {
        _headersRelayed = true;
    }

    int Cgi::getOutFd() const {
        return (_outRdFd);
    }

    State Cgi::getState() const {
        return (_state);
    }

    int Cgi::getInFd() const {
        return (_inWrFd);
    }

    const std::string& Cgi::rawOutput() const {
        return _output;
    }

    void Cgi::cleanup() {
        if (_inWrFd != -1) {
            close(_inWrFd);
            _inWrFd = -1;
        }
        if (_outRdFd != -1) {
            close(_outRdFd);
            _outRdFd = -1;
        }
        if (_pid != -1) {
            //waitpid(_pid, NULL, 0);
            waitpid(_pid, NULL, WNOHANG); //**** */
            _pid = -1;
        }
    }

    bool Cgi::isTimedOut(int limitSec) const {
        // Idle timeout, not a hard cap: as long as the CGI keeps making
        // progress (input fed/written, output read) this never fires, however
        // long the whole transfer takes. It only fires once *limitSec* seconds
        // pass with no activity at all — i.e. the CGI is genuinely stuck.
        return ((time(NULL) - _lastActivity) >= limitSec);
    }

    pid_t Cgi::getPid() const {
        return _pid;
    }

    void Cgi::setTimedOut() {
        _state = FAILED;
    }

    void Cgi::run(RequestData& req, const std::string& interpreter, const std::string& scriptPath) {
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
            char* av[] = {const_cast<char*>(interpreter.c_str()), const_cast<char*>(scriptPath.c_str()), NULL};
            std::vector<std::string> envStr = buildEnv(req, scriptPath);
            std::vector<char*> envp;
            for (std::vector<std::string>::const_iterator it = envStr.begin(); it != envStr.end(); ++it) {
                envp.push_back(const_cast<char*>(it->c_str()));
            }
            envp.push_back(NULL);
            execve(av[0], av, &envp[0]);
            _exit(1);
        }
        else {
            _lastActivity = time(NULL);
            // Take ownership of the body without copying it: req.body was already
            // built specifically for this CGI call and isn't needed afterwards.
            _input.swap(req.body);
            _totalFed = _input.size();
            _maxInputBytes = req.maxBodySize;
            _inWrFd = inPipe[1];
            _outRdFd = outPipe[0];
            close(inPipe[0]);
            close(outPipe[1]);
            //fcntl(_inWrFd, F_SETFL, O_NONBLOCK);
            //fcntl(_outRdFd, F_SETFL, O_NONBLOCK);
            fcntl(_inWrFd, F_SETFL, fcntl(_inWrFd, F_GETFL) | O_NONBLOCK); /*** */
            fcntl(_outRdFd, F_SETFL, fcntl(_outRdFd, F_GETFL) | O_NONBLOCK); /*** */
            // Keep stdin open even if nothing has arrived yet: more body may still
            // be streamed in via feed(). The caller closes it (via finishInput())
            // once it knows no more body is coming.
            _state = WRITING;
        }
    }
}