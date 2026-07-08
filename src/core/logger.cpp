#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/utils/platform_utils.hpp>
#include <game_req_analyzer/utils/time_utils.hpp>
#include <fmt/chrono.h>
#include <mutex>
#include <fstream>
#include <iostream>

using namespace game_req;

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Result<void> Logger::init(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    config_ = config;
    min_level_ = config.min_level;
    
    if (!config.log_file.empty()) {
        auto dir = config.log_file.parent_path();
        if (!dir.empty()) {
            std::error_code ec;
            fs::create_directories(dir, ec);
        }
        
        file_stream_.open(config.log_file, std::ios::app);
        if (!file_stream_.is_open()) {
            return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
                std::format("Failed to open log file: {}", config.log_file.string())));
        }
        
        current_file_size_ = fs::file_size(config.log_file);
        last_rotation_ = chrono::system_clock::now();
    }
    
    LOG_INFO("Logger initialized (level: {})", static_cast<u8>(min_level_));
    return success();
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (file_stream_.is_open()) {
        file_stream_.flush();
        file_stream_.close();
    }
}

void Logger::log_impl(Level level, StringView msg, std::source_location loc) {
    if (level < min_level_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    write_to_console(level, msg, loc);
    if (file_stream_.is_open()) {
        write_to_file(msg);
    }
}

void Logger::write_to_console(Level level, StringView msg, std::source_location loc) {
    if (!config_.color_output) {
        fmt::print("[{}] {}\n", 
            level == Level::Trace ? "TRC" :
            level == Level::Debug ? "DBG" :
            level == Level::Info  ? "INF" :
            level == Level::Warn  ? "WRN" :
            level == Level::Error ? "ERR" :
            level == Level::Critical ? "CRT" : "???", msg);
        return;
    }
    
    fmt::terminal_color color;
    StringView level_str;
    
    switch (level) {
        case Level::Trace:    color = fmt::terminal_color::bright_black; level_str = "TRC"; break;
        case Level::Debug:    color = fmt::terminal_color::blue;         level_str = "DBG"; break;
        case Level::Info:     color = fmt::terminal_color::green;        level_str = "INF"; break;
        case Level::Warn:     color = fmt::terminal_color::yellow;       level_str = "WRN"; break;
        case Level::Error:    color = fmt::terminal_color::red;          level_str = "ERR"; break;
        case Level::Critical: color = fmt::terminal_color::bright_red;   level_str = "CRT"; break;
        default:              color = fmt::terminal_color::white;        level_str = "???"; break;
    }
    
    if (config_.show_timestamp) {
        auto now = chrono::system_clock::now();
        auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
        
        fmt::print(fmt::fg(fmt::terminal_color::bright_black), "[{:%H:%M:%S}.{:03d}] ", 
            now, ms.count());
    }
    
    if (config_.show_level) {
        fmt::print(fmt::fg(color), "[{}] ", level_str);
    }
    
    if (config_.show_location) {
        fmt::print(fmt::fg(fmt::terminal_color::bright_black), "{}::{} ", 
            loc.file_name(), loc.function_name());
    }
    
    fmt::print("{}\n", msg);
}

void Logger::write_to_file(StringView msg) {
    auto now = chrono::system_clock::now();
    auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    file_stream_ << fmt::format("[{:%Y-%m-%d %H:%M:%S}.{:03d}] {}\n", 
        now, ms.count(), msg);
    
    current_file_size_ += msg.size() + 32;
    
    if (current_file_size_ > config_.max_file_size) {
        rotate_log_file();
    }
}

void Logger::rotate_log_file() {
    if (!file_stream_.is_open()) return;
    
    file_stream_.close();
    
    Path base = config_.log_file;
    Path rotated = base;
    rotated.replace_extension(base.extension().string() + ".1");
    
    for (int i = config_.max_files - 1; i >= 1; --i) {
        Path old = base;
        old.replace_extension(base.extension().string() + "." + std::to_string(i));
        Path new_path = base;
        new_path.replace_extension(base.extension().string() + "." + std::to_string(i + 1));
        
        if (fs::exists(old)) {
            fs::rename(old, new_path);
        }
    }
    
    if (fs::exists(base)) {
        fs::rename(base, rotated);
    }
    
    file_stream_.open(base, std::ios::app);
    current_file_size_ = 0;
    last_rotation_ = chrono::system_clock::now();
}