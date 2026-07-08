#include <game_req_analyzer/ui/terminal_ui.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <fmt/format.h>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace game_req;

ProgressDisplay::ProgressDisplay(TerminalUI& ui, StringView title, u64 total)
    : ui_(ui), title_(title), total_(total) {}

void ProgressDisplay::start() {
    start_time_ = std::chrono::steady_clock::now();
    started_ = true;
    finished_ = false;
    
    // Show initial progress
    ui_.show_progress(0.0f, fmt::format("Starting {}...", title_));
}

void ProgressDisplay::update(u64 current, StringView status) {
    if (!started_) return;
    
    current_ = current;
    
    // Calculate progress
    f32 progress = total_ > 0 ? static_cast<f32>(current_) / total_ : 0.0f;
    progress = std::clamp(progress, 0.0f, 1.0f);
    
    // Format status message
    std::string status_str;
    if (!status.empty()) {
        status_str = std::string(status);
    } else {
        status_str = fmt::format("{}/{}", current_, total_);
    }
    
    // Update progress bar
    ui_.show_progress(progress, status_str);
}

void ProgressDisplay::finish(bool success) {
    if (!started_) return;
    
    finished_ = true;
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
    
    // Show final progress
    ui_.show_progress(1.0f, success ? 
                      fmt::format("{} completed in {}", title_, format_duration(duration)) : 
                      fmt::format("{} failed after {}", title_, format_duration(duration)));
    
    // Add a newline to move to next line
    fmt::print("\n");
}

void ProgressDisplay::set_total(u64 total) {
    total_ = total;
    // If we've already started and current exceeds new total, adjust
    if (started_ && current_ > total_) {
        current_ = total_;
    }
}

void ProgressDisplay::draw_bar(f32 progress) {
    // This is handled by the TerminalUI::show_progress method now
    // Keeping this method for interface compatibility
}

String ProgressDisplay::format_eta() const {
    if (!started_ || total_ == 0) {
        return "Calculating...";
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    
    if (elapsed.count() == 0 || current_ == 0) {
        return "Calculating...";
    }
    
    // Calculate estimated total time
    double elapsed_seconds = static_cast<double>(elapsed.count());
    double progress = static_cast<double>(current_) / total_;
    
    if (progress <= 0.0) {
        return "Calculating...";
    }
    
    double estimated_total = elapsed_seconds / progress;
    double remaining = estimated_total - elapsed_seconds;
    
    if (remaining <= 0) {
        return "Just a moment...";
    }
    
    // Format remaining time
    int hours = static_cast<int>(remaining) / 3600;
    int minutes = (static_cast<int>(remaining) % 3600) / 60;
    int seconds = static_cast<int>(remaining) % 60;
    
    std::ostringstream oss;
    if (hours > 0) {
        oss << std::setw(2) << std::setfill('0') << hours << ":"
            << std::setw(2) << std::setfill('0') << minutes << ":"
            << std::setw(2) << std::setfill('0') << seconds;
    } else if (minutes > 0) {
        oss << std::setw(2) << std::setfill('0') << minutes << ":"
            << std::setw(2) << std::setfill('0') << seconds;
    } else {
        oss << std::setw(2) << std::setfill('0') << seconds << "s";
    }
    
    return oss.str();
}
