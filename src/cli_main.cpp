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
    return fmt::format("{:.2f} {}", val, units[i]);
}

std::string format_number(uint64_t n) {
    std::string s = std::to_string(n);
    int len = s.length();
    for (int i = len - 3; i > 0; i -= 3) {
        s.insert(i, ",");
    }
    return s;
}

int main(int argc, char* argv[]) {
    try {
        CliOptions opts = parse_args(argc, argv);
        
        if (opts.path.empty()) {
            std::cerr << "Error: No path specified\n";
            print_help(argv[0]);
            return 1;
        }
        
        if (!fs::exists(opts.path)) {
            std::cerr << "Error: Path does not exist: " << opts.path << "\n";
            return 1;
        }
        
        // Initialize logger
        Logger::instance().set_level(opts.verbose ? LogLevel::Debug : LogLevel::Info);
        
        // Load config if specified
        AppConfig config;
        if (!opts.config_file.empty()) {
            auto result = config.load(opts.config_file);
            if (!result) {
                std::cerr << "Warning: Failed to load config: " << result.error().message << "\n";
            }
        }
        
        // Configure analyzers
        AnalyzerConfig analyzer_config;
        
        // Create analyzers
        TextureAnalyzer texture_analyzer(analyzer_config);
        ModelAnalyzer model_analyzer(analyzer_config);
        AudioAnalyzer audio_analyzer(analyzer_config);
        ScriptAnalyzer script_analyzer(analyzer_config);
        ExecutableAnalyzer executable_analyzer(analyzer_config);
        ShaderAnalyzer shader_analyzer(analyzer_config);
        ParticleAnalyzer particle_analyzer(analyzer_config);
        PhysicsAnalyzer physics_analyzer(analyzer_config);
        AIAnalyzer ai_analyzer(analyzer_config);
        NetworkAnalyzer network_analyzer(analyzer_config);
        
        AssetAggregator aggregator(analyzer_config);
        
        // Setup scanner
        FileScanner scanner;
        scanner.set_recursive(opts.recursive);
        scanner.set_max_depth(opts.max_depth);
        
        if (opts.thread_count > 0) {
            scanner.set_thread_count(opts.thread_count);
        }
        
        ProgressDisplay progress(!opts.no_progress, opts.no_color);
        
        // Scan
        LOG_INFO("Starting scan of {}", opts.path.string());
        auto scan_result = scanner.scan(opts.path);
        if (!scan_result) {
            std::cerr << "Scan failed: " << scan_result.error().message << "\n";
            return 1;
        }
        
        const auto& files = scan_result.value();
        progress.set_total(files.size());
        LOG_INFO("Found {} files", files.size());
        
        // Analyze each file
        for (size_t i = 0; i < files.size(); ++i) {
            progress.update(i + 1, files[i].path.filename().string());
            
            const auto& file = files[i];
            
            // Texture
            if (file.type == FileType::Texture) {
                auto result = texture_analyzer.analyze_single(file);
                if (result) aggregator.add_texture(result.value());
            }
            // Model
            else if (file.type == FileType::Model) {
                auto result = model_analyzer.analyze_single(file);
                if (result) aggregator.add_model(result.value());
            }
            // Audio
            else if (file.type == FileType::Audio) {
                auto result = audio_analyzer.analyze_single(file);
                if (result) aggregator.add_audio(result.value());
            }
            // Script
            else if (file.type == FileType::Script) {
                auto result = script_analyzer.analyze_single(file);
                if (result) aggregator.add_script(result.value());
            }
            // Executable
            else if (file.type == FileType::Executable) {
                auto result = executable_analyzer.analyze_single(file);
                if (result) aggregator.add_executable(result.value());
            }
            // Shader
            else if (file.type == FileType::Shader) {
                auto result = shader_analyzer.analyze_single(file);
                if (result) aggregator.add_shader(result.value());
            }
            // Particle
            else if (file.type == FileType::Particle) {
                auto result = particle_analyzer.analyze_single(file);
                if (result) aggregator.add_particle(result.value());
            }
            // Physics
            else if (file.type == FileType::Physics) {
                auto result = physics_analyzer.analyze_single(file);
                if (result) aggregator.add_physics(result.value());
            }
            // AI
            else if (file.type == FileType::AI) {
                auto result = ai_analyzer.analyze_single(file);
                if (result) aggregator.add_ai(result.value());
            }
            // Network
            else if (file.type == FileType::Network) {
                auto result = network_analyzer.analyze_single(file);
                if (result) aggregator.add_network(result.value());
            }
        }
        
        progress.finish();
        
        // Calculate requirements
        LOG_INFO("Calculating requirements...");
        RequirementCalculator calc;
        GameProfile profile;
        
        auto aggregated = aggregator.get_aggregated();
        auto requirements = calc.calculate(aggregated, profile);
        
        // Output results
        std::string output;
        
        if (opts.format == "json") {
            // JSON output
            nlohmann::json j;
            j["scan_path"] = opts.path.string();
            j["total_files"] = files.size();
            j["requirements"] = requirements.to_json();
            j["aggregated"] = aggregated.to_json();
            output = j.dump(2);
        } else if (opts.format == "terminal" || opts.format == "text") {
            // Terminal output
            std::ostringstream oss;
            oss << "\n";
            if (!opts.no_color) oss << "\033[1;36m";
            oss << "═══════════════════════════════════════════════════════════════\n";
            oss << "  gcanner - Game System Requirements Analysis Report\n";
            oss << "═══════════════════════════════════════════════════════════════\n";
            if (!opts.no_color) oss << "\033[0m";
            
            oss << "  Game Path: " << opts.path << "\n";
            oss << "  Files Scanned: " << format_number(files.size()) << "\n";
            oss << "  Total Size: " << format_bytes(aggregated.total_size) << "\n\n";
            
            auto print_req = [&](const char* label, const HardwareRequirement& req, const char* color) {
                if (!opts.no_color) oss << color;
                oss << "  " << label << ":\n";
                oss << "    CPU: " << req.cpu_vendor << " " << req.cpu_arch 
                    << " @ " << std::fixed << std::setprecision(1) << req.cpu_base_clock << " GHz\n";
                oss << "         " << req.cpu_cores << "C/" << req.cpu_threads << "T\n";
                oss << "    GPU: " << req.gpu_vendor << " " << req.gpu_architecture 
                    << " (" << req.gpu_vram/1024 << " GB " << req.gpu_vram_type << ")\n";
                oss << "    RAM: " << req.ram_capacity/1024 << " GB " << req.ram_type 
                    << " @ " << req.ram_speed << " MT/s\n";
                oss << "    Storage: " << req.storage_type << " " << req.storage_capacity << " GB\n";
                oss << "    API: DX" << req.dx_version << ", Vulkan " << req.vk_version << "\n\n";
                if (!opts.no_color) oss << "\033[0m";
            };
            
            print_req("Minimum", requirements.minimum, "\033[32m");
            print_req("Recommended", requirements.recommended, "\033[34m");
            if (requirements.high.cpu_cores > 0)
                print_req("High", requirements.high, "\033[33m");
            if (requirements.ultra.cpu_cores > 0)
                print_req("Ultra", requirements.ultra, "\033[35m");
            
            // Asset breakdown
            oss << "  Asset Breakdown:\n";
            auto print_asset = [&](const char* name, const auto& stats) {
                if (stats.count > 0) {
                    oss << "    " << name << ": " << format_number(stats.count) 
                        << " files, " << format_bytes(stats.total_size) << "\n";
                }
            };
            print_asset("Textures", aggregated.textures);
            print_asset("Models", aggregated.models);
            print_asset("Audio", aggregated.audio);
            print_asset("Scripts", aggregated.scripts);
            print_asset("Executables", aggregated.executables);
            print_asset("Shaders", aggregated.shaders);
            print_asset("Particles", aggregated.particles);
            print_asset("Physics", aggregated.physics);
            print_asset("AI", aggregated.ai);
            print_asset("Network", aggregated.network);
            
            output = oss.str();
        }
        
        // Write output
        if (!opts.output_file.empty()) {
            std::ofstream out(opts.output_file);
            out << output;
            std::cout << "Report saved to: " << opts.output_file << "\n";
        } else {
            std::cout << output;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
}