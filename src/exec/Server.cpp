/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 20:11:29
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-15 / 15:20:29
 */

#include "exec/Server.hpp"
#include "net/Address.hpp"
#include "utils/str.hpp"
#include "exec/Executor.hpp"
#include "http/Status.hpp"
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
        _cgiByClient.erase(fd);
    }

    // Spawns the CGI child and wires its pipes into the poller. `bodyComplete`
    // says whether the whole request body is already known (info.reqData.body
    // holds it all) or whether more will still arrive later via feedCgiStream().
    void Server::buildCgi(int fd, CgiInfo& info, bool bodyComplete) {
        Cgi* cgi = new Cgi(fd);
        cgi->run(info.reqData, info.interpreter, info.scriptPath);
        if (cgi->getState() == FAILED) {
            delete cgi;
            std::map<int, exec::Connection*>::iterator it = _connections.find(fd);
            if (it != _connections.end()) {
                it->second->setResponse("HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                _poller.setFdEvents(fd, POLLOUT);
            }
            return;
        }
        if (bodyComplete) cgi->finishInput();
        int outFd = cgi->getOutFd();
        _poller.addFd(outFd, POLLIN);
        _cgi[outFd] = cgi;
        if (cgi->getInFd() != -1) {
            int inFd = cgi->getInFd();
            // A pipe with room reports POLLOUT-ready on every single poll()
            // call whether or not we actually have anything queued to write —
            // so if we're still waiting on more feed() calls and there's
            // nothing queued yet, register with no events instead of POLLOUT,
            // or that's an instantly-ready fd forever, i.e. a busy loop.
            // finishInput() (above) always leaves something to do (the close),
            // so bodyComplete always wants POLLOUT on.
            short events = (bodyComplete || cgi->pendingInputBytes() > 0) ? POLLOUT : 0;
            _poller.addFd(inFd, events);
            _cgi[inFd] = cgi;
        }
        if (bodyComplete) {
            // Nothing left to stream in; stop reading the client and wait for
            // the CGI's response.
            _poller.setFdEvents(fd, 0);
        } else {
            // Keep reading the client so onReadable() keeps handing us more
            // body via feedCgiStream().
            _cgiByClient[fd] = cgi;
        }
    }

    bool Server::dispatchCgi(int fd, exec::Connection* conCl) {
        try {
            const config::ServerBlock& sb = _config.getServerBlock(_clAddr[fd]);
            CgiInfo info;
            std::string errRes;
            CgiDispatch d = _executor.prepareCgi(sb, conCl->getRequest(), info, errRes);
            if (d == CGI_NONE) return (false);
            if (d == CGI_ERROR) {
                conCl->clearRequestBody();
                conCl->setResponse(errRes);
                _poller.setFdEvents(fd, POLLOUT);
                return (true);
            }
            bool bodyComplete = conCl->isRequestComplete();
            // Whatever body prepareCgi() already picked up is now inside
            // info.reqData; drop the connection's own copy of it.
            conCl->clearRequestBody();
            buildCgi(fd, info, bodyComplete);
            return (true);
        } catch (const std::exception& e) {
            conCl->setResponse("HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
            _poller.setFdEvents(fd, POLLOUT);
            return (true);
        }
    }

    void Server::feedCgiStream(int fd, exec::Connection* conCl, Cgi* cgi) {
        std::string chunk = conCl->takeAvailableBody();
        cgi->feed(chunk);
        bool complete = conCl->isRequestComplete();
        if (complete) cgi->finishInput();
        // There's now something to do on the CGI's stdin fd (fresh bytes to
        // write, or — once finishInput() ran — the close): make sure POLLOUT
        // is on, since buildCgi()/a previous empty round may have paused it.
        if (cgi->getInFd() != -1 && (!chunk.empty() || complete)) {
            _poller.setFdEvents(cgi->getInFd(), POLLOUT);
        }
        if (complete) {
            _cgiByClient.erase(fd);
            _poller.setFdEvents(fd, 0);
        }
    }

    void Server::handleCl(int fd, short revents) {
        exec::Connection *conCl = _connections[fd];
        if (revents & POLLIN) {
            conCl->onReadable();
            if (conCl->getState() == CLOSING) {
                // Client vanished (or aborted) before completing a request:
                // onReadable() already flagged this. Without closing it here,
                // this fd would stay registered for POLLIN forever, and a
                // socket at EOF reports POLLIN-ready on every single poll()
                // call — a tight, always-instantly-ready busy loop pegging a
                // whole CPU core, not just a leaked connection.
                _cgiByClient.erase(fd);
                _toClose.push_back(fd);
                return;
            }
            std::map<int, Cgi*>::iterator streaming = _cgiByClient.find(fd);
            if (streaming != _cgiByClient.end()) {
                feedCgiStream(fd, conCl, streaming->second);
            }
            else {
                // As soon as headers are ready, decide whether this is a CGI
                // location — before the (possibly large) body has fully
                // arrived — so a big upload can stream straight into the CGI
                // instead of sitting fully buffered in the connection first.
                bool handled = conCl->getRequest().isHeadersReady() && !conCl->getRequest().hasError()
                             && dispatchCgi(fd, conCl);
                if (!handled) {
                    if (conCl->isRequestComplete()) {
                        try {
                            const config::ServerBlock& sb = _config.getServerBlock(_clAddr[fd]);
                            std::string res = _executor.execute(sb, conCl->getRequest());
                            conCl->clearRequestBody();
                            conCl->setResponse(res);
                            _poller.setFdEvents(fd, POLLOUT);
                        } catch(const std::exception& e) {
                            conCl->setResponse("HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                            _poller.setFdEvents(fd, POLLOUT);
                        }
                    }
                    else if (conCl->getRequest().hasError()) {
                        http::status::Code code = conCl->getRequest().getErrorCode();
                        std::string res = "HTTP/1.1 " + str::to_string(code) + " " + http::status::getReasonPhrase(code) + "\r\n" +
                        "Content-Length: 0\r\n"
                        "Connection: close\r\n\r\n";
                        conCl->setResponse(res);
                        _poller.setFdEvents(fd, POLLOUT);
                    }
                }
            }
        }
        if (revents & POLLOUT) {
            conCl->onWritable();
            if (conCl->getState() == CLOSING){
                _toClose.push_back(fd);
            }
            else if (!conCl->hasPendingOutput()) {
                // A streaming response (CGI output relayed as it arrives) can
                // have nothing queued right now between two chunks, while
                // still very much in progress — pause POLLOUT, or a writable
                // socket with nothing to send reports ready on every single
                // poll() call, a busy loop. relayCgiOutput()/feedCgiStream()
                // re-enable it once there's something to send again.
                _poller.setFdEvents(fd, _cgiByClient.count(fd) ? POLLIN : 0);
            }
        }
    }

    void Server::handleCgi(int fd) {
        Cgi* cgi = _cgi[fd];
        bool wasInFd = (fd == cgi->getInFd());
        if (wasInFd) cgi->onWritable();
        else if (fd == cgi->getOutFd()) {
            cgi->onReadable();
            // Relay whatever just arrived (and, on the very first bytes, the
            // headers) straight to the client instead of buffering the whole
            // CGI output — see relayCgiOutput() for why that matters under
            // concurrent load.
            relayCgiOutput(cgi);
        }
        if (wasInFd) {
            if (cgi->getInFd() == -1) {
                // Input side just finished and closed itself (see
                // Cgi::onWritable()): drop just this fd now instead of
                // waiting for the whole CGI to finish, or this closed fd
                // number stays registered with the poller and keeps firing
                // for nothing.
                _poller.deleteFd(fd);
                _cgi.erase(fd);
            } else if (cgi->pendingInputBytes() == 0) {
                // Drained everything queued and still waiting on more
                // feed() calls (finishInput() hasn't run — that always
                // leaves the close to do, handled above): pause POLLOUT here
                // too, or this pipe reports writable-forever and pegs a CPU
                // core doing nothing until the next feed() call re-enables
                // it (see Server::feedCgiStream()).
                _poller.setFdEvents(fd, 0);
            }
        }
        if (cgi->getState() == DONE || cgi->getState() == FAILED) {
            std::map<int, Cgi*>::iterator it = _cgi.begin();
            while (it != _cgi.end()) {
                if (it->second == cgi) {
                    _poller.deleteFd(it->first);
                    std::map<int, Cgi*>::iterator toErase = it++;
                    _cgi.erase(toErase);
                } else ++it;
            }
            // If this CGI died (timeout/error) while still mid-stream, stop
            // tracking it against its client so feedCgiStream() is never
            // called again on a Cgi that's about to be destroyed.
            std::map<int, Cgi*>::iterator cit = _cgiByClient.find(cgi->getClientFd());
            if (cit != _cgiByClient.end() && cit->second == cgi) _cgiByClient.erase(cit);
            _cgiToClose.push_back(cgi);
        }
    }

    // Drains whatever the CGI has newly produced and relays it to the client.
    // The first bytes are buffered only until the header/body separator is
    // found (bounded by the real size of the CGI's own header block, which is
    // tiny); from then on every chunk is forwarded as soon as it arrives, so
    // we never need the whole (possibly huge) response body in memory at
    // once — buffering it all is what let 20-odd concurrent large CGI
    // transfers add up to more memory than was actually available.
    void Server::relayCgiOutput(Cgi* cgi) {
        std::map<int, Connection*>::iterator cit = _connections.find(cgi->getClientFd());
        if (cit == _connections.end()) return; // client already gone; let the CGI run to completion and self-clean
        Connection* conCl = cit->second;

        if (!cgi->headersRelayed()) {
            const std::string& raw = cgi->peekOutput();
            std::string::size_type pos = raw.find("\r\n\r\n");
            std::size_t sepLen = 4;
            if (pos == std::string::npos) {
                pos = raw.find("\n\n");
                sepLen = 2;
            }
            if (pos == std::string::npos) return; // header block not complete yet
            std::string head = buildStreamedCgiHead(raw.substr(0, pos));
            cgi->dropOutputPrefix(pos + sepLen);
            cgi->setHeadersRelayed();
            conCl->beginStreamResponse(head);
            // The client may still be mid-upload (this connection could
            // still be in _cgiByClient, streaming body in) while output
            // starts flowing back out already, so keep POLLIN too in that
            // case instead of clobbering it.
            short events = _cgiByClient.count(cgi->getClientFd()) ? (POLLIN | POLLOUT) : POLLOUT;
            _poller.setFdEvents(cgi->getClientFd(), events);
        }
        std::string body = cgi->takeOutput();
        if (body.empty()) return;
        conCl->appendStreamChunk(body);
        // Make sure POLLOUT is on: a previous round may have paused it once
        // _wrBuf ran dry (see the POLLOUT handling in handleCl()).
        short events = _cgiByClient.count(cgi->getClientFd()) ? (POLLIN | POLLOUT) : POLLOUT;
        _poller.setFdEvents(cgi->getClientFd(), events);
    }

    // Turns the CGI's own header block into an HTTP response head. No
    // Content-Length (the whole point is we don't know the total size up
    // front) and no chunk framing either — Connection: close plus the actual
    // socket close once the CGI finishes is what tells the client where the
    // body ends, which is standard, valid HTTP/1.1 for a non-keep-alive
    // response.
    std::string Server::buildStreamedCgiHead(const std::string& headerBlock) const {
        std::string statusLine = "200 OK";
        std::string outHeaders;
        std::stringstream hs(headerBlock);
        std::string line;
        while (std::getline(hs, line)) {
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            if (line.empty())
                continue;
            std::string lower = str::tolower(line);
            if (lower.compare(0, 7, "status:") == 0) {
                std::string val = str::trim(line.substr(7));
                if (!val.empty()) statusLine = val;
                continue;
            }
            // Neither of these can be trusted/reused: we don't know the
            // total size upfront (that's the whole point of streaming), and
            // chunk framing isn't used here at all (see the comment above).
            if (lower.compare(0, 15, "content-length:") == 0) continue;
            if (lower.compare(0, 18, "transfer-encoding:") == 0) continue;
            outHeaders += line + "\r\n";
        }
        return ("HTTP/1.1 " + statusLine + "\r\n" + outHeaders + "Connection: close\r\n\r\n");
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
        std::string statusLine = "200 OK";
        std::string outHeaders;
        std::stringstream hs(headers);
        std::string line;
        while (std::getline(hs, line)) {
            if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
            if (line.empty())
            continue;
            std::string lower = str::tolower(line);
            if (lower.compare(0, 7, "status:") == 0) {
                std::string val = str::trim(line.substr(7));
                if (!val.empty())
                statusLine = val;
                continue;
            }
            if (lower.compare(0, 15, "content-length:") == 0)
            continue;
            outHeaders += line + "\r\n";
        }
        std::stringstream res;
        res << "HTTP/1.1 " << statusLine << "\r\n";
        res << outHeaders;
        res << "Content-Length: " << body.size() << "\r\n";
        // Without this, a client that assumes the connection stays open (the
        // default in HTTP/1.1) can reuse it right after a CGI response and get
        // "connection reset by peer" once we actually close it.
        res << "Connection: close\r\n";
        res << "\r\n";
        res << body;
        return res.str();
    }


    void Server::delCgi(Cgi* cgi) {
        std::map<int, Connection*>::iterator it = _connections.find(cgi->getClientFd());
        if (it != _connections.end()) {
            if (cgi->headersRelayed()) {
                // Already streaming: flush whatever's left and let the
                // response end (no framing to close out — see
                // buildStreamedCgiHead()). If the CGI died mid-stream
                // (FAILED) after already committing to "200 OK", the best we
                // can do is end it here — the client sees a truncated body,
                // same as any proxy would produce when its upstream drops
                // mid-response.
                std::string remaining = cgi->takeOutput();
                if (!remaining.empty()) it->second->appendStreamChunk(remaining);
                it->second->finishStreamResponse();
            } else {
                std::string res;
                if (cgi->getState() == DONE) res = cgiToHttp(cgi->rawOutput());
                else res = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
                it->second->setResponse(res);
            }
            _poller.setFdEvents(cgi->getClientFd(), POLLOUT);
        }
        cgi->cleanup();
        delete cgi;
    }

    void Server::run() {
        while (true) {
            std::vector<pollfd> pl = _poller.pollReady(120);
            for (size_t i = 0; i < pl.size(); ++i) {
                int fd = pl[i].fd;
                // pl is a snapshot taken before this loop started: handling an
                // earlier entry can already have closed a connection (or cleaned
                // up a CGI) whose fd also has a later, now-stale entry in this
                // same batch. Re-check membership instead of assuming "not a
                // listener, not a CGI fd => must still be a live client", which
                // would otherwise default-construct (and dereference) a NULL
                // Connection* via _connections[fd].
                if (_lsAddr.count(fd))              acceptCl(fd);
                else if (_cgi.count(fd))            handleCgi(fd);
                else if (_connections.count(fd))    handleCl(fd, pl[i].revents);
            }
            // Collect distinct timed-out Cgi*s first: a single Cgi has two
            // entries in _cgi (its in-fd and out-fd), so acting on it while
            // iterating would both queue it for cleanup twice (double free via
            // delCgi) and never drop its now-stale fd entries from _cgi/_poller.
            std::vector<Cgi*> timedOut;
            for (std::map<int, Cgi*>::const_iterator it = _cgi.begin(); it != _cgi.end(); ++it) {
                Cgi* cgi = it->second;
                if ((cgi->getState() == WRITING || cgi->getState() == READING) && cgi->isTimedOut(CGI_TIMEOUT_SEC)) {
                    bool already = false;
                    for (size_t k = 0; k < timedOut.size(); ++k) {
                        if (timedOut[k] == cgi) { already = true; break; }
                    }
                    if (!already) timedOut.push_back(cgi);
                }
            }
            for (size_t i = 0; i < timedOut.size(); ++i) {
                Cgi* cgi = timedOut[i];
                kill(cgi->getPid(), SIGKILL);
                cgi->setTimedOut();
                std::map<int, Cgi*>::iterator sub = _cgi.begin();
                while (sub != _cgi.end()) {
                    if (sub->second == cgi) {
                        _poller.deleteFd(sub->first);
                        std::map<int, Cgi*>::iterator toErase = sub++;
                        _cgi.erase(toErase);
                    } else ++sub;
                }
                std::map<int, Cgi*>::iterator cit = _cgiByClient.find(cgi->getClientFd());
                if (cit != _cgiByClient.end() && cit->second == cgi) _cgiByClient.erase(cit);
                _cgiToClose.push_back(cgi);
            }
            for (size_t i = 0; i < _toClose.size(); ++i) {
                delCl(_toClose[i]);
            }
            for (size_t i = 0; i < _cgiToClose.size(); ++i) {
                delCgi(_cgiToClose[i]);
            }
            _cgiToClose.clear();
            _toClose.clear();
        }
    }
}
