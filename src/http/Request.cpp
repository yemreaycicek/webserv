/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-23 / 13:20:59
 * @ Modified by: yaycicek
 * @ Modified time: 2026-08-04 / 18:24:32
 */

#include "http/Request.hpp"
#include "http/Status.hpp"
#include "http/Exception.hpp"

#include <stdexcept>
#include <string>

#ifdef DEBUG
  #include "utils/io.hpp"
#endif

namespace http {
    Request::Request() : _state(STATE_REQUEST_LINE), _errorCode(status::OK) {}
    Request::Request(const Request& other) : _state(other._state), _rawBuffer(other._rawBuffer), _requestLine(other._requestLine), _header(other._header), _body(other._body) {}
    
    Request& Request::operator=(const Request& other) {
        if (this != &other) {
            _state = other._state;
            _rawBuffer = other._rawBuffer;
            _errorCode = other._errorCode;
            _requestLine = other._requestLine;
            _header = other._header;
            _body = other._body;
        }
        return ((*this));
    }
    
    Request::~Request() {}

    void Request::parse(const std::string& chunk) {
        _rawBuffer += chunk;

        try {
            while (_state != STATE_COMPLETE && _state != STATE_ERROR) {
                switch (_state) {
                    case STATE_REQUEST_LINE:
                        if (_requestLine.parse(_rawBuffer)) _state = STATE_HEADER;
                        else return;
                        break;
                    case STATE_HEADER:
                        if (_header.parse(_rawBuffer)) _state = STATE_BODY;
                        else return;
                        break;
                    case STATE_BODY:
                        if (_body.parse(_rawBuffer, _header.getContentLength(), _header.isChunked())) _state = STATE_COMPLETE;
                        else return;
                        break;
                    default:
                        return;
                }
            }
        } catch (const http::Exception& e) {
            _state = STATE_ERROR;
            _errorCode = e.getStatusCode();
            #ifdef DEBUG
                io::errln(std::string("HTTP parse error: ") + e.what());
            #endif
        } catch (const std::exception& e) {
            _state = STATE_ERROR;
            _errorCode = status::INTERNAL_SERVER_ERROR;
            #ifdef DEBUG
                io::errln(std::string("HTTP internal error: ") + e.what());
            #endif
        }
    }

    bool Request::isComplete() const {
        return (_state == STATE_COMPLETE);
    }

    bool Request::hasError() const {
        return (_state == STATE_ERROR);
    }

    Method Request::getMethod() const {
        return (_requestLine.getMethod());
    }

    const std::string& Request::getUri() const {
        return (_requestLine.getUri());
    }

    const std::string& Request::getBody() const {
        return (_body.getContent());
    }

    std::string Request::getHeader(const std::string& key) const {
        return (_header.get(key));
    }

    void Request::validateHeaders() const {
        if (_requestLine.getVersion() == "HTTP/1.1" && !_header.has("host")) {
            throw http::Exception(http::status::BAD_REQUEST, "Missing Host header field");
        }

        if (_header.has("transfer-encoding") && _header.has("content-length")) {
            throw http::Exception(http::status::BAD_REQUEST, "Both Transfer-Encoding and Content-Length present");
        }
    }
}

