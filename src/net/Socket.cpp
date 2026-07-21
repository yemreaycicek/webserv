/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-20 / 18:07:38
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-21 / 11:29:53
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
        setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y));
        if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y)) < 0)
            throw InvalidArgument("setReuseAddr: setsockopt faild");
    }

    void Socket::setNonBlocking(){
        if (fcntl(_fd, F_SETFL, O_NONBLOCK) < 0)
            throw InvalidArgument("setNonBlocking: fcntl faild");
    }

    int Socket::getFd() const {
        return (_fd);
    }
}