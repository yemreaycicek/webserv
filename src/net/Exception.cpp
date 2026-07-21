/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-21 / 11:17:08
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-21 / 11:19:34
 */

#include "net/Exception.hpp"

#include <stdexcept>
#include <string>

namespace net {
    Exception::Exception(const std::string& message) : std::runtime_error(message) {}
    Exception::~Exception() throw() {}
}