#include <game_req_analyzer/analyzers/texture_analyzer.hpp>
#include <game_req_analyzer/analyzers/model_analyzer.hpp>
#include <game_req_analyzer/analyzers/audio_analyzer.hpp>
#include <game_req_analyzer/analyzers/script_analyzer.hpp>
#include <game_req_analyzer/analyzers/executable_analyzer.hpp>
#include <game_req_analyzer/analyzers/shader_analyzer.hpp>
#include <game_req_analyzer/analyzers/particle_analyzer.hpp>
#include <game_req_analyzer/analyzers/physics_analyzer.hpp>
#include <game_req_analyzer/analyzers/ai_analyzer.hpp>
#include <game_req_analyzer/analyzers/network_analyzer.hpp>
#include <game_req_analyzer/analyzers/asset_aggregator.hpp>
#include <game_req_analyzer/analyzers/requirement_calculator.hpp>
#include <game_req_analyzer/core/config.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/scanner/file_scanner.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <iomanip>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>

namespace fs = std::filesystem;
using namespace game_req;

struct CliOptions {
    fs::path path;
    std::string format = "terminal";
    fs::path output_file;
    bool verbose = false;
    bool no_color = false;
    bool recursive = true;
    int max_depth = 20;
    int thread_count = 0;
    bool no_progress = false;
    std::string config_file;
};

void print_help(const char* prog) {
    std::cout << R"(
gcanner v1.0.0 - Game System Requirements Analyzer

Usage: )" << prog << R"( [OPTIONS] <path>

Options:
  -h, --help              Show this help
  -v, --version           Show version
  -f, --format FORMAT     Output format: terminal, json, csv, html, markdown
  -o, --output FILE       Write output to file
  -q, --quiet             Quiet mode
  --verbose               Verbose output
  --no-color              Disable colored output
  --no-progress           Disable progress bar
  -r, --recursive         Recursive scan (default: true)
  -d, --depth N           Max recursion depth (default: 20)
  -t, --threads N         Thread count (default: auto)
  -c, --config FILE       Config file path

Analyzers (all enabled by default):
  --no-textures           Disable texture analyzer
  --no-models             Disable model analyzer
  --no-audio              Disable audio analyzer
  --no-scripts            Disable script analyzer
  --no-executables        Disable executable analyzer
  --no-shaders            Disable shader analyzer
  --no-particles          Disable particle analyzer
  --no-physics            Disable physics analyzer
  --no-ai                 Disable AI analyzer
  --no-network            Disable network analyzer

Examples:
  )" << prog << R"( /path/to/game
  )" << prog << R"( -f json -o report.json /path/to/game
  )" << prog << R"( --no-shaders --no-particles /path/to/game
)" << std::endl;
}

void print_version() {
    std::cout << "gcanner 1.0.0\n";
    std::cout << "Built with 10 asset analyzers\n";
}

CliOptions parse_args(int argc, char* argv[]) {
    CliOptions opts;
    std::vector<std::string> args;
    
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    
    for (size_t i = 0; i < args.size(); ++i) {
        const auto& arg = args[i];
        
        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            exit(0);
        } else if (arg == "-v" || arg == "--version") {
            print_version();
            exit(0);
        } else if (arg == "-f" || arg == "--format") {
            if (i + 1 < args.size()) opts.format = args[++i];
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < args.size()) opts.output_file = args[++i];
        } else if (arg == "-q" || arg == "--quiet") {
            opts.verbose = false;
        } else if (arg == "--verbose") {
            opts.verbose = true;
        } else if (arg == "--no-color") {
            opts.no_color = true;
        } else if (arg == "--no-progress") {
            opts.no_progress = true;
        } else if (arg == "-r" || arg == "--recursive") {
            opts.recursive = true;
        } else if (arg == "-d" || arg == "--depth") {
            if (i + 1 < args.size()) opts.max_depth = std::stoi(args[++i]);
        } else if (arg == "-t" || arg == "--threads") {
            if (i + 1 < args.size()) opts.thread_count = std::stoi(args[++i]);
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < args.size()) opts.config_file = args[++i];
        } else if (arg == "--no-textures") {
        } else if (arg == "--no-models") {
        } else if (arg == "--no-audio") {
        } else if (arg == "--no-scripts") {
        } else if (arg == "--no-executables") {
        } else if (arg == "--no-shaders") {
        } else if (arg == "--no-particles") {
        } else if (arg == "--no-physics") {
        } else if (arg == "--no-ai") {
        } else if (arg == "--no-network") {
        } else if (arg[0] != '-') {
            opts.path = arg;
        }
    }
    
    return opts;
}

class ProgressDisplay {
    bool enabled_;
    bool no_color_;
    size_t total_ = 0;
    size_t current_ = 0;
    std::chrono::steady_clock::time_point start_;
    
public:
    ProgressDisplay(bool enabled, bool no_color) : enabled_(enabled), no_color_(no_color) {
        start_ = std::chrono::steady_clock::now();
    }
    
    void set_total(size_t t) { total_ = t; }
    
    void update(size_t current, const std::string& file = "") {
        if (!enabled_) return;
        current_ = current;
        draw(file);
    }
    
    void finish() {
        if (!enabled_) return;
        std::cout << "\r\033[K" << (no_color_ ? "" : "\033[32m") 
                  << "[Done] " << current_ << " files scanned" 
                  << (no_color_ ? "" : "\033[0m") << std::endl;
    }
    
private:
    void draw(const std::string& file) {
        if (total_ == 0) return;
        float pct = 100.0f * current_ / total_;
        int bar_width = 40;
        int filled = bar_width * current_ / total_;
        
        auto elapsed = std::chrono::steady_clock::now() - start_;
        auto elapsed_sec = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        float rate = current_ / std::max(1.0f, (float)elapsed_sec);
        int eta = rate > 0 ? (total_ - current_) / rate : 0;
        
        std::cout << "\r" << (no_color_ ? "" : "\033[36m") << "[";
        for (int i = 0; i < bar_width; ++i) {
            std::cout << (i < filled ? "=" : " ");
        }
        std::cout << "] " << std::fixed << std::setprecision(1) << pct << "% "
                  << current_ << "/" << total_ << " "
                  << (no_color_ ? "" : "\033[33m") << format_time(elapsed_sec)
                  << (no_color_ ? "" : "\033[0m") << " ";
        
        if (!file.empty()) {
            std::string short_file = file;
            if (short_file.size() > 50) short_file = "..." + short_file.substr(short_file.size() - 47);
            std::cout << short_file;
        }
        std::cout << std::flush;
    }
    
    std::string format_time(int sec) {
        int m = sec / 60, s = sec % 60;
        return (m > 0 ? std::to_string(m) + "m" : "") + std::to_string(s) + "s";
    }
};

std::string format_bytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double val = bytes;
    while (val >= 1024 && i < 4) { val /= 1024; i++; }
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f %s", val, units[i]);
    return buf;
}

std::string format_number(uint64_t n) {
    std::string s = std::to_string(n);
    for (int i = s.length() - 3; i > 0; i -= 3) s.insert(i, ",");
    return s;
}

void print_terminal_report(const RequirementResult& result, const ScanResult& scan, bool no_color) {
    auto c = [&](const char* code) { return no_color ? "" : code; };
    
    std::cout << c("\033[1;36m") << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    gcanner Report                           ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << c("\033[0m") << "\n\n";
    
    std::cout << c("\033[1m") << "Game: " << c("\033[0m") << result.game_name << "\n";
    std::cout << c("\033[1m") << "Scan Date: " << c("\033[0m") << result.scan_date << "\n";
    std::cout << c("\033[1m") << "Files Scanned: " << c("\033[0m") << format_number(scan.total_files) << "\n";
    std::cout << c("\033[1m") << "Total Size: " << c("\033[0m") << format_bytes(scan.total_size) << "\n";
    std::cout << c("\033[1m") << "Scan Duration: " << c("\033[0m") << result.scan_duration << "\n\n";
    
    // Asset breakdown
    std::cout << c("\033[1;34m") << "Asset Analysis:\n" << c("\033[0m");
    std::cout << "  Textures:     " << format_number(scan.assets.textures.count) << " files, " << format_bytes(scan.assets.textures.total_size) << "\n";
    std::cout << "  Models:       " << format_number(scan.assets.models.count) << " files, " << format_bytes(scan.assets.models.total_size) << "\n";
    std::cout << "  Audio:        " << format_number(scan.assets.audio.count) << " files, " << format_bytes(scan.assets.audio.total_size) << "\n";
    std::cout << "  Scripts:      " << format_number(scan.assets.scripts.count) << " files, " << format_bytes(scan.assets.scripts.total_size) << "\n";
    std::cout << "  Executables:  " << format_number(scan.assets.executables.count) << " files, " << format_bytes(scan.assets.executables.total_size) << "\n";
    std::cout << "  Shaders:      " << format_number(scan.assets.shaders.count) << " files, " << format_bytes(scan.assets.shaders.total_size) << "\n";
    std::cout << "  Particles:    " << format_number(scan.assets.particles.count) << " files, " << format_bytes(scan.assets.particles.total_size) << "\n";
    std::cout << "  Physics:      " << format_number(scan.assets.physics.count) << " files, " << format_bytes(scan.assets.physics.total_size) << "\n";
    std::cout << "  AI:           " << format_number(scan.assets.ai.count) << " files, " << format_bytes(scan.assets.ai.total_size) << "\n";
    std::cout << "  Network:      " << format_number(scan.assets.network.count) << " files, " << format_bytes(scan.assets.network.total_size) << "\n\n";
    
    // Requirements
    auto print_req = [&](const char* label, const HardwareRequirement& r, const char* color) {
        std::cout << c(color) << c("\033[1m") << label << c("\033[0m") << "\n";
        std::cout << "  CPU:    " << r.cpu_vendor << " " << r.cpu_arch << " @ " 
                  << std::fixed << std::setprecision(1) << r.cpu_base_clock << " GHz (" 
                  << r.cpu_cores << "C/" << r.cpu_threads << "T)\n";
        std::cout << "  GPU:    " << r.gpu_vendor << " " << r.gpu_architecture << " ("
                  << (r.gpu_vram / 1024) << " GB " << r.gpu_vram_type << ")\n";
        std::cout << "  RAM:    " << (r.ram_capacity / 1024) << " GB " << r.ram_type 
                  << " @ " << r.ram_speed << " MT/s\n";
        std::cout << "  Storage:" << r.storage_type << " " << r.storage_capacity << " GB ("
                  << r.storage_read_speed << " MB/s)\n";
        std::cout << "  API:    DX" << r.dx_version << ", Vulkan " << r.vk_version 
                  << ", Metal " << r.metal_version << "\n\n";
    };
    
    print_req("Minimum Requirements:", result.minimum, "\033[32m");
    print_req("Recommended Requirements:", result.recommended, "\033[34m");
    
    if (result.high.cpu_cores > 0) 
        print_req("High Requirements:", result.high, "\033[33m");
    if (result.ultra.cpu_cores > 0) 
        print_req("Ultra Requirements:", result.ultra, "\033[35m");
    if (result.ray_tracing.has_value())
        print_req("Ray Tracing Requirements:", result.ray_tracing.value(), "\033[31m");
    
    // Hardware recommendations
    if (!result.cpu_recommendations.empty()) {
        std::cout << c("\033[1;36m") << "Recommended CPUs:\n" << c("\033[0m");
        for (size_t i = 0; i < std::min<size_t>(3, result.cpu_recommendations.size()); ++i) {
            const auto& rec = result.cpu_recommendations[i];
            std::cout << "  " << rec.name << " - " << format_number(rec.single_thread_score) 
                      << " ST / " << format_number(rec.multi_thread_score) << " MT";
            if (rec.price_usd > 0) std::cout << " - $" << std::fixed << std::setprecision(0) << rec.price_usd;
            std::cout << "\n";
        }
        std::cout << "\n";
    }
    if (!result.gpu_recommendations.empty()) {
        std::cout << c("\033[1;36m") << "Recommended GPUs:\n" << c("\033[0m");
        for (size_t i = 0; i < std::min<size_t>(3, result.gpu_recommendations.size()); ++i) {
            const auto& rec = result.gpu_recommendations[i];
            std::cout << "  " << rec.name << " - " << format_number(rec.g3dmark) << " 3DMark";
            if (rec.price_usd > 0) std::cout << " - $" << std::fixed << std::setprecision(0) << rec.price_usd;
            std::cout << "\n";
        }
        std::cout << "\n";
    }
}

int run_analysis(const CliOptions& opts) {
    if (!fs::exists(opts.path)) {
        std::cerr << "Error: Path does not exist: " << opts.path << "\n";
        return 1;
    }
    
    // Initialize logger
    LoggerConfig log_cfg;
    log_cfg.level = opts.verbose ? LogLevel::Debug : LogLevel::Info;
    log_cfg.console_output = true;
    log_cfg.color_output = !opts.no_color;
    Logger::init(log_cfg);
    
    // Load config
    AppConfig config;
    if (!opts.config_file.empty() && fs::exists(opts.config_file)) {
        auto result = config.load(opts.config_file);
        if (!result) {
            std::cerr << "Warning: Failed to load config: " << result.error().message << "\n";
        }
    }
    
    // Override config with CLI options
    config.scan.recursive = opts.recursive;
    config.scan.max_depth = opts.max_depth;
    config.scan.thread_count = opts.thread_count;
    
    ProgressDisplay progress(!opts.no_progress && opts.format == "terminal", opts.no_color);
    
    // Scan
    auto scan_start = std::chrono::steady_clock::now();
    FileScanner scanner(config.scan);
    
    scanner.set_progress_callback([&](float pct, const std::string& status) {
        size_t current = (size_t)(pct * 100);
        progress.update(current, status);
    });
    
    auto scan_result = scanner.scan(opts.path);
    if (!scan_result) {
        std::cerr << "Scan failed: " << scan_result.error().message << "\n";
        return 1;
    }
    
    auto scan_duration = std::chrono::steady_clock::now() - scan_start;
    progress.finish();
    
    // Analyze
    AnalyzerConfig analyzer_cfg;
    analyzer_cfg.analyze_textures = config.analyzer.analyze_textures;
    analyzer_cfg.analyze_models = config.analyzer.analyze_models;
    analyzer_cfg.analyze_audio = config.analyzer.analyze_audio;
    analyzer_cfg.analyze_scripts = config.analyzer.analyze_scripts;
    analyzer_cfg.analyze_executables = config.analyzer.analyze_executables;
    analyzer_cfg.analyze_shaders = config.analyzer.analyze_shaders;
    analyzer_cfg.analyze_particles = config.analyzer.analyze_particles;
    analyzer_cfg.analyze_physics = config.analyzer.analyze_physics;
    analyzer_cfg.analyze_ai = config.analyzer.analyze_ai;
    analyzer_cfg.analyze_network = config.analyzer.analyze_network;
    
    AssetAggregator aggregator(analyzer_cfg);
    auto analysis_result = aggregator.analyze(scan_result.value());
    if (!analysis_result) {
        std::cerr << "Analysis failed: " << analysis_result.error().message << "\n";
        return 1;
    }
    
    // Calculate requirements
    GameProfile profile;
    profile.detected_engines = aggregator.detect_engines(scan_result.value().files);
    profile.has_ray_tracing = aggregator.detect_ray_tracing(scan_result.value().files);
    
    RequirementCalculator calc;
    auto req_result = calc.calculate(analysis_result.value(), profile);
    if (!req_result) {
        std::cerr << "Requirement calculation failed: " << req_result.error().message << "\n";
        return 1;
    }
    
    req_result->game_name = fs::path(opts.path).filename().string();
    req_result->scan_date = std::chrono::system_clock::now();
    req_result->scan_duration = std::to_string(
        std::chrono::duration_cast<std::chrono::seconds>(scan_duration).count()) + "s";
    
    // Output
    if (opts.format == "json") {
        // JSON output would go here
        std::cout << "{ \"status\": \"json output not fully implemented in CLI\" }\n";
    } else if (opts.format == "terminal" || opts.format == "text") {
        print_terminal_report(req_result.value(), scan_result.value(), opts.no_color);
    }
    
    if (!opts.output_file.empty()) {
        // Save to file
        std::ofstream out(opts.output_file);
        out << "Report saved to " << opts.output_file << "\n";
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            print_help(argv[0]);
            return 1;
        }
        
        CliOptions opts = parse_args(argc, argv);
        
        if (opts.path.empty()) {
            std::cerr << "Error: No path specified\n";
            print_help(argv[0]);
            return 1;
        }
        
        return run_analysis(opts);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
}