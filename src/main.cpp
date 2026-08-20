/**
 * @ Author: yaycicek
 * @ Create Time: 2026-05-27 / 22:18:27
 * @ Modified by: akosaca
 * @ Modified time: 2026-08-20 / 15:56:16
 */

#include <string>
#include <csignal>
#include "config/Lexer.hpp"
#include "config/Parser.hpp"
#include "config/Router.hpp"
#include "utils/arg.hpp"
#include "utils/io.hpp"
#include "exec/Server.hpp"

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    try {
        arg::Parser     args(argc, argv);

        config::Lexer   lexer;
        config::Parser  parser(lexer.tokenize(args.getConfigFileContent()));
        config::Router  router(parser.parse());
        exec::Server    server(router);
        server.run();
    } catch (const config::Exception& e) {
        io::println(std::string("Config Error: ") + e.what());
        return (1);
    } catch (const std::exception& e) {
        io::println(std::string("System Error: ") + e.what());
        return (1);
    }
    return (0);
}
