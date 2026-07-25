/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 20:11:29
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-25 / 19:13:47
 */

#include "exec/Server.hpp"
#include "net/Address.hpp"
#include "utils/str.hpp"
#include <cstdlib>
#include <poll.h>
#include <iostream>

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
    exec::Server::~Server() {
        for (size_t i = 0; i < _listenSockets.size(); ++i) {
            delete _listenSockets[i];
        }
    }

    void Server::addCl(int cl_fd, short events) {
        exec::Connection* con = new exec::Connection(cl_fd);
        _connections[cl_fd] = con;
        _poller.addFd(cl_fd, POLLIN);
    }

    void Server::acceptCl(int ls_fd) {
        int cl_fd = _listenSockets[ls_fd]->acceptRun();
        if (cl_fd > 0) {
            _clAddr[cl_fd] = _lsAddr[ls_fd];
            addCl(cl_fd, POLLIN);
        }
    }
    
    void Server::run() {
        while (true) {
            std::vector<pollfd> pl = _poller.pollReady(120);
            for (size_t i = 0; i < pl.size(); ++i) {
                if (pl[i].revents & POLLIN) {
                    if (_lsAddr.count(pl[i].fd)) {
                        // listen
                        acceptCl(pl[i].fd);
                    } else {
                        // chlid
                        //handleCl(pl[i].fd, pl[i].revents);
                    }
                }
            }
        }
    }
    
}
