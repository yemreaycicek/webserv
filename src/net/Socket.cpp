/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-20 / 18:07:38
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-21 / 12:07:14
 */


#include "net/Socket.hpp"

#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

namespace net {
    Socket::Socket(int fd) : _fd(fd) {}

    Socket::~Socket(){
        if (_fd >= 0) close(_fd);
    }

    void Socket::setReuseAddr(){
        int y = 1;
        if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y)) < 0) throw SocketError("setReuseAddr: setsockopt failed");
    }

    void Socket::setNonBlocking(){
        if (fcntl(_fd, F_SETFL, O_NONBLOCK) < 0) throw SocketError("setNonBlocking: fcntl failed");
    }

    int Socket::getFd() const {
        return (_fd);
    }

    Socket::SocketError::SocketError(const std::string& message) : Exception(message) {}
    Socket::SocketError::~SocketError() throw() {}
}