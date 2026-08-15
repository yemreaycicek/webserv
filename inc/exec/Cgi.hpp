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
            State   getState() const; //durum paylaşmak için
            int     getInFd()  const; //in fd sayısını örğenmek için
            int     getOutFd() const; //out fd sayısını örğenmek için
            void    cleanup(); //temizlik yapmka için
            int getClientFd() const;
            const std::string& rawOutput() const;
            bool isTimedOut(int limitSec) const;
            pid_t getPid() const;
            void setTimedOut();

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
            std::string _output;
            time_t _startedAt;
    };
}

#endif // WEBSERV_EXEC_CGI_HPP 
