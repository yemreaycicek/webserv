/**
 * @ Author: yaycicek
 * @ Create Time: 2026-05-27 / 22:18:27
 * @ Modified by: yaycicek
 * @ Modified time: 2026-07-04 / 18:40:07
 */

#include <string>
#include <vector>

#include "config/Lexer.hpp"
#include "config/Parser.hpp"
#include "config/Router.hpp"
#include "utils/arg.hpp"
#include "utils/io.hpp"

int main(int argc, char **argv)
{
    try {
        arg::Parser args(argc, argv);

        config::Lexer lexer;
        std::vector<config::Token> tokens = lexer.tokenize(args.getConfigFileContent());

        config::Parser parser(tokens);
        std::vector<config::ServerBlock> servers = parser.parse();

        config::Router router(servers);
    } catch (const config::Parser::SyntaxError& e) {
        io::println(std::string("Config Error: ") + e.what());
        return (1);
    } catch (const std::exception& e) {
        io::println(std::string("System Error: ") + e.what());
        return (1);
    }
    return (0);
}
