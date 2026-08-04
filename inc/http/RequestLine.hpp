/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-23 / 13:43:04
 * @ Modified by: yaycicek
 * @ Modified time: 2026-08-04 / 18:15:30
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

            Method getMethod() const;
            const std::string& getUri() const;
            const std::string& getVersion() const;
            void setMethod(const std::string& methodString);
    
        private:
            Method _method;
            std::string _uri;
            std::string _version;

    };
}

#endif // WEBSERV_HTTP_REQUEST_LINE_HPP
