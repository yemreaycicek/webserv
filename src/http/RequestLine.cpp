/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-23 / 13:51:32
 * @ Modified by: yaycicek
 * @ Modified time: 2026-08-03 / 16:01:01
 */

#include "http/RequestLine.hpp"

#include <stdexcept>
#include <string>

#include "http/Exception.hpp"
#include "http/Status.hpp"

namespace http {
    RequestLine::RequestLine() : _method(UNKNOWN) {}
    RequestLine::RequestLine(const RequestLine& other) : _method(other._method) {}
    
    RequestLine& RequestLine::operator=(const RequestLine& other) {
        if (this != &other) {
            _method = other._method;
            _uri = other._uri;
            _version = other._version;
        }
        return ((*this));
    }
    
    RequestLine::~RequestLine() {}

    bool RequestLine::parse(std::string& buffer) {
        std::size_t crlf = buffer.find("\r\n");
        if (crlf == std::string::npos) {
            return (false);
        }

        std::string line = buffer.substr(0, crlf);
        buffer.erase(0, crlf + 2);

        std::size_t firstSpace = line.find(' ');
        if (firstSpace == std::string::npos) {
            throw http::Exception(http::status::BAD_REQUEST, "Malformed request line (missing URI)");
        }
        std::size_t secondSpace = line.find(' ', firstSpace + 1);
        if (secondSpace == std::string::npos) {
            throw http::Exception(http::status::BAD_REQUEST, "Malformed request line (missing HTTP version)");
        }
        
        std::string methodString = line.substr(0, firstSpace);
        _method = getMethod();
        if (_method == UNKNOWN) {
            throw http::Exception(http::status::NOT_IMPLEMENTED, "Unknown HTTP Method '" + methodString + "'");
        }

        _uri = line.substr(firstSpace + 1, secondSpace - (firstSpace + 1));

        _version = line.substr(secondSpace + 1);
        if (_version.find(' ') != std::string::npos) {
            throw http::Exception(http::status::BAD_REQUEST, "Trailing garbage in request line");
        }
        if (_version != "HTTP/1.1" && _version != "HTTP/1.0") {
            throw http::Exception(http::status::HTTP_VERSION_NOT_SUPPORTED, _version);
        }

        return (true);
    }

    Method RequestLine::getMethod() const {
        return (_method);
    }
}
