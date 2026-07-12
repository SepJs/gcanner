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
#include <vector>
#include <string>
#include <algorithm>

namespace fs = std::filesystem;
using namespace game_req;

struct Options {
    fs::path path;
    std::string format = "terminal";
    fs::path output_file;
    bool verbose = false;
    bool no_color = false;
    bool no_progress = false;
};

void print_help(const char* prog) {
    std::cout << R"(
gcanner 1.0.0 - Game System Requirements Analyzer
Usage: )" << prog << R"( [OPTIONS] <game_directory>

Options:
  -f, --format FMT    Output: terminal, json, csv, html, markdown
  -o, --output FILE   Save report to file
  -v, --verbose       Verbose output
  --no-color          Disable colors
  --no-progress       Disable progress bar
  -h, --help          Show help
  -V, --version       Show version

Analyzers (all on by default):
  --no-textures --no-models --no-audio --no-scripts
  --no-executables --no-shaders --no-particles
  --no-physics --no-ai --no-network

Examples:
  )" << prog << R"( /path/to/game
  )" << prog << R"( -f json -o report.json /path/to/game
)" << std::endl;
}

void print_version() {
    std::cout << "gcanner 1.0.0\n10 analyzers: texture, model, audio, script, executable, shader, particle, physics, ai, network\n";
}

Options parse_args(int argc, char* argv[]) {
    Options opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") { print_help(argv[0]); exit(0); }
        else if (arg == "-V" || arg == "--version") { print_version(); exit(0); }
        else if ((arg == "-f" || arg == "--format") && i+1 < argc) opts.format = argv[++i];
        else if ((arg == "-o" || arg == "--output") && i+1 < argc) opts.output_file = argv[++i];
        else if (arg == "-v" || arg == "--verbose") opts.verbose = true;
        else if (arg == "--no-color") opts.no_color = true;
        else if (arg == "--no-progress") opts.no_progress = true;
        else if (arg[0] != '-') opts.path = arg;
        else { std::cerr << "Unknown option: " << arg << "\n"; exit(1); }
    }
    return opts;
}

std::string fmt_bytes(uint64_t b) {
    const char* u[] = {"B","KB","MB","GB","TB"}; int i=0; double v=b;
    while(v>=1024&&i<4){v/=1024;i++;} return fmt::format("{:.2f} {}",v,u[i]);
}
std::string fmt_num(uint64_t n) { std::string s=std::to_string(n); for(int i=s.size()-3;i>0;i-=3)s.insert(i,","); return s; }

int main(int argc, char* argv[]) {
    try {
        Options opt = parse_args(argc, argv);
        if (opt.path.empty()) { print_help(argv[0]); return 1; }
        if (!fs::exists(opt.path)) { std::cerr << "Path not found: " << opt.path << "\n"; return 1; }

        Logger::instance().set_level(opt.verbose ? LogLevel::Debug : LogLevel::Info);
        
        AnalyzerConfig acfg;
        TextureAnalyzer ta(acfg); ModelAnalyzer ma(acfg); AudioAnalyzer aa(acfg);
        ScriptAnalyzer sa(acfg); ExecutableAnalyzer ea(acfg); ShaderAnalyzer sha(acfg);
        ParticleAnalyzer pa(acfg); PhysicsAnalyzer pha(acfg); AIAnalyzer aia(acfg); NetworkAnalyzer na(acfg);
        AssetAggregator agg(acfg);

        FileScanner scanner;
        auto result = scanner.scan(opt.path);
        if (!result) { std::cerr << "Scan failed: " << result.error().message << "\n"; return 1; }
        const auto& files = result.value();

        size_t total = files.size(), done = 0;
        auto start = std::chrono::steady_clock::now();
        auto draw = [&](size_t d, const std::string& f="") {
            if (opt.no_progress) return;
            float p = 100.0f * d / total; int w=40, fill=w*d/total;
            auto et = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-start).count();
            std::cout << "\r[" << std::string(fill,'=') << std::string(w-fill,' ') << "] "
                      << std::fixed<<std::setprecision(1)<<p<<"% "<<d<<"/"<<total<<" "<<f<<std::flush;
        };

        for (const auto& file : files) {
            draw(++done, file.path.filename().string());
            switch(file.type) {
                case FileType::Texture:      if(auto r=ta.analyze_single(file); r) agg.add_texture(r.value()); break;
                case FileType::Model:        if(auto r=ma.analyze_single(file); r) agg.add_model(r.value()); break;
                case FileType::Audio:        if(auto r=aa.analyze_single(file); r) agg.add_audio(r.value()); break;
                case FileType::Script:       if(auto r=sa.analyze_single(file); r) agg.add_script(r.value()); break;
                case FileType::Executable:   if(auto r=ea.analyze_single(file); r) agg.add_executable(r.value()); break;
                case FileType::Shader:       if(auto r=sha.analyze_single(file); r) agg.add_shader(r.value()); break;
                case FileType::Particle:     if(auto r=pa.analyze_single(file); r) agg.add_particle(r.value()); break;
                case FileType::Physics:      if(auto r=pha.analyze_single(file); r) agg.add_physics(r.value()); break;
                case FileType::AI:           if(auto r=aia.analyze_single(file); r) agg.add_ai(r.value()); break;
                case FileType::Network:      if(auto r=na.analyze_single(file); r) agg.add_network(r.value()); break;
                default: break;
            }
        }
        if (!opt.no_progress) std::cout << "\n";

        GameProfile gp;
        RequirementCalculator rc;
        auto req = rc.calculate(agg.get_stats(), gp);
        if (!req) { std::cerr << "Calc failed: " << req.error().message << "\n"; return 1; }
        auto& r = req.value();
        r.game_name = fs::path(opt.path).filename().string();
        r.scan_date = std::chrono::system_clock::now();

        // Output
        std::string out;
        if (opt.format == "json") {
            nlohmann::json j; j["game"]=r.game_name; j["requirements"]=r.to_json();
            j["stats"]=agg.get_stats().to_json(); out=j.dump(2);
        } else {
            std::ostringstream os;
            auto C = [&](const char* c){ return opt.no_color?"":c; };
            os << C("\033[1;36m") << "\n=== gcanner Report ===\n" << C("\033[0m");
            os << "Game: " << r.game_name << "\nFiles: " << fmt_num(done) << "\n\n";

            auto pr = [&](const char* t, const HardwareRequirement& h, const char* col) {
                os << C(col) << C("\033[1m") << t << C("\033[0m") << "\n";
                os << "  CPU: " << h.cpu_vendor << " " << h.cpu_arch << " @ " 
                   << std::fixed<<std::setprecision(1)<<h.cpu_base_clock << "GHz (" << h.cpu_cores << "C/" << h.cpu_threads << "T)\n";
                os << "  GPU: " << h.gpu_vendor << " " << h.gpu_architecture << " (" 
                   << (h.gpu_vram/1024) << "GB " << h.gpu_vram_type << ")\n";
                os << "  RAM: " << (h.ram_capacity/1024) << "GB " << h.ram_type << " @" << h.ram_speed << "MT/s\n";
                os << "  Storage: " << h.storage_type << " " << h.storage_capacity << "GB (" << h.storage_read_speed << "MB/s)\n\n";
            };
            pr("Minimum:", r.minimum, "\033[32m");
            pr("Recommended:", r.recommended, "\033[34m");
            if(r.high.cpu_cores) pr("High:", r.high, "\033[33m");
            if(r.ultra.cpu_cores) pr("Ultra:", r.ultra, "\033[35m");
            if(r.ray_tracing) pr("Ray Tracing:", *r.ray_tracing, "\033[31m");

            // CPU/GPU recommendations
            auto cpu_recs = rc.recommend_cpus(r.recommended);
            if(!cpu_recs.empty()) {
                os << C("\033[1;36m") << "Top CPU Picks:\n" << C("\033[0m");
                for(size_t i=0;i<std::min<size_t>(3,cpu_recs.size());++i) {
                    os << "  " << cpu_recs[i].name << " - " << fmt_num(cpu_recs[i].single_thread_score) 
                       << " ST / " << fmt_num(cpu_recs[i].multi_thread_score) << " MT";
                    if(cpu_recs[i].price_usd>0) os << " - $" << std::fixed<<std::setprecision(0)<<cpu_recs[i].price_usd;
                    os << "\n";
                }
            }
            auto gpu_recs = rc.recommend_gpus(r.recommended);
            if(!gpu_recs.empty()) {
                os << C("\033[1;36m") << "Top GPU Picks:\n" << C("\033[0m");
                for(size_t i=0;i<std::min<size_t>(3,gpu_recs.size());++i) {
                    os << "  " << gpu_recs[i].name << " - " << fmt_num(gpu_recs[i].g3dmark) << " 3DMark";
                    if(gpu_recs[i].price_usd>0) os << " - $" << std::fixed<<std::setprecision(0)<<gpu_recs[i].price_usd;
                    os << "\n";
                }
            }
            out = os.str();
        }

        std::cout << out;
        if (!opt.output_file.empty()) {
            std::ofstream of(opt.output_file); of << out;
            std::cout << "\nSaved to: " << opt.output_file << "\n";
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error << "Error: " << e.what() << "\n"; return 1;
    }
}