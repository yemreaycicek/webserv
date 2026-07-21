/**
 * @ Author: akosaca
 * @ Create Time: 2026-07-22 / 00:36:02
 * @ Modified by: akosaca
 * @ Modified time: 2026-07-22 / 00:39:53
 */


#include "exec/Exception.hpp"

#include <stdexcept>
#include <string>

namespace exec {
    Exception::Exception(const std::string& message) : std::runtime_error(message) {}
    Exception::~Exception() throw() {}
}