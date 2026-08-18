/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 20:11:41
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-14 / 20:38:54
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
    // Backpressure bounds on how much unwritten CGI input we let pile up per
    // connection: past the high mark we stop reading more body from the client
    // socket until the child has drained it back down past the low mark. This
    // caps memory usage per in-flight upload instead of buffering it whole.
    static const std::size_t CGI_INPUT_HIGH_MARK = 1 * 1024 * 1024;
    static const std::size_t CGI_INPUT_LOW_MARK  = 256 * 1024;
    // NOTE: there is deliberately no equivalent backpressure on the CGI-output
    // -> client direction. See the comment in Server::relayCgiOutput() for why:
    // pausing there can deadlock against a client that writes its whole request
    // before reading any response (perfectly standard HTTP/1.1 behavior).
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
            void                                spawnCgiStream(int fd, CgiInfo& info, bool bodyComplete);
            bool                                dispatchCgi(int fd, exec::Connection* conCl);
            void                                feedCgiStream(int fd, exec::Connection* conCl, Cgi* cgi);
            void                                handleCgi(int fd);
            void                                relayCgiOutput(Cgi* cgi);
            std::string                         buildStreamedCgiHead(const std::string& headerBlock) const;
            std::string                         encodeChunk(const std::string& data) const;
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
            std::map<int, Cgi*>                 _cgiByClient; // client fd -> Cgi still receiving/streaming body
            std::vector<Cgi*>                   _cgiToClose;
    };
}

#endif // WEBSERV_EXEC_SERVER_HPP 
