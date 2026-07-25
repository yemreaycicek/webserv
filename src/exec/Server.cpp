/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 20:11:29
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-25 / 16:51:10
 */

#include "exec/Server.hpp"
#include "net/Address.hpp"
#include "utils/str.hpp"
#include <cstdlib>
#include <poll.h>


namespace exec {
    Server::Server(const config::Router& config) : _config(config), _poller() {
        std::vector<std::string> listenAddr = _config.getListenAddresses();
        for (size_t i = 0; i < listenAddr.size(); ++i){
            std::string addr = listenAddr[i];
            std::string::size_type pos = addr.find(':');
            std::string ip = addr.substr(0, pos);
            unsigned short port;
            if (!str::to_numeric(addr.substr(pos + 1), port)) throw std::runtime_error("Server: invalid port in listen address");
            net::Address address(ip, port);
            net::ListenSocket* ls = new net::ListenSocket(address);
            _poller.addFd(ls->getFd(), POLLIN);
            _listenSockets.push_back(ls);
            _lsAddr[ls->getFd()] = addr;
        }
    }
}
