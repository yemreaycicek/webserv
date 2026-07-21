/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-20 / 18:36:42
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-21 / 17:12:55
 */

#ifndef WEBSERV_NET_LISTENSOCKET_HPP
#define WEBSERV_NET_LISTENSOCKET_HPP

#include "Address.hpp"
#include "Socket.hpp"
#include "net/Exception.hpp"
#include <stdexcept>


namespace net {
    class ListenSocket : public Socket {
        public:
            explicit ListenSocket(const Address& addr);
            int acceptRun();
            ~ListenSocket();
            class CreateSocketERR : public Exception {
                public:
                    CreateSocketERR(const std::string& message);
                    ~CreateSocketERR() throw();
            };
        private:
            static int createSocket();
    };
}

#endif // WEBSERV_NET_LISTENSOCKET_HPP 
