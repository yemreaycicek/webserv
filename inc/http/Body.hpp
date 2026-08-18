/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-24 / 12:54:45
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-24 / 13:14:09
 */

#ifndef WEBSERV_HTTP_BODY_HPP
#define WEBSERV_HTTP_BODY_HPP

#include <string>

namespace http {
    class Body {
        public:
            Body();
            Body(const Body& other);
            Body& operator=(const Body& other);
            ~Body();

            bool parse(std::string& buffer, std::size_t contentLength, bool isChunked);

            const std::string& getContent() const;
            void clear();
            std::string takeContent();
    
        private:
            std::string _content;            
    };
}

#endif // WEBSERV_HTTP_BODY_HPP
