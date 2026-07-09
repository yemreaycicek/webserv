/**
 * @ Author: yaycicek
 * @ Create Time: 2026-05-27 / 22:18:27
 * @ Modified by: yaycicek
 * @ Modified time: 2026-07-09 / 14:45:53
 */

#include <string>

#include "config/Lexer.hpp"
#include "config/Parser.hpp"
#include "config/Router.hpp"
#include "utils/arg.hpp"
#include "utils/io.hpp"

int main(int argc, char **argv)
{
    try {
        arg::Parser     args(argc, argv);

        config::Lexer   lexer;
        config::Parser  parser(lexer.tokenize(args.getConfigFileContent()));
        config::Router  router(parser.parse());
    } catch (const config::Exception& e) {
        io::println(std::string("Config Error: ") + e.what());
        return (1);
    } catch (const std::exception& e) {
        io::println(std::string("System Error: ") + e.what());
        return (1);
    }
    return (0);
}
