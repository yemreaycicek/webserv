/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-20 / 18:07:45
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-21 / 20:47:02
 */

#include "net/ListenSocket.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string>
#include <stdexcept>
#include <fcntl.h>



namespace net {
    ListenSocket::ListenSocket(const Address& addr) : Socket( createSocket() ) {
        setReuseAddr();
        setNonBlocking();

        if (bind(getFd(), addr.getAddr(), addr.getSize()) < 0) throw ListenSocketError("ListenSocket: bind() failed");
        if (listen(getFd(), 128) < 0 ) throw ListenSocketError("ListenSocket: listen() failed");
    }

    int ListenSocket::createSocket() {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
            throw ListenSocketError("ListenSocket: socket() failed");
        return (fd);
    }

    ListenSocket::~ListenSocket() {}

    int ListenSocket::acceptRun() {
        int client_fd = accept(getFd(), NULL, NULL);

        if (client_fd < 0) return (client_fd);

        if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
            close(client_fd);
            return (-1);
        }
        // Without this, small writes (our chunked-response pieces, or a client
        // sending in small increments) interact with the peer's delayed-ACK
        // timer via Nagle's algorithm, each one stalling tens of ms — turning
        // a sub-second loopback transfer into one slow enough to trip the CGI
        // timeout on a large body. Best-effort: not fatal if unsupported.
        int y = 1;
        setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &y, sizeof(y));
        return (client_fd);
    }

    ListenSocket::ListenSocketError::ListenSocketError(const std::string& message) : Exception(message) {}
    ListenSocket::ListenSocketError::~ListenSocketError() throw() {}
}