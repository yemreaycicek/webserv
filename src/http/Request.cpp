/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-23 / 13:20:59
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-24 / 13:16:25
 */

#include "http/Request.hpp"

#include <string>

namespace http {
    Request::Request() : _state(STATE_REQUEST_LINE), _method(UNKNOWN) {}
    Request::Request(const Request& other) : _state(other._state), _method(other._method) {}
    
    Request& Request::operator=(const Request& other) {
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
    
    Request::~Request() {}

    void Request::parse() {
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

    void Request::parseRequestLine() {
        // ...
    }

        void Request::parseHeaders() {
        // ...
    }

        void Request::parseBody() {
        // ...
    }
}

