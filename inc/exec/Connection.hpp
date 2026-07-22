/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 17:44:34
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-22 / 19:15:06
 */

#ifndef WEBSERV_EXEC_CONNECTION_HPP
#define WEBSERV_EXEC_CONNECTION_HPP

#include "net/Socket.hpp"
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
            int                 getFd() const;
            ConState            getState() const;
            void                onReadable();
            void                onWritable();
            bool                isRequestComplete() const;
            void                setResponse(const std::string& resp);
            const std::string&  getRequestData() const;
        private:
            net::Socket _socket;
            std::string _rdBuf;
            std::string _wrBuf;
            ConState    _state;
            Connection(const Connection& other);
            Connection& operator=(const Connection& other);
            
    };
}

#endif // WEBSERV_EXEC_CONNECTION_HPP 
