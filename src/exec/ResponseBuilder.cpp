/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-01 / 14:40:55
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-06 / 13:26:27
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

    std::string ResponseBuilder::buildRedirect(http::status::Code status, const std::string& location) const {
        std::string res = buildStatusLine(status);
        res += "Location: " + location + "\r\n";
        res += "Content-Length: 0\r\n";
        res += "Connection: close\r\n";
        res += "\r\n";
        return res;
    }
}