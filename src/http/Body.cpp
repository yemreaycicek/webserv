/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-24 / 12:54:35
 * @ Modified by: yaycicek
 * @ Modified time: 2026-08-03 / 16:06:56
 */

#include "http/Body.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>

#include "http/Exception.hpp"
#include "http/Status.hpp"

namespace http {
    Body::Body() {}
    Body::Body(const Body& other) : _content(other._content) {}

    Body& Body::operator=(const Body& other) {
        if (this != &other) {
            _content = other._content;
        }
        return ((*this));
    }

    Body::~Body() {}

    bool Body::parse(std::string& buffer, std::size_t contentLength, bool isChunked) {
        if (isChunked) {
            while (true) {
                std::size_t crlf = buffer.find("\r\n");
                if (crlf == std::string::npos) {
                    return (false);
                }

                std::string hexStr = buffer.substr(0, crlf);
                char *endPtr;
                std::size_t chunkSize = std::strtoul(hexStr.c_str(), &endPtr, 16);
                if (endPtr == hexStr.c_str()) {
                    throw http::Exception(http::status::BAD_REQUEST, "Invalid chunk size hex");
                }

                if (buffer.length() < crlf + 2 + chunkSize + 2) {
                    return (false);
                } else if (buffer.substr(crlf + 2 + chunkSize, 2) != "\r\n") {
                    throw http::Exception(http::status::BAD_REQUEST, "Malformed chunked body (missing CRLF)");
                }
                
                if (chunkSize == 0) {
                    buffer.erase(0, crlf + 4);
                    return (true);
                }

                _content += buffer.substr(crlf + 2, chunkSize);
                buffer.erase(0, crlf + 2 + chunkSize + 2);
            }
        } else {
            if (contentLength == 0) {
                return (true);
            } else if (buffer.length() >= contentLength) {
                _content += buffer.substr(0, contentLength);
                buffer.erase(0, contentLength);
                return (true);
            }
        }
        return (false);
    }

    const std::string& Body::getContent() const {
        return (_content);
    }
}
