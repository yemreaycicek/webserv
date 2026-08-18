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
    // Connection::Connection(int fd) : _socket(fd), _state(READING_REQUEST) {}
    Connection::Connection(int fd) : _socket(fd), _wrOffset(0), _wrComplete(true), _state(READING_REQUEST) {}
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
        //char bf[4096];
        char bf[65536]; /****** */
        ssize_t rc = recv(getFd(), bf, sizeof(bf), 0);
        if (rc > 0) {
            _request.parse(std::string(bf, rc));
        } else if (rc < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return; // spurious wakeup, nothing to read yet
            _state = CLOSING;
        } else { // rc == 0: peer closed
            _state = CLOSING;
        }
    }

    void Connection::onWritable(){
        if (_wrOffset < _wrBuf.size()) {
            ssize_t sd = send(getFd(), _wrBuf.c_str() + _wrOffset, _wrBuf.size() - _wrOffset, 0);
            if (sd > 0) {
                _wrOffset += sd;
            } else if (sd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return; // socket buffer full, try again later
                _state = CLOSING;
                return;
            } else {
                _state = CLOSING;
                return;
            }
        }
        if (_wrOffset >= _wrBuf.size()) {
            // Reclaim whatever's already been sent instead of letting _wrBuf grow
            // forever while a streaming response keeps appendStreamChunk()-ing.
            if (!_wrBuf.empty()) {
                _wrBuf.clear();
                _wrOffset = 0;
            }
            if (_wrComplete) _state = CLOSING;
        }
    }

    void Connection::setResponse(const std::string& resp){
        _wrBuf = resp;
        _wrOffset = 0;
        _wrComplete = true;
        _state = SENDING_RESPONSE;
    }

    void Connection::beginStreamResponse(const std::string& head){
        _wrBuf = head;
        _wrOffset = 0;
        _wrComplete = false;
        _state = SENDING_RESPONSE;
    }

    void Connection::appendStreamChunk(const std::string& chunk){
        _wrBuf += chunk;
    }

    void Connection::finishStreamResponse(){
        _wrComplete = true;
    }

    std::size_t Connection::pendingSendBytes() const {
        return (_wrBuf.size() - _wrOffset);
    }

    void Connection::clearRequestBody(){
        _request.clearBody();
    }

    std::string Connection::takeAvailableBody(){
        return (_request.takeBody());
    }
}