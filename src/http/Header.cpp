/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-23 / 16:50:32
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-13 / 19:16:02
 */

#include "http/Header.hpp"

#include <stdexcept>
#include <string>

#include "http/Exception.hpp"
#include "http/Status.hpp"
#include "utils/str.hpp"

namespace http {
    Header::Header() {}
    Header::Header(const Header& other) : _fields(other._fields) {}

    Header& Header::operator=(const Header& other) {
        if (this != &other) {
            _fields = other._fields;
        }
        return ((*this));
    }

    Header::~Header() {}

    bool Header::parse(std::string& buffer) {
        while (true) {
            std::size_t crlf = buffer.find("\r\n");
            if (crlf == std::string::npos) {
                return (false);
            } else if (crlf == 0) {
                buffer.erase(0, 2);
                return (true);
            }

            std::string line = buffer.substr(0, crlf);
            buffer.erase(0, crlf + 2);
            addHeaderLine(line);
        }
    }

    void Header::addHeaderLine(const std::string& line) {
        std::size_t colon = line.find(':');
        if (colon == std::string::npos) {
            throw http::Exception(http::status::BAD_REQUEST, "Malformed header line (missing colon)");
        }
        if (colon == 0 || std::isspace(line.at(colon - 1))) {
             throw http::Exception(http::status::BAD_REQUEST, "Whitespace between header field name and colon");
        }

        std::string key = str::tolower(line.substr(0, colon));
        std::string value = str::trim(line.substr(colon + 1));

        if (isUniqueField(key) && has(key)) {
            throw http::Exception(http::status::BAD_REQUEST, "Duplicate '" + key + "' header field");
        }
        _fields[key] = value;
    }

    bool Header::has(const std::string& key) const {
        return (_fields.find(str::tolower(key)) != _fields.end());
    }

    std::string Header::get(const std::string& key) const {
        std::map<std::string, std::string>::const_iterator it = _fields.find(str::tolower(key));
        if (it == _fields.end()) {
            return ("");
        }
        return (it->second);
    }

    std::size_t Header::getContentLength() const {
        std::string value = get("content-length");
        if (value.empty()) {
            return (0);
        }

        for (std::size_t i = 0; i < value.length(); i++) {
            if (!std::isdigit(value.at(i))) {
                throw http::Exception(http::status::BAD_REQUEST, "Invalid Content-Length value");
            }
        }
        
        std::size_t contentLength;
        if (!str::to_numeric(value, contentLength)) {
            throw http::Exception(http::status::BAD_REQUEST, "Content-Length value out of range");
        }
        return (contentLength);
    }

    bool Header::isChunked() const {
        std::string value = get("transfer-encoding");
        if (value.length() == 0) {   
            return (false);
        }

        std::string lastCoding = 0;
        std::size_t start = 0;
        while (start <= value.length()) {
            std::size_t comma = value.find(",", start);
            if (comma == std::string::npos) {
                comma = value.length();
            }

            std::string coding = str::tolower(str::trim(value.substr(start, comma - start)));
            if (!coding.empty()) {
                lastCoding = coding;
            }
            start = comma + 1;
        }

        if (lastCoding != "chunked") {
            throw http::Exception(http::status::BAD_REQUEST, "Transfer-Encoding must end with the chunked coding");
        }
        return (true);
    }

    bool Header::isUniqueField(const std::string& key) const {
        return (key == "host" || key == "content-length");
    }

    const std::map<std::string, std::string>& Header::getHeaders() const {
        return (_fields);
    }
}
