/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-23 / 13:20:59
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-23 / 17:52:26
 */

#include "http/HTTPRequest.hpp"

#include <string>

namespace http {
    HTTPRequest::HTTPRequest() : _state(STATE_REQUEST_LINE), _method(UNKNOWN) {}
    HTTPRequest::HTTPRequest(const HTTPRequest& other) : _state(other._state), _method(other._method) {}
    
    HTTPRequest& HTTPRequest::operator=(const HTTPRequest& other) {
        if (this != &other) {
            _state = other._state;
            _method = other._method;
            _uri = other._uri;
            _version = other._version;
            _headers = other._headers;
            _body = other._body;
        }
        return ((*this));
    }
    
    HTTPRequest::~HTTPRequest() {}

    void HTTPRequest::parse() {
        while (_state != STATE_COMPLETE && _state != STATE_ERROR) {
            switch (_state) {
                case STATE_REQUEST_LINE:
                    parseRequestLine();
                    break;
                case STATE_HEADERS:
                    parseHeaders();
                    break;
                case STATE_BODY:
                    parseBody();
                    break;
                default:
                    break;
            }
        }
    }

    void HTTPRequest::parseRequestLine() {
        // ...
    }

        void HTTPRequest::parseHeaders() {
        // ...
    }

        void HTTPRequest::parseBody() {
        // ...
    }
}

