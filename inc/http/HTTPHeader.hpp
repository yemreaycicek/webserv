#ifndef WEBSERV_HTTP_HTTP_HEADER_HPP
#define WEBSERV_HTTP_HTTP_HEADER_HPP

#include <map>
#include <string>

namespace http {
    class HTTPHeader {
        public:
            HTTPHeader();
            HTTPHeader(const HTTPHeader& other);
            HTTPHeader& operator=(const HTTPHeader& other);
            ~HTTPHeader();

            bool parse(std::string& buffer);

            bool has(const std::string& key) const;
            std::string get(const std::string& key) const;

            std::size_t getContentLength() const;
            bool isChunked() const;

        private:
            std::map<std::string, std::string> _fields;

            void addHeaderLine(const std::string& line);
    };
}

#endif // WEBSERV_HTTP_HTTP_HEADER_HPP
