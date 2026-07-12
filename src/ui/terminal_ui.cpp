#include <game_req_analyzer/ui/terminal_ui.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/core/application.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

#define APP_VERSION "1.0.0"

namespace game_req {

TerminalUI::TerminalUI(const OutputConfig& config) : config_(config) {}

TerminalUI::~TerminalUI() = default;

void TerminalUI::initialize() {
    if (!initialized_) {
        initialized_ = true;
        detect_terminal_size();
        // Enable ANSI colors if requested features
        if (config_.enable_ansi_colors) {
            // ANSI colors are enabled by default on most terminals
        }
        if (config_.enable_unicode) {
            // UTF-8 is typically the default on modern systems
        }
    }
}

void TerminalUI::shutdown() {
    if (initialized_) {
        // Reset terminal attributes if needed
        initialized_ = false;
    }
}

void TerminalUI::show_banner() {
    if (!initialized_) return;
    
    if (config_.use_unicode_symbols) {
        fmt::print("\n");
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "╔════════════════════════════════════════════════════════════════════════════╗\n");
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "║                                                                     ║\n");
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "║  ███╗   ███╗ █████╗ ████████╗███████╗██████╗ ███╗   ███╗███████╗  ║\n");
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "║  ████╗ ████║██╔══██╗╚══██╔══╝██╔════╝██╔══██╗████╗ ████║██╔════╝  ║\n");
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "║  ██╔████╔██║███████║   ██║   █████╗  ██████╔╝██╔████╔██║█████╗   ║\n");
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "║  ██║╚██╔╝██║██╔══██║   ██║   ██╔══╝  ██╔══██╗██║╚██╔╝██║██╔══╝   ║\n");
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "║  ██║ ╚═╝ ██║██║  ██║   ██║   ███████╗██║  ██║██║ ╚═╝ ██║███████╗  ║\n");
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "║  ╚═╝     ╚═╝╚═╝  ╚═╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝  ║\n");
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "║                                                                     ║\n");
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "║            Premium Game System Requirements Analyzer               ║\n");
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "║                                                                     ║\n");
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "╚════════════════════════════════════════════════════════════════════════════╝\n\n");
    } else {
        fmt::print("\n");
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "==================================================================\n");
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "                    GameReqAnalyzer v{}                          \n", APP_VERSION);
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "           Premium Game System Requirements Analyzer               \n");
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "==================================================================\n\n");
    }
}

void TerminalUI::show_help() {
    if (!initialized_) return;
    
    fmt::print("Usage: game-req-analyzer [options] <path>\n\n");
    fmt::print("Options:\n");
    fmt::print("  -h, --help          Show this help message\n");
    fmt::print("  -v, --version       Show version information\n");
    fmt::print("  -o, --output <file> Output results to file\n");
    fmt::print("  -f, --format <fmt>  Output format (json, csv, markdown, html, text)\n");
    fmt::print("  -q, --quiet         Quiet mode (minimal output)\n");
    fmt::print("  -v, --verbose       Verbose output\n");
    fmt::print("  --no-color          Disable colored output\n");
    fmt::print("  --no-progress       Disable progress bar\n");
    fmt::print("  -i, --interactive   Run in interactive mode\n");
    fmt::print("\nExamples:\n");
    fmt::print("  game-req-analyzer /path/to/game\n");
    fmt::print("  game-req-analyzer -f json -o report.json /path/to/game\n");
    fmt::print("  game-req-analyzer --interactive\n\n");
}

void TerminalUI::show_progress(f32 progress, StringView status) {
    if (!initialized_ || !config_.show_progress) return;
    
    // Clamp progress between 0 and 1
    progress = std::clamp(progress, 0.0f, 1.0f);
    
    // Calculate bar width (leave room for percentage and status)
    int bar_width = std::max(20, term_width_ - 30);
    int filled = static_cast<int>(bar_width * progress);
    
    // Build the progress bar
    fmt::print("\r[");
    for (int i = 0; i < bar_width; ++i) {
        if (i < filled) {
            fmt::print(fg(fmt::color::green), "█");
        } else {
            fmt::print(fg(fmt::color::gray), "░");
        }
    }
    fmt::print(fmt::fg(fmt::color::white), "] {:3.0f}% {}", progress * 100.0f, status);
    fmt::print(fmt::fg(fmt::color::white), "\r");
    std::fflush(stdout);
}

void TerminalUI::show_scan_results(const ScanResult& result) {
    if (!initialized_) return;
    
    fmt::print(fg(fmt::color::green), "✓ Scan complete: {} files", result.file_count());
    if (result.total_size() >= 1024ULL * 1024 * 1024) {
        fmt::print(", {:.2f} GB", static_cast<double>(result.total_size()) / (1024*1024*1024));
    } else if (result.total_size() >= 1024ULL * 1024) {
        fmt::print(", {:.2f} MB", static_cast<double>(result.total_size()) / (1024*1024));
    } else {
        fmt::print(", {:.2f} KB", static_cast<double>(result.total_size()) / 1024);
    }
    fmt::print("\n");
    
    if (config_.verbose) {
        fmt::print("  Duration: {}\n", format_duration(result.duration()));
        fmt::print("  Throughput: {:.2f} MB/s\n", 
                   static_cast<double>(result.total_size()) / 1024 / 1024 / 
                   std::max(result.duration().count(), 1ms));
    }
}

void TerminalUI::show_analysis_results(const AggregatedAssets& assets) {
    if (!initialized_) return;
    
    fmt::print(fg(fmt::color::green), "✓ Analysis complete\n");
    
    if (config_.verbose || config_.show_details) {
        fmt::print("  Textures: {} ({} VRAM)\n", 
                   assets.textures.total_textures,
                   format_bytes(assets.textures.total_vram));
        fmt::print("  Models: {} ({} vertices)\n", 
                   assets.models.total_models,
                   format_number(assets.models.total_vertices));
        fmt::print("  Audio: {} files ({})\n", 
                   assets.audio.total_files,
                   format_bytes(assets.audio.total_size));
        fmt::print("  Scripts: {} files ({} lines)\n", 
                   assets.scripts.total_files,
                   format_number(assets.scripts.total_lines));
        fmt::print("  Executables: {} files\n", 
                   assets.executables.total_files);
        fmt::print("  Estimated CPU work: {} points\n", 
                   format_number(assets.total_cpu_work));
        fmt::print("  Estimated GPU work: {} points\n", 
                   format_number(assets.total_gpu_work));
        fmt::print("  Primary engine: {}\n", 
                   assets.primary_engine.empty() ? "Unknown" : assets.primary_engine);
    }
}

void TerminalUI::show_requirements(const RequirementResult& req) {
    if (!initialized_) return;
    
    fmt::print(fg(fmt::color::green), "✓ Requirements calculated\n\n");
    
    // Minimum requirements
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "MINIMUM REQUIREMENTS:\n");
    fmt::print("  OS: {}\n", req.minimum.os_minimum);
    fmt::print("  CPU: {} {} @ {:.1f}-{:.1f} GHz ({} cores/{} threads)\n", 
               req.minimum.cpu_vendor, req.minimum.cpu_arch,
               req.minimum.cpu_base_clock, req.minimum.cpu_boost_clock,
               req.minimum.cpu_cores, req.minimum.cpu_threads);
    fmt::print("  GPU: {} {} ({} GB VRAM)\n", 
               req.minimum.gpu_vendor, req.minimum.gpu_architecture,
               req.minimum.gpu_vram / 1024);
    fmt::print("  RAM: {} GB ({})\n", req.minimum.ram_capacity / 1024, req.minimum.ram_type);
    fmt::print("  Storage: {} {} ({} GB available)\n", 
               req.minimum.storage_type, req.minimum.storage_type, req.minimum.storage_capacity);
    fmt::print("  DirectX: {}\n", req.minimum.dx_version);
    fmt::print("\n");
    
    // Recommended requirements
    fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "RECOMMENDED REQUIREMENTS:\n");
    fmt::print("  OS: {}\n", req.recommended.os_recommended);
    fmt::print("  CPU: {} {} @ {:.1f}-{:.1f} GHz ({} cores/{} threads)\n", 
               req.recommended.cpu_vendor, req.recommended.cpu_arch,
               req.recommended.cpu_base_clock, req.recommended.cpu_boost_clock,
               req.recommended.cpu_cores, req.recommended.cpu_threads);
    fmt::print("  GPU: {} {} ({} GB VRAM)\n", 
               req.recommended.gpu_vendor, req.recommended.gpu_architecture,
               req.recommended.gpu_vram / 1024);
    fmt::print("  RAM: {} GB ({})\n", req.recommended.ram_capacity / 1024, req.recommended.ram_type);
    fmt::print("  Storage: {} {} ({} GB available)\n", 
               req.recommended.storage_type, req.recommended.storage_type, req.recommended.storage_capacity);
    fmt::print("  DirectX: {}\n", req.recommended.dx_version);
    
    if (config_.verbose || config_.show_details) {
        fmt::print("\n");
        fmt::print(fg(fmt::color::cyan), "CONFIDENCE: {:.1f}%\n", req.confidence * 100.0f);
    }
}

void TerminalUI::show_hardware_recommendations(const RequirementResult& req) {
    if (!initialized_) return;
    
    fmt::print(fg(fmt::color::green), "✓ Hardware recommendations generated\n\n");
    
    // CPU recommendations - using database query
    fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold, "CPU RECOMMENDATIONS:\n");
    fmt::print("  Minimum: {} {} ({} cores, {:.1f} GHz base)\n",
               req.minimum.cpu_vendor, req.minimum.cpu_arch,
               req.minimum.cpu_cores, req.minimum.cpu_base_clock);
    fmt::print("  Recommended: {} {} ({} cores, {:.1f} GHz base)\n",
               req.recommended.cpu_vendor, req.recommended.cpu_arch,
               req.recommended.cpu_cores, req.recommended.cpu_base_clock);
    fmt::print("\n");
    
    // GPU recommendations
    fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold, "GPU RECOMMENDATIONS:\n");
    fmt::print("  Minimum: {} {} ({} GB VRAM)\n",
               req.minimum.gpu_vendor, req.minimum.gpu_architecture,
               req.minimum.gpu_vram / 1024);
    fmt::print("  Recommended: {} {} ({} GB VRAM)\n",
               req.recommended.gpu_vendor, req.recommended.gpu_architecture,
               req.recommended.gpu_vram / 1024);
    fmt::print("\n");
}

void TerminalUI::show_comparison_table(const std::vector<const CPUEntry*>& cpus, const std::vector<const GPUEntry*>& gpus) {
    if (!initialized_) return;
    
    fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold, "HARDWARE COMPARISON\n");
    fmt::print("====================================================================================================================\n");
    
    // CPU comparison
    if (!cpus.empty()) {
        fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "CPUs:\n");
        fmt::print("{:<4} {:<25} {:<6} {:<6} {:<6} {:<8} {:<8} {:<8}\n", 
                   "#", "Model", "Cores", "Threads", "GHz", "Score", "$", "Perf/$");
        fmt::print("-------------------------------------------------------------------------------\n");
        for (size_t i = 0; i < cpus.size() && i < 5; ++i) {
            const auto& cpu = cpus[i];
            fmt::print("{:<4} {:<25} {:<6} {:<6} {:<6.1f} {:<8} ${:<7.1f} {:<8.2f}\n",
                       static_cast<int>(i+1),
                       cpu->name.substr(0, 24),
                       cpu->cores,
                       cpu->threads,
                       cpu->base_clock,
                       cpu->passmark_mt,
                       cpu->price_usd,
                       cpu->price_usd > 0 ? cpu->passmark_mt / cpu->price_usd : 0.0f);
        }
        fmt::print("\n");
    }
    
    // GPU comparison
    if (!gpus.empty()) {
        fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "GPUs:\n");
        fmt::print("{:<4} {:<25} {:<6} {:<6} {:<6} {:<8} {:<8} {:<8}\n", 
                   "#", "Model", "VRAM", "Bandw", "Clock", "Score", "$", "Perf/$");
        fmt::print("-------------------------------------------------------------------------------\n");
        for (size_t i = 0; i < gpus.size() && i < 5; ++i) {
            const auto& gpu = gpus[i];
            fmt::print("{:<4} {:<25} {:<6} {:<6} {:<6} {:<8} ${:<7.1f} {:<8.2f}\n",
                       static_cast<int>(i+1),
                       gpu->name.substr(0, 24),
                       gpu->vram / 1024,
                       static_cast<int>(gpu->vram_bandwidth),
                       static_cast<int>(gpu->boost_clock),
                       gpu->passmark_g3d,
                       gpu->price_usd,
                       gpu->price_usd > 0 ? gpu->passmark_g3d / gpu->price_usd : 0.0f);
        }
        fmt::print("\n");
    }
    
    fmt::print("====================================================================================================================\n");
}

void TerminalUI::show_error(const Error& err) {
    if (!initialized_) return;
    
    fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, "Error: {}\n", err.message);
    if (config_.verbose && !err.frames.empty()) {
        fmt::print(fg(fmt::color::red), "  Stack trace:\n");
        for (const auto& frame : err.frames) {
            fmt::print("    {}\n", frame);
        }
    }
}

void TerminalUI::show_warning(StringView msg) {
    if (!initialized_) return;
    
    fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "Warning: {}\n", msg);
}

void TerminalUI::show_info(StringView msg) {
    if (!initialized_) return;
    
    if (config_.verbose) {
        fmt::print(fg(fmt::color::blue) | fmt::emphasis::bold, "Info: {}\n", msg);
    }
}

void TerminalUI::show_success(StringView msg) {
    if (!initialized_) return;
    
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "Success: {}\n", msg);
}

bool TerminalUI::confirm(StringView prompt, bool default_yes) {
    if (!initialized_) return default_yes;
    
    fmt::print("{} [{}]: ", prompt, default_yes ? "Y/n" : "y/N");
    std::fflush(stdout);
    
    char response;
    std::cin >> std::noskipws >> response;
    
    // Clear any remaining input
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    // Convert to lowercase for comparison
    response = std::tolower(static_cast<unsigned char>(response));
    
    if (response == '\n' || response == '\r') {
        return default_yes;
    }
    
    return (response == 'y' && !default_yes) || (response == 'y' || response == '\n' || response == '\r');
}

String TerminalUI::input(StringView prompt, StringView default_value) {
    if (!initialized_) return default_value;
    
    fmt::print("{}: ", prompt);
    if (!default_value.empty()) {
        fmt::print(" [{}] ", default_value);
    }
    std::fflush(stdout);
    
    String result;
    std::getline(std::cin, result);
    
    if (result.empty() && !default_value.empty()) {
        return default_value;
    }
    
    return result;
}

int TerminalUI::select_option(const std::vector<String>& options, StringView prompt) {
    if (!initialized_ || options.empty()) return -1;
    
    fmt::print("{}\n", prompt);
    for (size_t i = 0; i < options.size(); ++i) {
        fmt::print("  [{}] {}\n", static_cast<int>(i+1), options[i]);
    }
    fmt::print("Select option: ");
    std::fflush(stdout);
    
    int choice;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return -1;
    }
    
    // Clear any remaining input
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    if (choice < 1 || choice > static_cast<int>(options.size())) {
        return -1;
    }
    
    return choice - 1;
}

void TerminalUI::clear_screen() {
    if (!initialized_) return;
    
    // ANSI escape code to clear screen
    fmt::print("\033[2J\033[H");
    std::fflush(stdout);
}

void TerminalUI::move_cursor(u32 row, u32 col) {
    if (!initialized_) return;
    
    // ANSI escape code to move cursor
    fmt::print("\033[{};{}H", row + 1, col + 1); // +1 because terminals are 1-indexed
    std::fflush(stdout);
}

void TerminalUI::save_cursor() {
    if (!initialized_) return;
    
    // ANSI escape code to save cursor position
    fmt::print("\033[s");
    std::fflush(stdout);
}

void TerminalUI::restore_cursor() {
    if (!initialized_) return;
    
    // ANSI escape code to restore cursor position
    fmt::print("\033[u");
    std::fflush(stdout);
}

std::pair<u32, u32> TerminalUI::terminal_size() const {
    // Try to get actual terminal size
    struct winsize w;
    if (isatty(STDOUT_FILENO) && ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        return {static_cast<u32>(w.ws_col), static_cast<u32>(w.ws_row)};
    }
    
    // Fallback to stored values
    return {term_width_, term_height_};
}

void TerminalUI::set_verbose(bool v) {
    verbose_ = v;
}

void TerminalUI::set_color(bool c) {
    color_ = c;
    if (!c && initialized_) {
        // Disable colors if requested
        fmt::print("\033[0m"); // Reset all attributes
        std::fflush(stdout);
    }
}

void TerminalUI::detect_terminal_size() {
    struct winsize w;
    if (isatty(STDOUT_FILENO) && ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        term_width_ = static_cast<u32>(w.ws_col);
        term_height_ = static_cast<u32>(w.ws_row);
    } else {
        // Default fallback values
        term_width_ = 80;
        term_height_ = 24;
    }
}

void TerminalUI::print_colored(fmt::color color, StringView text) {
    if (!initialized_ || !color_) {
        fmt::print("{}", text);
        return;
    }
    
    fmt::print(fmt::fg(color), "{}", text);
    std::fflush(stdout);
}

void TerminalUI::print_line(char ch, fmt::color color) {
    if (!initialized_) return;
    
    auto [width, height] = terminal_size();
    if (!color_) {
        fmt::print("{}\n", String(width, ch));
        return;
    }
    
    fmt::print(fmt::fg(color), "{}\n", String(width, ch));
    std::fflush(stdout);
}

void TerminalUI::print_box(StringView title, const std::vector<String>& lines) {
    if (!initialized_) return;
    
    auto [width, height] = terminal_size();
    size_t title_len = title.size();
    size_t content_width = width - 4; // Account for borders and padding
    
    // Print top border
    fmt::print("╔");
    for (size_t i = 0; i < width - 2; ++i) {
        fmt::print("═");
    }
    fmt::print("╗\n");
    
    // Print title if provided
    if (!title.empty()) {
        fmt::print("║ ");
        fmt::print("{:^{}}", title, width - 4);
        fmt::print(" ║\n");
        
        // Print separator line under title
        fmt::print("║");
        for (size_t i = 0; i < width - 2; ++i) {
            fmt::print("─");
        }
        fmt::print("║\n");
    }
    
    // Print content lines
    for (const auto& line : lines) {
        fmt::print("║ ");
        if (line.size() > content_width) {
            // Truncate if too long
            fmt::print("{:.{}}", line, content_width);
        } else {
            fmt::print("{:<{}}", line, content_width);
        }
        fmt::print(" ║\n");
    }
    
    // Print bottom border
    fmt::print("╚");
    for (size_t i = 0; i < width - 2; ++i) {
        fmt::print("═");
    }
    fmt::print("╝\n");
    std::fflush(stdout);
}

String TerminalUI::format_bytes(u64 bytes) const {
    if (!initialized_) return "0 B";
    
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double val = static_cast<double>(bytes);
    
    while (val >= 1024.0 && i < 4) {
        val /= 1024.0;
        i++;
    }
    
    if (config_.use_integer_bytes && i > 0) {
        return fmt::format("{} {}", static_cast<u64>(std::round(val)), units[i]);
    } else {
        return fmt::format("{:.2f} {}", val, units[i]);
    }
}

String TerminalUI::format_number(u64 num) const {
    if (!initialized_) return "0";
    
    if (!config_.use_grouping_separators) {
        return fmt::format("{}", num);
    }
    
    // Add thousands separators
    std::string str = std::to_string(num);
    int insert_pos = static_cast<int>(str.length()) - 3;
    
    while (insert_pos > 0) {
        str.insert(insert_pos, ",");
        insert_pos -= 3;
    }
    
    return str;
}

String TerminalUI::format_duration(Duration d) const {
    if (!initialized_) return "0ms";
    
    using namespace std::chrono;
    
    switch (config_.duration_format) {
        case OutputConfig::DurationFormat::Short: {
            auto ms = duration_cast<milliseconds>(d).count();
            return fmt::format("{} ms", ms);
        }
        case OutputConfig::DurationFormat::Long: {
            auto s = duration_cast<seconds>(d).count();
            auto ms = duration_cast<milliseconds>(d).count() % 1000;
            if (ms > 0) {
                return fmt::format("{} s {} ms", s, ms);
            } else {
                return fmt::format("{} s", s);
            }
        }
        case OutputConfig::DurationFormat::ISO:
        default: {
            // Human readable format
            auto days_dur = duration_cast<std::chrono::days>(d);
            d -= days_dur;
            auto hours_dur = duration_cast<hours>(d);
            d -= hours_dur;
            auto minutes_dur = duration_cast<minutes>(d);
            d -= minutes_dur;
            auto seconds_dur = duration_cast<seconds>(d);
            
            std::string result;
            if (days_dur.count() > 0) {
                result += fmt::format("{}d ", days_dur.count());
            }
            if (hours_dur.count() > 0 || !result.empty()) {
                result += fmt::format("{}h ", hours_dur.count());
            }
            if (minutes_dur.count() > 0 || !result.empty()) {
                result += fmt::format("{}m ", minutes_dur.count());
            }
            result += fmt::format("{}s", seconds_dur.count());
            
            return result;
        }
    }
}
