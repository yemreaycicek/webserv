#include "config/Exception.hpp"

#include <stdexcept>
#include <string>

namespace config {
    Exception::Exception(const std::string& message) : std::runtime_error(message) {}
    Exception::~Exception() throw() {}
}
