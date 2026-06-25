/**
 * @ Author: yaycicek
 * @ Create Time: 2026-06-24 / 15:45:24
 * @ Modified by: yaycicek
 * @ Modified time: 2026-06-25 / 13:46:21
 */

#include "http/Exception.hpp"

#include <stdexcept>
#include <string>

#include "utils/str.hpp"

namespace http {
    Exception::Exception(status::Code statusCode, const std::string& detail) : std::runtime_error(str::to_string(statusCode) + " " + status::getReasonPhrase(statusCode) + ": " + detail), _statusCode(statusCode) {}
    Exception::~Exception() throw() {}

    status::Code Exception::getStatusCode() const {
        return (_statusCode);
    }
}
