#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>

namespace game_req {

struct ScanConfig {
    Path game_path;
    bool recursive = true;
    bool follow_symlinks = false;
    u32 max_depth = 50;
    u64 max_file_size = 100 * GiB;
    u32 thread_count = std::thread::hardware_concurrency();
    bool use_mmap = true;
    bool verify_checksums = false;
    std::unordered_set<String> excluded_dirs = {
        ".git", ".svn", "node_modules", "__pycache__", ".vs", "build", "dist"
    };
    std::unordered_set<String> excluded_exts = {
        ".tmp", ".temp", ".bak", ".backup", ".log", ".pid", ".lock"
    };
};

struct AnalyzerConfig {
    bool analyze_textures = true;
    bool analyze_models = true;
    bool analyze_audio = true;
    bool analyze_scripts = true;
    bool analyze_executables = true;
    bool analyze_shaders = true;
    bool analyze_particles = true;
    bool analyze_physics = true;
    bool analyze_ai = true;
    bool analyze_network = true;
    u32 texture_sample_count = 1000;
    u32 model_sample_count = 500;
    f32 compression_assumption = 0.3f;
    f32 lod_factor = 0.7f;
};

struct HardwareConfig {
    bool auto_update = true;
    u32 update_interval_days = 7;
    Path cache_dir;
    String preferred_source = "passmark";
    bool include_mobile = false;
    bool include_workstation = true;
    bool include_server = false;
    u32 min_benchmark_score = 1000;
};

struct OutputConfig {
    enum class Format { Terminal, Json, Csv, Html, Markdown };
    Format format = Format::Terminal;
    Path output_file;
    bool verbose = false;
    bool show_progress = true;
    bool color_output = true;
    u32 max_display_items = 20;
    bool show_recommendations = true;
    bool show_alternatives = true;
    bool show_bottlenecks = true;
};

struct AppConfig {
    ScanConfig scan;
    AnalyzerConfig analyzer;
    HardwareConfig hardware;
    OutputConfig output;
    Path config_file;
    
    static Result<AppConfig> load(const Path& path);
    Result<void> save(const Path& path) const;
    static AppConfig defaults();
};

} // namespace game_req