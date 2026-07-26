/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-23 / 16:50:32
 * @ Modified by: yaycicek
 * @ Modified time: 2026-07-26 / 13:26:38
 */

#include "http/Header.hpp"

#include <cstdlib>
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

        std::string key = str::tolower(line.substr(0, colon));
        std::string value = str::trim(line.substr(colon + 1));
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

        char* endPtr;
        std::size_t contentLength = std::strtoul(value.c_str(), &endPtr, 10);
        if ((*endPtr) != '\0') {
            throw http::Exception(http::status::BAD_REQUEST, "Invalid Content-Length value");
        }
        return (contentLength);

    }

    bool Header::isChunked() const {
        std::string transferEncoding = get("transfer-encoding");
        return (transferEncoding.find("chunked") != std::string::npos);
    }
}
