/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-06 / 20:22:57
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-15 / 13:54:37
 */


#ifndef WEBSERV_EXEC_CGI_HPP
#define WEBSERV_EXEC_CGI_HPP

#include <string>
#include <map>
#include <vector>
#include <sys/types.h> 
#include <ctime>

namespace exec {
    enum State {
        NOT_STARTED,
        WRITING,
        READING,
        DONE,
        FAILED
    };
    struct RequestData {
        std::string method;
        std::string path;
        std::string query;
        std::map<std::string, std::string> headers;
        std::string body;
        // Declared Content-Length from the request headers. `body` may only hold
        // a prefix of this (the rest streams in later), so CGI env must use this,
        // not body.size().
        std::size_t contentLength;
        // The location's configured client_max_body_size, carried along so the
        // streaming path can still enforce it for chunked bodies (whose total
        // size isn't known upfront from a Content-Length header).
        std::size_t maxBodySize;

        RequestData() : contentLength(0), maxBodySize(0) {}
    };
    struct CgiInfo {
        bool        isCgi;
        std::string interpreter;
        std::string scriptPath;
        RequestData reqData;
    };
    class Cgi {
        public:
            Cgi(int clientFd);
            ~Cgi();

            void    run(RequestData& req, const std::string& interpreter, const std::string& scriptPath); //cgi başlatmak için. initial işlemler gerçekleştiririz
            void    onWritable(); //yazıyorken çağırırız
            void    onReadable(); // okuyorken çağııryoruz
            // Streaming input support: more body bytes can be appended as they
            // arrive from the client, without needing the whole body upfront.
            void    feed(const std::string& chunk);
            void    finishInput(); // no more body will ever be fed; close stdin once drained
            std::size_t pendingInputBytes() const;
            void    setMaxInputBytes(std::size_t n);
            bool    inputOverLimit() const; // total fed so far exceeds the configured max
            State   getState() const; //durum paylaşmak için
            int     getInFd()  const; //in fd sayısını örğenmek için
            int     getOutFd() const; //out fd sayısını örğenmek için
            void    cleanup(); //temizlik yapmka için
            int getClientFd() const;
            const std::string& rawOutput() const;
            bool isTimedOut(int limitSec) const;
            pid_t getPid() const;
            void setTimedOut();

            // Output streaming support, mirroring the input side: lets Server
            // relay the CGI's stdout to the client as it arrives instead of
            // buffering the whole (possibly huge) response before sending it.
            const std::string& peekOutput() const;   // inspect without consuming (e.g. to look for the header/body separator)
            void dropOutputPrefix(std::size_t n);     // drop the first n bytes (the header block, once parsed)
            std::string takeOutput();                 // drain+return whatever's newly accumulated
            bool headersRelayed() const;
            void setHeadersRelayed();

        private:
            Cgi(const Cgi&);
            Cgi& operator=(const Cgi&);

            std::vector<std::string> buildEnv(const RequestData& req, const std::string& scriptPath) const;
            int _clientFd;
            State   _state;
            int     _inWrFd;
            int     _outRdFd;
            pid_t   _pid;
            std::string _input;
            std::size_t _inputOffset;
            bool        _inputDone;
            std::size_t _totalFed;
            std::size_t _maxInputBytes; // 0 = unlimited
            std::string _output;
            bool        _hadAnyOutput;
            bool        _headersRelayed;
            // Timestamp of the last time this CGI made forward progress (spawn,
            // a body chunk fed to it, a write to its stdin, or a read from its
            // stdout) — isTimedOut() is an *idle* timeout measured from this,
            // not a hard cap on the whole request's duration, so a slow-but-
            // steady large transfer is never killed just for taking a while.
            time_t _lastActivity;
    };
}

#endif // WEBSERV_EXEC_CGI_HPP 
