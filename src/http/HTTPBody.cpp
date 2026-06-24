/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-24 / 12:54:35
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-24 / 13:06:55
 */

#include "http/HTTPBody.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace http {
    HTTPBody::HTTPBody() {}
    HTTPBody::HTTPBody(const HTTPBody& other) : _content(other._content) {}

    HTTPBody& HTTPBody::operator=(const HTTPBody& other) {
        if (this != &other) {
            _content = other._content;
        }
        return ((*this));
    }

    HTTPBody::~HTTPBody() {}

    bool HTTPBody::parse(std::string& buffer, std::size_t contentLength, bool isChunked) {
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
                    throw std::runtime_error("400 Bad Request: Invalid chunk size hex");
                }

                if (buffer.length() < crlf + 2 + chunkSize + 2) {
                    return (false);
                } else if (buffer.substr(crlf + 2 + chunkSize, 2) != "\r\n") {
                    throw std::runtime_error("400 Bad Request: Malformed chunked body (missing CRLF)");
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
            } else if (buffer.length() <= contentLength) {
                _content += buffer.substr(0, contentLength);
                buffer.erase(0, contentLength);
                return (true);
            }
        }
        return (false);
    }
}
