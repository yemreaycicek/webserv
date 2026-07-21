/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-20 / 18:36:13
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-21 / 11:29:42
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
            class InvalidArgument : public Exception {
                public:
                    InvalidArgument(const std::string& message);
                    ~InvalidArgument() throw();
            };
        public:
            Socket(int fd);
            void    setNonBlocking();
            void    setReuseAddr();
            int     getFd() const;
            ~Socket();
    };
}

#endif //WEBSERV_NET_ADDRESS_HPP
