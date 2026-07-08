#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/application.hpp>
#include <game_req_analyzer/core/config.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/utils/platform_utils.hpp>

using namespace game_req;

int main(int argc, char* argv[]) {
    try {
        Application app;
        return app.run(argc, argv);
    } catch (const std::exception& e) {
        fmt::print(stderr, fmt::fg(fmt::terminal_color::red), "Fatal error: {}\n", e.what());
        return 1;
    } catch (...) {
        fmt::print(stderr, fmt::fg(fmt::terminal_color::red), "Unknown fatal error\n");
        return 1;
    }
}