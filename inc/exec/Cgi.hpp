/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-06 / 20:22:57
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-14 / 15:02:08
 */


#ifndef WEBSERV_EXEC_CGI_HPP
#define WEBSERV_EXEC_CGI_HPP

#include <string>
#include <map>
#include <vector>
#include <sys/types.h> 

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
        private:
            Cgi(const Cgi&);
            Cgi& operator=(const Cgi&);

            std::vector<std::string> buildEnv(const RequestData& req) const;
            int _clientFd;
            State   _state;
            int     _inWrFd;
            int     _outRdFd;
            pid_t   _pid;
            std::string _input;
            std::size_t _inputOffset;
            std::string _output;
    };
}

#endif // WEBSERV_EXEC_CGI_HPP 
