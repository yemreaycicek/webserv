/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-23 / 13:10:50
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-24 / 13:15:24
 */

#ifndef WEBSERV_HTPP_REQUEST_HPP
#define WEBSERV_HTPP_REQUEST_HPP

#include <map>
#include <string>

namespace http {
    enum Method {
        GET,
        POST,
        DELETE,
        UNKNOWN
    };

    enum State {
        STATE_REQUEST_LINE,
        STATE_HEADERS,
        STATE_BODY,
        STATE_COMPLETE,
        STATE_ERROR
    };

    class Request {
        public:
            Request();
            Request(const Request& other);
            Request& operator=(const Request& other);
            ~Request();

            void parse();
    
        private:
            State _state;

            Method _method;
            std::string _uri;
            std::string _version;
            std::map<std::string, std::string> _headers;
            std::string _body;

            void parseRequestLine();
            void parseHeaders();
            void parseBody();
    };
}

#endif // WEBSERV_HTTP_REQUEST_HPP
