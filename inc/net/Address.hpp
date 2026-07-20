/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-20 / 18:37:49
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-20 / 22:32:24
 */

#ifndef ADRESS_HPP
#define ADRESS_HPP

#include <string>
#include <netinet/in.h>   // struct sockaddr_in
#include <sys/socket.h>   // struct sockaddr, socklen_t

namespace net {
    class Address {
        private:
            struct sockaddr_in _addr;
        public:
            Address(const std::string& host, unsigned short port);
            const struct sockaddr*  getAddr() const;
            socklen_t               getSize() const;
            unsigned short          getPort() const;
    };    
}

#endif //WEBSERV_NET_ADRESS_HPP
