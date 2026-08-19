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
    Cgi::Cgi(int clientFd) : _clientFd(clientFd), _state(NOT_STARTED), _inWrFd(-1), _outRdFd(-1), _pid(-1), _inputOffset(0), _inputDone(false), _hadAnyOutput(false), _headersRelayed(false), _lastActivity(0) {}
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
            }
            else if (n < 0) {
                // Pipe fds are non-blocking (see run()): a full pipe just means
                // "try again once POLLOUT fires next" rather than a real
                // failure. Without this, a single write() of the whole
                // remaining input (which can be much bigger than the pipe's
                // capacity) would otherwise have to be blocking to not
                // spuriously fail here — and a blocking write() that outruns
                // the pipe stalls this whole single-threaded server until the
                // CGI child drains it, which it can't do if it's
                // simultaneously blocked writing its own (unread) stdout
                // back to us.
                if (errno == EAGAIN || errno == EWOULDBLOCK) return ;
                _state = FAILED;
                return ;
            }
        }
        if (_inputOffset >= _input.size()) {
            // Reclaim whatever's already been written instead of letting
            // _input grow forever while feed() keeps appending more — this is
            // what actually bounds memory for a streamed body, not just the
            // non-blocking fds above.
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
        // A slow-but-steady client that's still handing us body bytes is
        // making progress, even if the pipe to the CGI hasn't drained any of
        // it yet — see the comment on _lastActivity.
        _lastActivity = time(NULL);
    }

    void Cgi::finishInput() {
        _inputDone = true;
        // Deliberately does NOT close _inWrFd here: this can be called outside
        // the poll-driven path (as soon as the client's request is complete),
        // and closing the fd without going through onWritable()/Server's
        // handleCgi() would leave Server's fd bookkeeping stale for that fd
        // number. The pipe is already registered for POLLOUT, which fires on
        // the next poll cycle regardless (there's room in the pipe buffer), so
        // onWritable() closes it properly once it runs.
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
            // relaying it out as it arrived, so "did we ever produce output"
            // has to be tracked separately from "is _output non-empty now".
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
            waitpid(_pid, NULL, 0);
            _pid = -1;
        }
    }

    bool Cgi::isTimedOut(int limitSec) const {
        // Idle timeout, not a hard cap: as long as the CGI keeps making
        // progress (input fed, input written, output read) this never fires,
        // no matter how long the whole transfer takes. It only fires once
        // limitSec seconds pass with no activity at all.
        return ((time(NULL) - _lastActivity) >= limitSec);
    }

    pid_t Cgi::getPid() const {
        return _pid;
    }

    void Cgi::setTimedOut() {
        _state = FAILED;
    }

    void Cgi::run(const RequestData& req, const std::string& interpreter, const std::string& scriptPath) {
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
            _input = req.body;
            _inWrFd = inPipe[1];
            _outRdFd = outPipe[0];
            close(inPipe[0]);
            close(outPipe[1]);
            // Non-blocking: see the comment in onWritable() for why a blocking
            // pipe here can deadlock the whole (single-threaded) server against
            // a CGI that reads-and-writes concurrently on a large body.
            fcntl(_inWrFd, F_SETFL, fcntl(_inWrFd, F_GETFL) | O_NONBLOCK);
            fcntl(_outRdFd, F_SETFL, fcntl(_outRdFd, F_GETFL) | O_NONBLOCK);
            // Keep stdin open even if nothing has arrived yet: more body may
            // still be streamed in via feed(). The caller closes it (via
            // finishInput()) once it knows no more body is coming.
            _state = WRITING;
        }
    }
}