/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 20:11:41
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-22 / 22:43:34
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
            Server(const config::Router& a);
            ~Server();
            void    run();
            
        private:
            Server(const Server& other);
            Server& operator=(const Server& other);
            //void    addClient(int fd);
            //void    deleteClient(int fd);
            //std::vector<net::ListenSocket*> _listenSockets;
            
    };
}

#endif // WEBSERV_EXEC_SERVER_HPP 
