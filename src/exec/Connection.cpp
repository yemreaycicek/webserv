/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 17:44:24
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-02 / 17:46:54
 */

#include "exec/Connection.hpp"
#include "http/Request.hpp"
#include <sys/socket.h>

namespace exec {
    Connection::Connection(int fd) : _socket(fd), _state(READING_REQUEST) {}

    Connection::~Connection() {}

    int Connection::getFd() const{
        return (_socket.getFd());
    }

    ConState Connection::getState() const{
        return (_state);
    }

    const std::string& Connection::getRequestData() const{
        return (_rdBuf);
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
            _rdBuf.append(bf, rc);
            _request.parse(std::string(bf, rc));
        } else if (rc <= 0){
            _state = CLOSING;
        }
    }

    void Connection::onWritable(){
        ssize_t sd = send(getFd(), _wrBuf.c_str(), _wrBuf.size(), 0);
        if (sd > 0) {
            _wrBuf.erase(0, sd);
            if (_wrBuf.empty()) _state = CLOSING;
        }
        else {
            _state = CLOSING;
        }
    }

    void Connection::setResponse(const std::string& resp){
        _wrBuf = resp;
        _state = SENDING_RESPONSE;
    }
}