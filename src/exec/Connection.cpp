/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 17:44:24
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-22 / 20:04:29
 */

#include "exec/Connection.hpp"
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

    bool Connection::isRequestComplete() const{
        ssize_t res = _rdBuf.find("\r\n\r\n", 0);
        if (res == std::string::npos) return (false);
        return (true);
    }

    void Connection::onReadable(){
        char bf[4096];
        ssize_t rc = recv(getFd(), bf, sizeof(bf), 0);        
        if (rc > 0) {
            _rdBuf.append(bf, rc);
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