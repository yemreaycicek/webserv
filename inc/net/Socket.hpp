/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-20 / 18:36:13
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-21 / 00:20:36
 */

#ifndef SOCKET_HPP
#define SOCKET_HPP

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
    };
}

#endif //WEBSERV_NET_ADDRESS_HPP
