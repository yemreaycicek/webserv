/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-01 / 14:40:55
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-01 / 15:05:51
 */

#include "utils/str.hpp"
#include "http/Status.hpp"
#include <string>
#include "exec/ResponseBuilder.hpp"

namespace exec {
    ResponseBuilder::ResponseBuilder() {}
    ResponseBuilder::~ResponseBuilder() {}

    std::string ResponseBuilder::buildStatusLine(http::status::Code status) const {
        return ("HTTP/1.1" + str::to_string(status) + " " + http::status::getReasonPhrase(status) + "\r\n"); 
    }
}