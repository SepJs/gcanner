#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/config.hpp>
#include <game_req_analyzer/core/error.hpp>

namespace game_req {

class Application {
public:
    Application();
    ~Application();
    
    int run(int argc, char* argv[]);
    
private:
    enum class Mode {
        Scan,
        Analyze,
        Interactive,
        UpdateDatabase,
        BrowseHardware,
        Compare,
        Help,
        Version
    };
    
    struct ParsedArgs {
        Mode mode = Mode::Interactive;
        Path game_path;
        Path config_file;
        Path output_file;
        OutputConfig::Format output_format = OutputConfig::Format::Terminal;
        bool verbose = false;
        bool no_color = false;
        bool force_update = false;
        bool dry_run = false;
        u32 threads = 0;
        String filter;
    };
    
    Result<void> initialize();
    Result<void> shutdown();
    Result<ParsedArgs> parse_args(int argc, char* argv[]);
    void print_help();
    void print_version();
    
    Result<void> run_scan(const ParsedArgs& args);
    Result<void> run_analyze(const ParsedArgs& args);
    Result<void> run_interactive(const ParsedArgs& args);
    Result<void> run_update_database(const ParsedArgs& args);
    Result<void> run_browse_hardware(const ParsedArgs& args);
    Result<void> run_compare(const ParsedArgs& args);
    
    Result<void> load_config(const Path& path);
    Result<void> save_config(const Path& path) const;
    
    AppConfig config_;
    bool initialized_ = false;
};

} // namespace game_req