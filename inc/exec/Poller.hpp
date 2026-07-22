/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-21 / 22:57:26
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-22 / 17:04:23
 */

#ifndef WEBSERV_EXEC_POLLER_HPP
#define WEBSERV_EXEC_POLLER_HPP

#include "exec/Exception.hpp"

#include <poll.h>
#include <vector>

namespace exec {
    class Poller {
        public:
            Poller();
            ~Poller();
            void                addFd(int fd, short events);
            void                deleteFd(int fd);
            void                setFdEvents(int fd, short events);
            std::vector<pollfd> pollReady(int timeout);
            class PollError : public Exception{
                public:
                    PollError(const std::string& message);
                    ~PollError() throw();
            };
        private:
            std::vector<pollfd> _fds;
    };
}

#endif // WEBSERV_EXEC_POLLER_HPP 
