/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-24 / 12:54:45
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-24 / 12:56:28
 */

#ifndef WEBSERV_HTTP_HTTP_BODY_HPP
#define WEBSERV_HTTP_HTTP_BODY_HPP

#include <string>

namespace http {
    class HTTPBody {
        public:
            HTTPBody();
            HTTPBody(const HTTPBody& other);
            HTTPBody& operator=(const HTTPBody& other);
            ~HTTPBody();

            bool parse(std::string& buffer, std::size_t contentLength, bool isChunked);

            const std::string& getContent() const;
    
        private:
            std::string _content;            
    };
}

#endif // WEBSERV_HTTP_HTTP_BODY_HPP
