/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 20:11:29
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-14 / 20:44:38
 */

#include "exec/Server.hpp"
#include "net/Address.hpp"
#include "utils/str.hpp"
#include "exec/Executor.hpp"
#include <cstdlib>
#include <poll.h>
#include <iostream>
#include <signal.h>

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
        if (ls == NULL) return;
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
    void Server::buildCgi(int fd, CgiInfo& info) {
        Cgi* cgi = new Cgi(fd);
        cgi->run(info.reqData, info.interpreter, info.scriptPath);
        if (cgi->getState() == FAILED) {
            delete cgi;
            return;
        }
        int outFd = cgi->getOutFd();
        _poller.addFd(outFd, POLLIN);
        _cgi[outFd] = cgi;
        if (cgi->getInFd() != -1) {
            int inFd = cgi->getInFd();
            _poller.addFd(inFd, POLLOUT);
            _cgi[inFd] = cgi;
        }
        _poller.setFdEvents(fd, 0);
    }
    void Server::handleCl(int fd, short revents) {
        exec::Connection *conCl = _connections[fd];
        if (revents & POLLIN) {
            conCl->onReadable();
            if (conCl->isRequestComplete()) {
                std::string res;
                try {
                    CgiInfo cgiInfo;
                    const config::ServerBlock& sb = _config.getServerBlock(_clAddr[fd]);
                    res = _executor.execute(sb, conCl->getRequest(), cgiInfo);
                    if (cgiInfo.isCgi) {
                        buildCgi(fd, cgiInfo);
                    }
                    else {
                        conCl->setResponse(res);
                        _poller.setFdEvents(fd, POLLOUT);
                    }
                } catch(const std::exception& e) {
                    res = "HTTP/1.1 500 Internal Server Error\r\n "
                    "Content-Length: 0\r\n"
                    "Connection: close\r\n\r\n";
                }
            }
            else if (conCl->getRequest().hasError()) {
                std::string res = "HTTP/1.1 400 Bad Request\r\n"
                "Content-Length: 0\r\n"
                "Connection: close\r\n\r\n";
                conCl->setResponse(res);
                _poller.setFdEvents(fd, POLLOUT);
            }
        }
        if (revents & POLLOUT) {
            conCl->onWritable();
            if (conCl->getState() == CLOSING){
                _toClose.push_back(fd);
            }
        }
    }

    void Server::handleCgi(int fd) {
        Cgi* cgi = _cgi[fd];
        if (fd == cgi->getInFd()) cgi->onWritable();
        else if (fd == cgi->getOutFd()) cgi->onReadable();
        if (cgi->getState() == DONE || cgi->getState() == FAILED) {
            std::map<int, Cgi*>::iterator it = _cgi.begin();
            while (it != _cgi.end()) {
                if (it->second == cgi) {
                    _poller.deleteFd(it->first);
                    std::map<int, Cgi*>::iterator toErase = it++;
                    _cgi.erase(toErase);
                } else ++it;
            }
            _cgiToClose.push_back(cgi);
        }
    }

    std::string Server::cgiToHttp(const std::string& raw) const {
        std::string headers;
        std::string body;
        std::string::size_type pos = raw.find("\r\n\r\n");
        size_t sepLen = 4;
        if (pos == std::string::npos) {
            pos = raw.find("\n\n");
            sepLen = 2;
        }
        if (pos != std::string::npos) {
            headers = raw.substr(0, pos);
            body = raw.substr(pos + sepLen);
        } else {
            body = raw;
        }
        std::stringstream res;
        res << "HTTP/1.1 200 OK\r\n";
        if (!headers.empty()) res << headers << "\r\n";
        res << "Content-Length: " << body.size() << "\r\n";
        res << "\r\n";
        res << body;
        return res.str();
    }


    void Server::delCgi(Cgi* cgi) {
        std::string res;
        if (cgi->getState() == DONE) res = cgiToHttp(cgi->rawOutput());
        else res = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        std::map<int, Connection*>::iterator it = _connections.find(cgi->getClientFd());
        if (it != _connections.end()) {
            it->second->setResponse(res);
            _poller.setFdEvents(cgi->getClientFd(), POLLOUT);
        }
        cgi->cleanup();
        delete cgi;
    }

    void Server::run() {
        while (true) {
            std::vector<pollfd> pl = _poller.pollReady(120);
            for (size_t i = 0; i < pl.size(); ++i) {
                if (_lsAddr.count(pl[i].fd))    acceptCl(pl[i].fd);
                else if (_cgi.count(pl[i].fd))  handleCgi(pl[i].fd);
                else                            handleCl(pl[i].fd, pl[i].revents);
            }
            for (size_t i = 0; i < _toClose.size(); ++i) {
                delCl(_toClose[i]);
            }
            for (size_t i = 0; i < _cgiToClose.size(); ++i) {
                delCgi(_cgiToClose[i]);
            }
            _cgiToClose.clear();
            _toClose.clear();
            
            for (std::map<int, Cgi*>::const_iterator it = _cgi.begin(); it != _cgi.end(); ++it) {
                if (it->second->getState() == WRITING || it->second->getState() == READING) {
                    if (it->second->isTimedOut(CGI_TIMEOUT_SEC)) {
                        kill(it->second->getPid(), SIGKILL);
                        it->second->setTimedOut();
                        _cgiToClose.push_back(it->second);

                    }
                }
            }
        }
    }
}
