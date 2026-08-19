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
    // Idle-ish hard cap on a CGI's total run time. Output is buffered so it
    // only really bounds how long we'll wait once things stop moving, but
    // input can now stream in over a while for a big body, so keep this
    // generous rather than the old 5s.
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
            // As soon as a request's headers are ready, decides whether it's a
            // CGI location — before its body (possibly large) has fully
            // arrived — so a CGI can start receiving that body as it streams
            // in. Returns true if the request has been fully handled by this
            // call (error queued, or CGI spawned); false if it's not a CGI
            // location at all, so the caller falls back to normal handling.
            bool                                dispatchCgi(int fd, exec::Connection* conCl);
            // Feeds whatever new body bytes just arrived on `fd` into the CGI
            // already streaming for it, closing its stdin once the request
            // is complete.
            void                                feedCgiStream(int fd, exec::Connection* conCl, Cgi* cgi);
            void                                handleCgi(int fd);
            // Drains whatever the CGI has newly produced and relays it to the
            // client as it arrives, instead of waiting for the CGI to finish
            // and buffering the whole (possibly huge) response — see the
            // comment on Connection::beginStreamResponse() for the framing.
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
            std::map<int, Cgi*>                 _cgiByClient; // client fd -> Cgi still receiving streamed body
            std::vector<Cgi*>                   _cgiToClose;
    };
}

#endif // WEBSERV_EXEC_SERVER_HPP
