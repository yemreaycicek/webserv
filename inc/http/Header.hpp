/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-23 / 16:47:15
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-13 / 19:15:13
 */

#ifndef WEBSERV_HTTP_HEADER_HPP
#define WEBSERV_HTTP_HEADER_HPP

#include <map>
#include <string>

namespace http {
    class Header {
        public:
            Header();
            Header(const Header& other);
            Header& operator=(const Header& other);
            ~Header();

            bool parse(std::string& buffer);

            bool has(const std::string& key) const;
            std::string get(const std::string& key) const;

            std::size_t getContentLength() const;
            bool isChunked() const;

            const std::map<std::string, std::string>& getHeaders() const; //! check

        private:
            std::map<std::string, std::string> _fields;

            void addHeaderLine(const std::string& line);

            bool isUniqueField(const std::string& key) const;
    };
}

#endif // WEBSERV_HTTP_HEADER_HPP
