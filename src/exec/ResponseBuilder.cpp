/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-01 / 14:40:55
 * @ Modified by: yaycicek
 * @ Modified time: 2026-08-04 / 16:40:04
 */

#include "utils/str.hpp"
#include "http/Status.hpp"
#include <string>
#include "exec/ResponseBuilder.hpp"

namespace exec {
    ResponseBuilder::ResponseBuilder() {}
    ResponseBuilder::~ResponseBuilder() {}

    std::string ResponseBuilder::buildStatusLine(http::status::Code status) const {
        return ("HTTP/1.1 " + str::to_string(status) + " " + http::status::getReasonPhrase(status) + "\r\n"); 
    }

    std::string ResponseBuilder::buildHeaders(const std::string& body, const std::string& contentType) const {
        return ("Content-Length: " + str::to_string(body.length()) + "\r\n" + "Content-Type: " + contentType + "\r\n" + "Connection: close\r\n");
    }

    std::string ResponseBuilder::build(http::status::Code status, const std::string& body, const std::string& contentType) const {
        return (buildStatusLine(status) + buildHeaders(body, contentType) + "\r\n" + body);
    }
}