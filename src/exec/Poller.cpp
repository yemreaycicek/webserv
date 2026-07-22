/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-21 / 22:57:06
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-22 / 17:20:31
 */

#include "exec/Poller.hpp"

#include <poll.h>
#include <vector>
#include <stdexcept>

namespace exec {
    Poller::Poller() {}
    Poller::~Poller() {}

    void Poller::addFd(int fd, short events){
        pollfd new_fd;
        new_fd.fd = fd;
        new_fd.events = events;
        new_fd.revents = 0;
        _fds.push_back(new_fd);
    }

    void Poller::deleteFd(int fd){
        for (std::vector<pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it){
            if (it->fd == fd){
                _fds.erase(it);
                return;
            }
        }
    }

    void Poller::setFdEvents(int fd, short events){
        for (std::vector<pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it){
            if (it->fd == fd){
                it->events = events;
                return;
            }
        }
    }

    std::vector<pollfd> Poller::pollReady(int timeout){
        std::vector<pollfd> readyFds;
        if (_fds.empty()) return (readyFds);
        if (poll(&_fds[0], _fds.size(), timeout) < 0) throw PollError("Poller: Poll() failed");
        for (std::vector<pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it){
            if (it->revents != 0) readyFds.push_back(*it);
        }
        return (readyFds);
    }
    Poller::PollError::PollError(const std::string& message) : Exception(message) {}
    Poller::PollError::~PollError() throw() {}
}
