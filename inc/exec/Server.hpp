/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 20:11:41
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-20 / 16:07:35
 */

#ifndef WEBSERV_EXEC_SERVER_HPP
#define WEBSERV_EXEC_SERVER_HPP

#include "net/ListenSocket.hpp"
#include "config/Router.hpp"
#include "exec/Poller.hpp"
#include "exec/Connection.hpp"
#include "exec/Server.hpp"
#include "exec/Executor.hpp"
#include "exec/Cgi.hpp"

#include <map>
#include <vector>
#include <string>

namespace exec {
    static const int CGI_TIMEOUT_SEC = 60;
    class Server {
        public:
            Server(const config::Router& config);
            ~Server();

            void                                run();
        private:
            Server(const Server& other);
            Server&                             operator=(const Server& other);

            void                                addCl(int cl_fd, short events);
            void                                delCl(int fd);
            void                                delCgi(Cgi* cgi);
            void                                handleCl(int fd, short revents);
            void                                acceptCl(int ls_fd);
            void                                buildCgi(int fd, CgiInfo& info, bool bodyComplete);
            bool                                dispatchCgi(int fd, exec::Connection* conCl);
            void                                feedCgiStream(int fd, exec::Connection* conCl, Cgi* cgi);
            void                                handleCgi(int fd);
            void                                relayCgiOutput(Cgi* cgi);
            std::string                         buildStreamedCgiHead(const std::string& headerBlock) const;
            std::string                         cgiToHttp(const std::string& raw) const;

            const config::Router&               _config;
            exec::Poller                        _poller;
            exec::Executor                      _executor;
            std::vector<net::ListenSocket*>     _listenSockets;
            std::vector<int>                    _toClose;
            std::map<int, exec::Connection*>    _connections;
            std::map<int, std::string>          _clAddr;
            std::map<int, std::string>          _lsAddr;
            std::map<int, Cgi*>                 _cgi;
            std::map<int, Cgi*>                 _cgiByClient;
            std::vector<Cgi*>                   _cgiToClose;
    };
}

#endif // WEBSERV_EXEC_SERVER_HPP 
