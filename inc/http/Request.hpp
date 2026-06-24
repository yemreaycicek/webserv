/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-23 / 13:10:50
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-24 / 13:28:16
 */

#ifndef WEBSERV_HTPP_REQUEST_HPP
#define WEBSERV_HTPP_REQUEST_HPP

#include <map>
#include <string>

#include "http/RequestLine.hpp"
#include "http/Header.hpp"
#include "http/Body.hpp"

namespace http {
    enum State {
        STATE_REQUEST_LINE,
        STATE_HEADER,
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

            void parse(const std::string& chunk);
    
        private:
            State _state;
            std::string _rawBuffer;

            RequestLine _requestLine;
            Header _header;
            Body _body;
    };
}

#endif // WEBSERV_HTTP_REQUEST_HPP
