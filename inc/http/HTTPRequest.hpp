/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-23 / 13:10:50
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-23 / 13:40:54
 */

#ifndef WEBSERV_HTTP_HTPP_REQUEST_HPP
#define WEBSERV_HTTP_HTPP_REQUEST_HPP

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

    class HTTPRequest {
        public:
            HTTPRequest();
            HTTPRequest(const HTTPRequest& other);
            HTTPRequest& operator=(const HTTPRequest& other);
            ~HTTPRequest();

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

#endif // WEBSERV_HTTP_HTPP_REQUEST_HPP
