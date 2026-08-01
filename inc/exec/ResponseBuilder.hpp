/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-01 / 14:21:08
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-01 / 14:41:42
 */


#include "utils/str.hpp"
#include "http/Status.hpp"
#include <string>

namespace exec {
    class ResponseBuilder {
        public:
            ResponseBuilder();
            ~ResponseBuilder();
    
            std::string build(http::status::Code status, const std::string& body, const std::string& contentType);
        private:
            std::string buildStatusLine(http::status::Code status) const;
            std::string buildHeaders(const std::string& body, const std::string& contentType) const;
    };
}