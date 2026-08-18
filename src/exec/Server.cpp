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
    // tells us whether the whole request body is already known (info.reqData.body
    // holds it all) or whether more will arrive later via feedCgiStream().
    void Server::spawnCgiStream(int fd, CgiInfo& info, bool bodyComplete) {
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
            _poller.addFd(inFd, POLLOUT);
            _cgi[inFd] = cgi;
        }
        if (bodyComplete) {
            // Nothing left to stream in; stop reading and wait for the CGI's response.
            _poller.setFdEvents(fd, 0);
        } else {
            // Keep reading the client so onReadable() keeps handing us more body.
            _cgiByClient[fd] = cgi;
        }
    }

    // As soon as a request's headers are ready, decide whether it targets a CGI
    // location, before its (possibly huge) body has fully arrived. Feeding the
    // body straight into the CGI's stdin pipe as it streams in, instead of
    // buffering the whole thing first, is what keeps memory bounded regardless
    // of upload size / concurrency.
    // Returns true if the request has been fully handled by this call (either an
    // error response was queued, or a CGI stream was started) — the caller must
    // not run any further routing on it. Returns false only when this isn't a
    // CGI location at all, so the caller should fall back to normal handling.
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
            // Whatever body prepareCgi() already picked up is now inside info.reqData;
            // drop the connection's own copy of it.
            conCl->clearRequestBody();
            spawnCgiStream(fd, info, bodyComplete);
            return (true);
        } catch (const std::exception& e) {
            conCl->setResponse("HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
            _poller.setFdEvents(fd, POLLOUT);
            return (true);
        }
    }

    // Drains whatever new body bytes the parser picked up in the last onReadable()
    // into the already-running CGI, and applies backpressure: if the child isn't
    // draining fast enough we stop reading more from the client until it catches up.
    void Server::feedCgiStream(int fd, exec::Connection* conCl, Cgi* cgi) {
        std::string chunk = conCl->takeAvailableBody();
        if (!chunk.empty()) cgi->feed(chunk);
        // Covers chunked bodies, whose total size isn't known upfront from a
        // Content-Length header so it can't be rejected before spawning the CGI.
        if (cgi->inputOverLimit()) {
            kill(cgi->getPid(), SIGKILL);
            std::map<int, Cgi*>::iterator it = _cgi.begin();
            while (it != _cgi.end()) {
                if (it->second == cgi) {
                    _poller.deleteFd(it->first);
                    std::map<int, Cgi*>::iterator toErase = it++;
                    _cgi.erase(toErase);
                } else ++it;
            }
            _cgiByClient.erase(fd);
            // The CGI may already be echoing output back (headers committed to
            // "200 OK" chunked) by the time we notice the input overran the
            // limit; setResponse() would then stomp mid-stream data with an
            // unrelated non-chunked body. Best effort in that case: just end
            // the chunked stream early instead of pretending we can still send
            // a clean 413.
            if (cgi->headersRelayed()) {
                conCl->appendStreamChunk("0\r\n\r\n");
                conCl->finishStreamResponse();
            } else {
                conCl->setResponse("HTTP/1.1 413 Content Too Large\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
            }
            cgi->cleanup();
            delete cgi;
            _poller.setFdEvents(fd, POLLOUT);
            return;
        }
        if (conCl->isRequestComplete()) {
            cgi->finishInput();
            _cgiByClient.erase(fd);
            // Only drop POLLIN: if output has already started streaming back,
            // POLLOUT must stay registered so the response keeps flowing.
            _poller.setFdEvents(fd, cgi->headersRelayed() ? POLLOUT : 0);
            return;
        }
        if (cgi->pendingInputBytes() > CGI_INPUT_HIGH_MARK) {
            _poller.setFdEvents(fd, cgi->headersRelayed() ? POLLOUT : 0);
        }
    }

    void Server::handleCl(int fd, short revents) {
        exec::Connection *conCl = _connections[fd];
        if (revents & POLLIN) {
            conCl->onReadable();

            if (conCl->getState() == CLOSING) {
                // Client vanished mid-request. If it had a CGI stream in progress,
                // leave the CGI running (it self-cleans on completion/timeout) but
                // stop tracking it against this now-dead connection.
                _cgiByClient.erase(fd);
                _toClose.push_back(fd);
                return;
            }

            // NOTE: none of the branches below may `return` early — a connection
            // that's simultaneously streaming CGI input and output can have both
            // POLLIN and POLLOUT set in `revents` at once, and the POLLOUT
            // handling at the bottom of this function must still run in that case.
            std::map<int, Cgi*>::iterator streaming = _cgiByClient.find(fd);
            if (streaming != _cgiByClient.end()) {
                feedCgiStream(fd, conCl, streaming->second);
            } else {
                bool handled = conCl->getRequest().isHeadersReady() && !conCl->getRequest().hasError()
                             && dispatchCgi(fd, conCl); // response queued or CGI stream started
                if (!handled) {
                    if (conCl->isRequestComplete()) {
                        try {
                            const config::ServerBlock& sb = _config.getServerBlock(_clAddr[fd]);
                            std::string res = _executor.execute(sb, conCl->getRequest());
                            // The request body has already been used to build `res`;
                            // drop the connection's own copy right away instead of
                            // holding onto it.
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
        }
    }

    void Server::handleCgi(int fd) {
        Cgi* cgi = _cgi[fd];
     /*aaççç*/   //if (fd == cgi->getInFd()) cgi->onWritable();
        bool wasInFd = (fd == cgi->getInFd()); /******** */
        if (wasInFd) cgi->onWritable(); /************* */
        else if (fd == cgi->getOutFd()) {
            cgi->onReadable();
            // Relay whatever just arrived (and, on the very first bytes, the
            // headers) straight to the client instead of buffering the whole
            // CGI output — this is what keeps a large echoed response from
            // ballooning memory the same way unbounded input buffering used to.
            relayCgiOutput(cgi);
        }
        if (wasInFd) {
            if (cgi->getInFd() == -1) { /*************** */
                _poller.deleteFd(fd);
                _cgi.erase(fd);
            }
            // The child just drained some (or all) of the queued input: resume
            // reading from the client if it was paused for backpressure.
            if (cgi->pendingInputBytes() < CGI_INPUT_LOW_MARK) {
                std::map<int, Cgi*>::iterator sit = _cgiByClient.find(cgi->getClientFd());
                if (sit != _cgiByClient.end() && sit->second == cgi) {
                    // Preserve POLLOUT if the response is already streaming back.
                    short events = cgi->headersRelayed() ? (POLLIN | POLLOUT) : POLLIN;
                    _poller.setFdEvents(cgi->getClientFd(), events);
                }
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
            _cgiToClose.push_back(cgi);
        }
    }

    // Drains whatever the CGI has newly produced and relays it to the client.
    // The first bytes are buffered (bounded by the real size of the CGI's own
    // header block, which is tiny) until the header/body separator is found;
    // from then on every subsequent chunk is forwarded immediately as an
    // HTTP/1.1 chunked-encoding piece, so we never need the whole (possibly
    // huge) response body in memory at once.
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
            // The client may still be mid-upload (this connection could still be
            // in _cgiByClient, streaming body in) while output starts flowing
            // back out already, so keep POLLIN too in that case instead of
            // clobbering it.
            bool stillReceiving = _cgiByClient.count(cgi->getClientFd()) != 0;
            _poller.setFdEvents(cgi->getClientFd(), stillReceiving ? (POLLIN | POLLOUT) : POLLOUT);
        }
        std::string body = cgi->takeOutput();
        if (!body.empty()) conCl->appendStreamChunk(encodeChunk(body));
        // Deliberately NOT backpressured: pausing here when the client hasn't
        // drained its send backlog would stop us reading the CGI's stdout, which
        // (for a CGI that reads-then-writes synchronously, like a simple echo)
        // stops it reading its stdin, which stalls our writes to it, which
        // engages *input* backpressure and stops us reading the client — but a
        // plain HTTP/1.1 client is allowed to write its whole request before
        // reading any response, so nothing would ever unstick that chain: a
        // genuine deadlock, only ever broken by the CGI timeout killing the
        // request. Always draining output avoids that; the tradeoff is that a
        // slow/non-concurrent-reading client lets `_wrBuf` grow up to the
        // response size for that one connection, same as the old design, but
        // input-side streaming still bounds the common case.
    }

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
            // Neither of these can be trusted/reused: we don't know the total
            // size upfront (that's the whole point of streaming), and we set
            // our own Transfer-Encoding below.
            if (lower.compare(0, 15, "content-length:") == 0) continue;
            if (lower.compare(0, 18, "transfer-encoding:") == 0) continue;
            outHeaders += line + "\r\n";
        }
        return ("HTTP/1.1 " + statusLine + "\r\n" + outHeaders +
                "Transfer-Encoding: chunked\r\n" "Connection: close\r\n\r\n");
    }

    std::string Server::encodeChunk(const std::string& data) const {
        if (data.empty()) return ("");
        std::stringstream ss;
        ss << std::hex << data.size();
        return (ss.str() + "\r\n" + data + "\r\n");
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
        std::string statusPrefix = "HTTP/1.1 " + statusLine + "\r\n" + outHeaders +
        "Content-Length: " + str::to_string(body.size()) + "\r\n" +
        "Connection: close\r\n\r\n";
        std::string result;
        result.reserve(statusPrefix.size() + body.size());
        result += statusPrefix;
        result += body;
        return result;
     /*aççç*/   // std::stringstream res;
        // res << "HTTP/1.1 " << statusLine << "\r\n";
        // res << outHeaders;
        // res << "Content-Length: " << body.size() << "\r\n";
        // res << "\r\n";
        // res << body;
        // return res.str();
    }


    void Server::delCgi(Cgi* cgi) {
        std::map<int, Connection*>::iterator it = _connections.find(cgi->getClientFd());
        if (it != _connections.end()) {
            if (cgi->headersRelayed()) {
                // Already streaming: flush whatever's left and close out the
                // chunked body. If the CGI died mid-stream (FAILED) after having
                // already committed to "200 OK", the best we can do is end the
                // stream early — the client sees a truncated body, same as any
                // proxy would produce when its upstream drops mid-response.
                std::string remaining = cgi->takeOutput();
                if (!remaining.empty()) it->second->appendStreamChunk(encodeChunk(remaining));
                it->second->appendStreamChunk("0\r\n\r\n");
                it->second->finishStreamResponse();
            } else {
                std::string res;
                if (cgi->getState() == DONE) res = cgiToHttp(cgi->rawOutput());
                else res = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
                it->second->setResponse(res);
            }
            _poller.setFdEvents(cgi->getClientFd(), POLLOUT);
        }
        // In case this CGI died (timeout/error) while still mid-stream, stop
        // tracking it against its client so feedCgiStream() is never called again
        // on a Cgi that's about to be destroyed.
        std::map<int, Cgi*>::iterator cit = _cgiByClient.find(cgi->getClientFd());
        if (cit != _cgiByClient.end() && cit->second == cgi) _cgiByClient.erase(cit);
        cgi->cleanup();
        delete cgi;
    }

    void Server::run() {
        while (true) {
            std::vector<pollfd> pl = _poller.pollReady(120);
            for (size_t i = 0; i < pl.size(); ++i) {
                int fd = pl[i].fd;
                // pl is a snapshot taken before this loop started: handling an
                // earlier entry can finish/clean up a CGI (or close a connection)
                // whose fd also has a later, now-stale entry in this same batch.
                // Re-check membership at dispatch time instead of assuming
                // "not a listener, not a CGI fd => must still be a live client",
                // which would otherwise default-construct (and use) a NULL
                // Connection* for a fd nothing owns anymore.
                if (_lsAddr.count(fd))              acceptCl(fd);
                else if (_cgi.count(fd))             handleCgi(fd);
                else if (_connections.count(fd))     handleCl(fd, pl[i].revents);
            }
            // Collect distinct timed-out Cgi*s first: a single Cgi has two entries
            // in _cgi (its in-fd and out-fd), so acting while iterating would both
            // queue it for cleanup twice (double free via delCgi) and never drop
            // its now-stale fd entries from _cgi/_poller.
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
