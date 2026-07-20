/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-20 / 18:37:09
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-20 / 22:04:59
 */

#include "net/Address.hpp"

#include <string>
#include <cstring>     // std::memset, std::memcpy
#include <arpa/inet.h> // htons, ntohs
#include <stdexcept>   // std::invalid_argument

namespace net {
    Address::Address(const std::string& host, unsigned short port){
        std::memset(&_addr, 0, sizeof(_addr));
        _addr.sin_family = AF_INET;
        _addr.sin_port = htons(port);
        _addr.sin_addr.s_addr = htonl(INADDR_ANY);
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
