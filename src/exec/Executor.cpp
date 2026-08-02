/**
 * @ Author: akosaca
 * @ Create Time: 2026-08-02 / 14:05:15
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-02 / 14:10:56
 */

#include "exec/Executor.hpp"
#include "http/RequestLine.hpp"

namespace exec {
    Executor::Executor() {}
    Executor::~Executor() {}

    
    std::string Executor::execute(const config::ServerBlock& sb, const http::Request& r) {
        //Method m = r.get;
        http::Method m = r.getMethod();
        if (m == http::Method::GET) {
            handleGet(sb, r);
        }
    }
}