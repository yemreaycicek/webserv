/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-20 / 18:37:09
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-20 / 22:33:36
 */

#include "net/Address.hpp"

#include <string>
#include <cstring>     // std::memset, std::memcpy
#include <arpa/inet.h> // htons, ntohs
#include <stdexcept>
#include <netinet/in.h>

namespace net {
    Address::Address(const std::string& host, unsigned short port){
        std::memset(&_addr, 0, sizeof(_addr));
        _addr.sin_family = AF_INET;
        _addr.sin_port = htons(port);
        if (host == "0.0.0.0"){
            _addr.sin_addr.s_addr = htonl(INADDR_ANY);
        } else if (host == "127.0.0.1") {
            _addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        } else {
            throw std::invalid_argument("Address: desteklenmeyen host: " + host); //? burada sadece iki tanesi mi olmalı yoksa hepsini almalıyız?
        }
    }

    const struct sockaddr* Address::getAddr() const {
        return (reinterpret_cast<const struct sockaddr*>(&_addr));
    }

    socklen_t Address::getSize() const {
        return (sizeof(_addr));
    }

    unsigned short Address::getPort() const{
        return (ntohs(_addr.sin_port));
    }
}
