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
        // Declared Content-Length from the request headers. `body` may only
        // hold a prefix of this while streaming in the rest, so CONTENT_LENGTH
        // must come from here, not body.size().
        std::size_t contentLength;

        RequestData() : contentLength(0) {}
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

            void    run(const RequestData& req, const std::string& interpreter, const std::string& scriptPath); //cgi başlatmak için. initial işlemler gerçekleştiririz
            void    onWritable(); //yazıyorken çağırırız
            void    onReadable(); // okuyorken çağııryoruz
            // Streaming input support: more body bytes can be appended as they
            // arrive from the client instead of needing the whole body upfront.
            void    feed(const std::string& chunk);
            void    finishInput(); // no more body is coming; close stdin once drained
            std::size_t pendingInputBytes() const; // bytes queued but not yet written to the pipe
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
            // buffering the whole (possibly huge) response before sending it —
            // otherwise every concurrent large CGI transfer holds its full
            // output in memory at once, which is what actually made the
            // server run out of headroom under a heavy concurrent load.
            const std::string& peekOutput() const; // inspect without consuming (e.g. to find the header/body separator)
            void        dropOutputPrefix(std::size_t n); // drop the first n bytes (the header block, once parsed)
            std::string takeOutput(); // drain and return whatever's newly accumulated
            bool        headersRelayed() const;
            void        setHeadersRelayed();

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
            bool        _inputDone; // true once no more feed() calls will come
            std::string _output;
            bool        _hadAnyOutput; // _output may already be drained via takeOutput() by the time we hit EOF
            bool        _headersRelayed;
            // Timestamp of the last time this CGI made forward progress (spawn,
            // a body chunk fed to it, a write to its stdin, or a read from its
            // stdout). isTimedOut() measures idle time from this, not a hard cap
            // on the whole request's duration — otherwise a slow-but-steady
            // large transfer (a real, legitimate client, just a slow one) gets
            // killed mid-stream even though it's still making progress.
            time_t _lastActivity;
    };
}

#endif // WEBSERV_EXEC_CGI_HPP 
