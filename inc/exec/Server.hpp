/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 20:11:41
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-25 / 16:51:59
 */

#ifndef WEBSERV_EXEC_SERVER_HPP
#define WEBSERV_EXEC_SERVER_HPP

#include "net/ListenSocket.hpp"
#include "config/Router.hpp"
#include "exec/Poller.hpp"
#include "exec/Connection.hpp"

#include <map>
#include <vector>
#include <string>

namespace exec {
    class Server {
        public:
            Server(const config::Router& config);
            ~Server();


        private:
            Server(const Server& other);
            Server&                             operator=(const Server& other);
            const config::Router&               _config;
            exec::Poller                        _poller;
            std::vector<net::ListenSocket*>     _listenSockets;          
            std::map<int, exec::Connection*>    _connections;
            std::map<int, std::string>          _clAddr;
            std::map<int, std::string>          _lsAddr;
    };
}

#endif // WEBSERV_EXEC_SERVER_HPP 
