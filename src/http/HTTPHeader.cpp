/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-23 / 16:50:32
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-23 / 17:52:22
 */

#include "http/HTTPHeader.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>

#include "utils/str.hpp"

namespace http {
    HTTPHeader::HTTPHeader() {}
    HTTPHeader::HTTPHeader(const HTTPHeader& other) : _fields(other._fields) {}

    HTTPHeader& HTTPHeader::operator=(const HTTPHeader& other) {
        if (this != &other) {
            _fields = other._fields;
        }
        return ((*this));
    }

    HTTPHeader::~HTTPHeader() {}

    bool HTTPHeader::parse(std::string& buffer) {
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

    void HTTPHeader::addHeaderLine(const std::string& line) {
        std::size_t colon = line.find(' ');
        if (colon == std::string::npos) {
            throw std::runtime_error("400 Bad Request: Malformed header line (missing colon)");
        }

        std::string key = str::tolower(line.substr(0, colon));
        std::string value = str::trim(line.substr(0, colon));
        _fields[key] = value;
    }

    bool HTTPHeader::has(const std::string& key) const {
        return (_fields.find(str::tolower(key)) != _fields.end());
    }

    std::string HTTPHeader::get(const std::string& key) const {
        std::map<std::string, std::string>::const_iterator it = _fields.find(str::tolower(key));
        if (it == _fields.end()) {
            return ("");
        }
        return (it->second);
    }

    std::size_t HTTPHeader::getContentLength() const {
        std::string value = get("content-length");
        if (value.empty()) {
            return (0);
        }

        char* endPtr;
        std::size_t contentLength = std::strtoul(value.c_str(), &endPtr, 10);
        if ((*endPtr) != '\0') {
            throw std::runtime_error("400 Bad Request: Invalid Content-Length value");
        }
        return (contentLength);

    }

    bool HTTPHeader::isChunked() const {
        std::string transferEncoding = get("transfer-encoding");
        return (transferEncoding.find("chunked") != std::string::npos);
    }
}
