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
            // Streaming response support (used for CGI output, so we never need
            // the whole body in memory at once): start with the status+headers,
            // append body chunks as they become available, then mark it finished
            // once no more data is coming.
            void                    beginStreamResponse(const std::string& head);
            void                    appendStreamChunk(const std::string& chunk);
            void                    finishStreamResponse();
            std::size_t             pendingSendBytes() const;
        private:
            net::Socket             _socket;
            std::string             _wrBuf;
            std::size_t             _wrOffset; //****** */
            bool                    _wrComplete; // false while more data may still be appended
            http::Request           _request;
            ConState                _state;
            Connection(const Connection& other);
            Connection& operator=(const Connection& other);
            
    };
}

#endif // WEBSERV_EXEC_CONNECTION_HPP 
