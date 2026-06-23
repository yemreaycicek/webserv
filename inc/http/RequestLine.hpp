/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-23 / 13:43:04
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-23 / 17:52:18
 */

#ifndef WEBSERV_HTTP_REQUEST_LINE_HPP
#define WEBSERV_HTTP_REQUEST_LINE_HPP

#include <string>

namespace http {
    enum Method {
        GET,
        POST,
        DELETE,
        UNKNOWN
    };

    class RequestLine {
        public:
            RequestLine();
            RequestLine(const RequestLine& other);
            RequestLine& operator=(const RequestLine& other);
            ~RequestLine();

            bool parse(std::string& requestLine);
    
        private:
            Method _method;
            std::string _uri;
            std::string _version;

            Method stringToMethod(const std::string& methodString) const;
    };
}

#endif // WEBSERV_HTTP_REQUEST_LINE_HPP
