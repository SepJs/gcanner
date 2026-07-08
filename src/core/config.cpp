#include <game_req_analyzer/core/config.hpp>
#include <game_req_analyzer/utils/json_utils.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/platform_utils.hpp>

using namespace game_req;

Result<AppConfig> AppConfig::load(const Path& path) {
    if (!FileUtils::exists(path)) {
        return make_unexpected(MAKE_ERROR(ErrorCode::FileNotFound, 
            std::format("Config file not found: {}", path.string())));
    }
    
    auto json_result = JsonUtils::parse_file(path);
    if (!json_result) return make_unexpected(json_result.error());
    
    AppConfig config;
    try {
        const auto& j = *json_result;
        
        if (j.contains("scan")) {
            const auto& s = j["scan"];
            config.scan.game_path = s.value("game_path", "");
            config.scan.recursive = s.value("recursive", true);
            config.scan.follow_symlinks = s.value("follow_symlinks", false);
            config.scan.max_depth = s.value("max_depth", 50);
            config.scan.max_file_size = s.value("max_file_size", 100 * GiB);
            config.scan.thread_count = s.value("thread_count", std::thread::hardware_concurrency());
            config.scan.use_mmap = s.value("use_mmap", true);
            config.scan.verify_checksums = s.value("verify_checksums", false);
            
            if (s.contains("excluded_dirs")) {
                for (const auto& d : s["excluded_dirs"]) {
                    config.scan.excluded_dirs.insert(d.get<String>());
                }
            }
            if (s.contains("excluded_exts")) {
                for (const auto& e : s["excluded_exts"]) {
                    config.scan.excluded_exts.insert(e.get<String>());
                }
            }
        }
        
        if (j.contains("analyzer")) {
            const auto& a = j["analyzer"];
            config.analyzer.analyze_textures = a.value("analyze_textures", true);
            config.analyzer.analyze_models = a.value("analyze_models", true);
            config.analyzer.analyze_audio = a.value("analyze_audio", true);
            config.analyzer.analyze_scripts = a.value("analyze_scripts", true);
            config.analyzer.analyze_executables = a.value("analyze_executables", true);
            config.analyzer.analyze_shaders = a.value("analyze_shaders", true);
            config.analyzer.analyze_particles = a.value("analyze_particles", true);
            config.analyzer.analyze_physics = a.value("analyze_physics", true);
            config.analyzer.analyze_ai = a.value("analyze_ai", true);
            config.analyzer.analyze_network = a.value("analyze_network", true);
            config.analyzer.texture_sample_count = a.value("texture_sample_count", 1000);
            config.analyzer.model_sample_count = a.value("model_sample_count", 500);
            config.analyzer.compression_assumption = a.value("compression_assumption", 0.3f);
            config.analyzer.lod_factor = a.value("lod_factor", 0.7f);
        }
        
        if (j.contains("hardware")) {
            const auto& h = j["hardware"];
            config.hardware.auto_update = h.value("auto_update", true);
            config.hardware.update_interval_days = h.value("update_interval_days", 7);
            config.hardware.cache_dir = h.value("cache_dir", "");
            config.hardware.preferred_source = h.value("preferred_source", "passmark");
            config.hardware.include_mobile = h.value("include_mobile", false);
            config.hardware.include_workstation = h.value("include_workstation", true);
            config.hardware.include_server = h.value("include_server", false);
            config.hardware.min_benchmark_score = h.value("min_benchmark_score", 1000);
        }
        
        if (j.contains("output")) {
            const auto& o = j["output"];
            String fmt = o.value("format", "terminal");
            if (fmt == "json") config.output.format = OutputConfig::Format::Json;
            else if (fmt == "csv") config.output.format = OutputConfig::Format::Csv;
            else if (fmt == "html") config.output.format = OutputConfig::Format::Html;
            else if (fmt == "markdown" || fmt == "md") config.output.format = OutputConfig::Format::Markdown;
            else config.output.format = OutputConfig::Format::Terminal;
            
            config.output.output_file = o.value("output_file", "");
            config.output.verbose = o.value("verbose", false);
            config.output.show_progress = o.value("show_progress", true);
            config.output.color_output = o.value("color_output", true);
            config.output.max_display_items = o.value("max_display_items", 20);
            config.output.show_recommendations = o.value("show_recommendations", true);
            config.output.show_alternatives = o.value("show_alternatives", true);
            config.output.show_bottlenecks = o.value("show_bottlenecks", true);
        }
        
        config.config_file = path;
        return config;
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::ParseError, 
            std::format("Failed to parse config: {}", e.what())));
    }
}

Result<void> AppConfig::save(const Path& path) const {
    Json j;
    
    j["scan"]["game_path"] = scan.game_path.string();
    j["scan"]["recursive"] = scan.recursive;
    j["scan"]["follow_symlinks"] = scan.follow_symlinks;
    j["scan"]["max_depth"] = scan.max_depth;
    j["scan"]["max_file_size"] = scan.max_file_size;
    j["scan"]["thread_count"] = scan.thread_count;
    j["scan"]["use_mmap"] = scan.use_mmap;
    j["scan"]["verify_checksums"] = scan.verify_checksums;
    j["scan"]["excluded_dirs"] = scan.excluded_dirs;
    j["scan"]["excluded_exts"] = scan.excluded_exts;
    
    j["analyzer"]["analyze_textures"] = analyzer.analyze_textures;
    j["analyzer"]["analyze_models"] = analyzer.analyze_models;
    j["analyzer"]["analyze_audio"] = analyzer.analyze_audio;
    j["analyzer"]["analyze_scripts"] = analyzer.analyze_scripts;
    j["analyzer"]["analyze_executables"] = analyzer.analyze_executables;
    j["analyzer"]["analyze_shaders"] = analyzer.analyze_shaders;
    j["analyzer"]["analyze_particles"] = analyzer.analyze_particles;
    j["analyzer"]["analyze_physics"] = analyzer.analyze_physics;
    j["analyzer"]["analyze_ai"] = analyzer.analyze_ai;
    j["analyzer"]["analyze_network"] = analyzer.analyze_network;
    j["analyzer"]["texture_sample_count"] = analyzer.texture_sample_count;
    j["analyzer"]["model_sample_count"] = analyzer.model_sample_count;
    j["analyzer"]["compression_assumption"] = analyzer.compression_assumption;
    j["analyzer"]["lod_factor"] = analyzer.lod_factor;
    
    j["hardware"]["auto_update"] = hardware.auto_update;
    j["hardware"]["update_interval_days"] = hardware.update_interval_days;
    j["hardware"]["cache_dir"] = hardware.cache_dir.string();
    j["hardware"]["preferred_source"] = hardware.preferred_source;
    j["hardware"]["include_mobile"] = hardware.include_mobile;
    j["hardware"]["include_workstation"] = hardware.include_workstation;
    j["hardware"]["include_server"] = hardware.include_server;
    j["hardware"]["min_benchmark_score"] = hardware.min_benchmark_score;
    
    String fmt_str;
    switch (output.format) {
        case OutputConfig::Format::Json: fmt_str = "json"; break;
        case OutputConfig::Format::Csv: fmt_str = "csv"; break;
        case OutputConfig::Format::Html: fmt_str = "html"; break;
        case OutputConfig::Format::Markdown: fmt_str = "markdown"; break;
        default: fmt_str = "terminal"; break;
    }
    j["output"]["format"] = fmt_str;
    j["output"]["output_file"] = output.output_file.string();
    j["output"]["verbose"] = output.verbose;
    j["output"]["show_progress"] = output.show_progress;
    j["output"]["color_output"] = output.color_output;
    j["output"]["max_display_items"] = output.max_display_items;
    j["output"]["show_recommendations"] = output.show_recommendations;
    j["output"]["show_alternatives"] = output.show_alternatives;
    j["output"]["show_bottlenecks"] = output.show_bottlenecks;
    
    auto write_result = JsonUtils::write_file(path, j, 4);
    if (!write_result) return make_unexpected(write_result.error());
    
    return success();
}

AppConfig AppConfig::defaults() {
    AppConfig config;
    
    auto cache_dir = PlatformUtils::cache_dir();
    if (cache_dir) {
        config.hardware.cache_dir = *cache_dir / "game-req-analyzer";
    }
    
    return config;
}