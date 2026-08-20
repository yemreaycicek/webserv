/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-23 / 13:10:50
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-20 / 15:59:31
 */

#ifndef WEBSERV_HTPP_REQUEST_HPP
#define WEBSERV_HTPP_REQUEST_HPP

#include <map>
#include <string>

#include "http/RequestLine.hpp"
#include "http/Header.hpp"
#include "http/Body.hpp"
#include "http/Status.hpp"

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

            bool isComplete() const;
            bool hasError() const;
            status::Code getErrorCode() const; //! check
            Method getMethod() const;
            const std::string& getUri() const;
            const std::string& getBody() const;
            void clearBody();
            std::string takeBody();
            std::string getHeader(const std::string& key) const;
            bool isHeadersReady() const;
            std::size_t getContentLength() const;

            const std::map<std::string, std::string>& getHeaders() const; //! check
    
        private:
            State _state;
            std::string _rawBuffer;
            status::Code _errorCode;

            RequestLine _requestLine;
            Header _header;
            Body _body;

            void validateHeaders() const;
    };
}

#endif // WEBSERV_HTTP_REQUEST_HPP
