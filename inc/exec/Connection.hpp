/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 17:44:34
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-02 / 17:44:26
 */

#ifndef WEBSERV_EXEC_CONNECTION_HPP
#define WEBSERV_EXEC_CONNECTION_HPP

#include "net/Socket.hpp"
#include "http/Request.hpp"
#include <string>

namespace exec {
    enum ConState {
        READING_REQUEST,
        SENDING_RESPONSE,
        CLOSING
    };

    class Connection {
        public:
            explicit Connection(int fd);
            ~Connection();

            const http::Request&    getRequest() const;
            int                     getFd() const;
            ConState                getState() const;
            void                    onReadable();
            void                    onWritable();
            bool                    isRequestComplete() const;
            void                    setResponse(const std::string& resp);
            void                    clearRequestBody();
            std::string             takeAvailableBody();
            // Streaming response support (used for CGI output, so we never
            // need the whole thing in memory at once): start with the
            // status line + headers, append body bytes as they become
            // available, then mark it finished once no more is coming. No
            // chunk framing — the response carries no Content-Length and the
            // connection is closed once finished, which is what tells the
            // client where the body ends (standard, valid HTTP/1.1 for a
            // Connection: close response).
            void                    beginStreamResponse(const std::string& head);
            void                    appendStreamChunk(const std::string& data);
            void                    finishStreamResponse();
            bool                    hasPendingOutput() const;
        private:
            net::Socket             _socket;
            std::string             _wrBuf;
            bool                    _wrComplete; // false while more data may still be appended
            http::Request           _request;
            ConState                _state;
            Connection(const Connection& other);
            Connection& operator=(const Connection& other);
            
    };
}

#endif // WEBSERV_EXEC_CONNECTION_HPP 
