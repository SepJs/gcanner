#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/config.hpp>
#include <fmt/format.h>
#include <fmt/color.h>
#include <chrono>
#include <mutex>
#include <fstream>
#include <source_location>

namespace game_req {

class Logger {
public:
    enum class Level : u8 {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Critical = 5,
        Off = 255
    };

    struct Config {
        Level min_level = Level::Info;
        bool color_output = true;
        bool show_timestamp = true;
        bool show_level = true;
        bool show_location = false;
        Path log_file;
        u64 max_file_size = 10 * MiB;
        u32 max_files = 5;
    };

    static Logger& instance();
    
    Result<void> init(const Config& config);
    void shutdown();
    
    template<typename... Args>
    void log(Level level, std::format_string<Args...> fmt, Args&&... args) {
        log_impl(level, std::format(fmt, std::forward<Args>(args)...), std::source_location::current());
    }
    
    template<typename... Args>
    void trace(std::format_string<Args...> fmt, Args&&... args) { log(Level::Trace, fmt, std::forward<Args>(args)...); }
    template<typename... Args>
    void debug(std::format_string<Args...> fmt, Args&&... args) { log(Level::Debug, fmt, std::forward<Args>(args)...); }
    template<typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args) { log(Level::Info, fmt, std::forward<Args>(args)...); }
    template<typename... Args>
    void warn(std::format_string<Args...> fmt, Args&&... args) { log(Level::Warn, fmt, std::forward<Args>(args)...); }
    template<typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args) { log(Level::Error, fmt, std::forward<Args>(args)...); }
    template<typename... Args>
    void critical(std::format_string<Args...> fmt, Args&&... args) { log(Level::Critical, fmt, std::forward<Args>(args)...); }

    void set_level(Level level) { min_level_ = level; }
    Level level() const { return min_level_; }

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log_impl(Level level, StringView msg, std::source_location loc);
    void write_to_console(Level level, StringView msg, std::source_location loc);
    void write_to_file(StringView msg);
    void rotate_log_file();
    
    Config config_;
    Level min_level_ = Level::Info;
    std::mutex mutex_;
    std::ofstream file_stream_;
    u64 current_file_size_ = 0;
    std::chrono::system_clock::time_point last_rotation_;
};

#define LOG_TRACE(...)   ::game_req::Logger::instance().trace(__VA_ARGS__)
#define LOG_DEBUG(...)   ::game_req::Logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...)    ::game_req::Logger::instance().info(__VA_ARGS__)
#define LOG_WARN(...)    ::game_req::Logger::instance().warn(__VA_ARGS__)
#define LOG_ERROR(...)   ::game_req::Logger::instance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::game_req::Logger::instance().critical(__VA_ARGS__)

} // namespace game_req