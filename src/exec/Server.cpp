/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 20:11:29
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-25 / 21:13:00
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
        std::map<int, exec::Connection*>::iterator it = _connections.begin();
        for (; it != _connections.end(); ++it)
            delete it->second;
        for (size_t i = 0; i < _listenSockets.size(); ++i)
            delete _listenSockets[i];
    }

    void Server::addCl(int cl_fd, short events) {
        exec::Connection* con = new exec::Connection(cl_fd);
        _connections[cl_fd] = con;
        _poller.addFd(cl_fd, events);
    }

    void Server::acceptCl(int ls_fd) {
        net::ListenSocket* ls = NULL;
        for (size_t i = 0; i < _listenSockets.size(); ++i) {
            if (_listenSockets[i]->getFd() == ls_fd) {
                ls = _listenSockets[i];
                break;
            }
        }
        int cl_fd = ls->acceptRun();
        if (cl_fd < 0) return;
        _clAddr[cl_fd] = _lsAddr[ls_fd];
        addCl(cl_fd, POLLIN);
    }

    void Server::delCl(int fd) {
        std::map<int, exec::Connection*>::iterator it = _connections.find(fd);
        if (it == _connections.end()) return;
        _poller.deleteFd(fd);
        delete it->second;
        _connections.erase(it);
        _clAddr.erase(fd);
    }

    void Server::handleCl(int fd, short revents) {
        exec::Connection *conCl;
        conCl = _connections[fd];
        if (revents & POLLIN) {
            conCl->onReadable();
            if (conCl->isRequestComplete()) {
                std::string buf = conCl->getRequestData();
                std::string res = "HTTP/1.1 200 OK\r\n\r\n";
                conCl->setResponse(res);
                _poller.setFdEvents(fd, POLLOUT);
            }
        }
        if (revents & POLLOUT) {
            conCl->onWritable();
            if (conCl->getState() == CLOSING){
                delCl(fd);
            }
        }
    }

    void Server::run() {
        while (true) {
            std::vector<pollfd> pl = _poller.pollReady(120);
            for (size_t i = 0; i < pl.size(); ++i) {
                if (_lsAddr.count(pl[i].fd)) {
                    acceptCl(pl[i].fd);
                }
                else {
                    handleCl(pl[i].fd, pl[i].revents);
                }
            }
        }
    }
}
