/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-20 / 18:36:42
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-21 / 16:47:08
 */

#ifndef WEBSERV_NET_LISTENSOCKET_HPP
#define WEBSERV_NET_LISTENSOCKET_HPP

#include "Address.hpp"
#include "Socket.hpp"

namespace net {
    class ListenSocket : public Socket {
        public:
            explicit ListenSocket(const Address& addr);
            int accept();
            ~ListenSocket();         
    };
}

#endif // WEBSERV_NET_LISTENSOCKET_HPP 
