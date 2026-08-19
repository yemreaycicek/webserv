/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 17:44:24
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-02 / 17:46:54
 */

#include "exec/Connection.hpp"
#include "http/Request.hpp"
#include <sys/socket.h>
#include <cerrno>

namespace exec {
    Connection::Connection(int fd) : _socket(fd), _wrComplete(true), _state(READING_REQUEST) {}

    Connection::~Connection() {}

    int Connection::getFd() const{
        return (_socket.getFd());
    }

    ConState Connection::getState() const{
        return (_state);
    }

    const http::Request& Connection::getRequest() const {
        return (_request);
    }

    bool Connection::isRequestComplete() const{
        return (_request.isComplete());
    }

    void Connection::onReadable(){
        char bf[4096];
        ssize_t rc = recv(getFd(), bf, sizeof(bf), 0);
        if (rc > 0) {
            _request.parse(std::string(bf, rc));
        } else if (rc < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return; // non-blocking socket, nothing to read yet
            _state = CLOSING;
        } else { // rc == 0: peer closed
            _state = CLOSING;
        }
    }

    void Connection::onWritable(){
        if (!_wrBuf.empty()) {
            ssize_t sd = send(getFd(), _wrBuf.c_str(), _wrBuf.size(), 0);
            if (sd > 0) {
                _wrBuf.erase(0, sd);
            }
            else if (sd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return; // socket buffer full, try again later
                _state = CLOSING;
                return;
            }
            else {
                _state = CLOSING;
                return;
            }
        }
        // Only actually close once nothing is queued AND no more is coming —
        // a streaming response can have _wrBuf empty between two chunks of
        // CGI output while it's still very much in progress.
        if (_wrBuf.empty() && _wrComplete) _state = CLOSING;
    }

    void Connection::setResponse(const std::string& resp){
        _wrBuf = resp;
        _wrComplete = true;
        _state = SENDING_RESPONSE;
    }

    void Connection::beginStreamResponse(const std::string& head){
        _wrBuf = head;
        _wrComplete = false;
        _state = SENDING_RESPONSE;
    }

    void Connection::appendStreamChunk(const std::string& data){
        _wrBuf += data;
    }

    void Connection::finishStreamResponse(){
        _wrComplete = true;
    }

    bool Connection::hasPendingOutput() const {
        return (!_wrBuf.empty());
    }

    void Connection::clearRequestBody(){
        _request.clearBody();
    }

    std::string Connection::takeAvailableBody(){
        return (_request.takeBody());
    }
}
