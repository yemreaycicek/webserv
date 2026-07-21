/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-20 / 18:36:13
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-21 / 11:56:29
 */

#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "net/Exception.hpp"


namespace net{
    class Socket {
        private:
            int _fd;
            Socket(const Socket& other);
            Socket& operator=(const Socket& other);
        public:
            Socket(int fd);
            void    setNonBlocking();
            void    setReuseAddr();
            int     getFd() const;
            ~Socket();
            class SocketError : public Exception {
                public:
                    SocketError(const std::string& message);
                    ~SocketError() throw();
            };
    };
}

#endif //WEBSERV_NET_ADDRESS_HPP
