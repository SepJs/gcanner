#include <game_req_analyzer/core/application.hpp>
#include <game_req_analyzer/core/config.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/scanner/file_scanner.hpp>
#include <game_req_analyzer/analyzers/texture_analyzer.hpp>
#include <game_req_analyzer/analyzers/model_analyzer.hpp>
#include <game_req_analyzer/analyzers/audio_analyzer.hpp>
#include <game_req_analyzer/analyzers/script_analyzer.hpp>
#include <game_req_analyzer/analyzers/asset_aggregator.hpp>
#include <game_req_analyzer/analyzers/requirement_calculator.hpp>
#include <game_req_analyzer/hardware/hardware_database.hpp>
#include <game_req_analyzer/hardware/hardware_updater.hpp>
#include <game_req_analyzer/ui/terminal_ui.hpp>
#include <game_req_analyzer/ui/interactive_mode.hpp>
#include <game_req_analyzer/utils/platform_utils.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/json_utils.hpp>

using namespace game_req;

Application::Application() {
    config_ = AppConfig::defaults();
}

Application::~Application() {
    if (initialized_) {
        shutdown().value_or(Result<void>{});
    }
}

Result<void> Application::initialize() {
    if (initialized_) return success();
    
    Logger::Config log_config;
    log_config.min_level = config_.output.verbose ? Logger::Level::Debug : Logger::Level::Info;
    log_config.color_output = config_.output.color_output;
    log_config.show_timestamp = true;
    log_config.show_level = true;
    log_config.show_location = config_.output.verbose;
    
    auto log_result = Logger::instance().init(log_config);
    if (!log_result) return log_result;
    
    LOG_INFO("Initializing GameReqAnalyzer v{}", "1.0.0");
    LOG_DEBUG("OS: {} {} ({})", PlatformUtils::os_name(), PlatformUtils::os_version(), PlatformUtils::architecture());
    LOG_DEBUG("CPU cores: {}, threads: {}", PlatformUtils::cpu_core_count(), PlatformUtils::cpu_thread_count());
    LOG_DEBUG("Total RAM: {} MB", PlatformUtils::total_ram() / MiB);
    
    initialized_ = true;
    return success();
}

Result<void> Application::shutdown() {
    if (!initialized_) return success();
    
    LOG_INFO("Shutting down...");
    Logger::instance().shutdown();
    initialized_ = false;
    return success();
}

Result<Application::ParsedArgs> Application::parse_args(int argc, char* argv[]) {
    ParsedArgs args;
    
    for (int i = 1; i < argc; ++i) {
        StringView arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            args.mode = Mode::Help;
        } else if (arg == "-v" || arg == "--version") {
            args.mode = Mode::Version;
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                args.config_file = argv[++i];
            }
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                args.output_file = argv[++i];
            }
        } else if (arg == "-f" || arg == "--format") {
            if (i + 1 < argc) {
                StringView fmt = argv[++i];
                if (fmt == "json") args.output_format = OutputConfig::Format::Json;
                else if (fmt == "csv") args.output_format = OutputConfig::Format::Csv;
                else if (fmt == "html") args.output_format = OutputConfig::Format::Html;
                else if (fmt == "md" || fmt == "markdown") args.output_format = OutputConfig::Format::Markdown;
                else args.output_format = OutputConfig::Format::Terminal;
            }
        } else if (arg == "--verbose") {
            args.verbose = true;
        } else if (arg == "--no-color") {
            args.no_color = true;
        } else if (arg == "--force-update") {
            args.force_update = true;
        } else if (arg == "--dry-run") {
            args.dry_run = true;
        } else if (arg == "-j" || arg == "--threads") {
            if (i + 1 < argc) {
                args.threads = static_cast<u32>(std::stoi(argv[++i]));
            }
        } else if (arg == "--scan") {
            args.mode = Mode::Scan;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                args.game_path = argv[++i];
            }
        } else if (arg == "--analyze") {
            args.mode = Mode::Analyze;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                args.game_path = argv[++i];
            }
        } else if (arg == "--interactive") {
            args.mode = Mode::Interactive;
        } else if (arg == "--update-db") {
            args.mode = Mode::UpdateDatabase;
        } else if (arg == "--browse") {
            args.mode = Mode::BrowseHardware;
        } else if (arg == "--compare") {
            args.mode = Mode::Compare;
        } else if (arg[0] != '-') {
            if (args.game_path.empty()) {
                args.game_path = arg;
            }
        }
    }
    
    return args;
}

void Application::print_help() {
    fmt::print(R"(
GameReqAnalyzer - Premium Game System Requirements Analyzer
Version: 1.0.0

Usage: game-req-analyzer [OPTIONS] [PATH]

Modes:
  --scan PATH          Scan game directory and analyze requirements
  --analyze PATH       Analyze previously scanned data
  --interactive        Run interactive mode (default)
  --update-db          Update hardware database from online sources
  --browse             Browse hardware database
  --compare            Compare hardware components

Options:
  -c, --config FILE    Configuration file path
  -o, --output FILE    Output file path
  -f, --format FMT     Output format: terminal, json, csv, html, md
  -j, --threads N      Number of worker threads (default: auto)
  --verbose            Enable verbose logging
  --no-color           Disable colored output
  --force-update       Force hardware database update
  --dry-run            Perform dry run without saving results
  -h, --help           Show this help message
  -v, --version        Show version information

Examples:
  game-req-analyzer --scan "/path/to/game"
  game-req-analyzer --scan "/path/to/game" -o report.json -f json
  game-req-analyzer --interactive
  game-req-analyzer --update-db --force-update
  game-req-analyzer --browse

)");
}

void Application::print_version() {
    fmt::print("GameReqAnalyzer v1.0.0\n");
    fmt::print("Built: {} {}\n", __DATE__, __TIME__);
    fmt::print("C++ Standard: C++20\n");
    fmt::print("Platform: ");
    #ifdef GAME_REQ_WINDOWS
    fmt::print("Windows");
    #elif defined(GAME_REQ_MACOS)
    fmt::print("macOS");
    #elif defined(GAME_REQ_LINUX)
    fmt::print("Linux");
    #else
    fmt::print("Unknown");
    #endif
    fmt::print("\n");
}

int Application::run(int argc, char* argv[]) {
    auto args = parse_args(argc, argv);
    if (!args) {
        LOG_ERROR("Failed to parse arguments: {}", args.error().message);
        return 1;
    }
    
    if (args->mode == Mode::Help) {
        print_help();
        return 0;
    }
    
    if (args->mode == Mode::Version) {
        print_version();
        return 0;
    }
    
    auto init_result = initialize();
    if (!init_result) {
        fmt::print(stderr, "Initialization failed: {}\n", init_result.error().message);
        return 1;
    }
    
    if (args->verbose) {
        config_.output.verbose = true;
        Logger::instance().set_level(Logger::Level::Debug);
    }
    if (args->no_color) {
        config_.output.color_output = false;
    }
    if (args->threads > 0) {
        config_.scan.thread_count = args->threads;
    }
    if (!args->config_file.empty()) {
        auto load_result = load_config(args->config_file);
        if (!load_result) {
            LOG_WARN("Failed to load config: {}", load_result.error().message);
        }
    }
    
    Result<void> result;
    switch (args->mode) {
        case Mode::Scan:
            result = run_scan(*args);
            break;
        case Mode::Analyze:
            result = run_analyze(*args);
            break;
        case Mode::Interactive:
            result = run_interactive(*args);
            break;
        case Mode::UpdateDatabase:
            result = run_update_database(*args);
            break;
        case Mode::BrowseHardware:
            result = run_browse_hardware(*args);
            break;
        case Mode::Compare:
            result = run_compare(*args);
            break;
        default:
            result = run_interactive(*args);
            break;
    }
    
    if (!result) {
        LOG_ERROR("Operation failed: {}", result.error().message);
        return 1;
    }
    
    return 0;
}

Result<void> Application::load_config(const Path& path) {
    auto result = AppConfig::load(path);
    if (!result) return make_unexpected(result.error());
    config_ = *result;
    LOG_INFO("Loaded configuration from {}", path.string());
    return success();
}

Result<void> Application::save_config(const Path& path) const {
    auto result = config_.save(path);
    if (!result) return make_unexpected(result.error());
    LOG_INFO("Saved configuration to {}", path.string());
    return success();
}

Result<void> Application::run_scan(const ParsedArgs& args) {
    if (args.game_path.empty()) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Game path required for scan mode"));
    }
    
    LOG_INFO("Starting scan of: {}", args.game_path.string());
    
    TerminalUI ui(config_.output);
    ui.initialize();
    ui.show_banner();
    
    FileScanner scanner(config_.scan);
    auto scan_result = scanner.scan_async(args.game_path, [&](f32 progress) {
        ui.show_progress(progress, "Scanning files...");
    });
    
    if (!scan_result) {
        ui.show_error(scan_result.error());
        return make_unexpected(scan_result.error());
    }
    
    ui.show_scan_results(*scan_result);
    
    LOG_INFO("Scan complete. Found {} files ({} MB)", 
        scan_result->file_count(), scan_result->total_size() / MiB);
    
    TextureAnalyzer tex_analyzer(config_.analyzer);
    ModelAnalyzer mdl_analyzer(config_.analyzer);
    AudioAnalyzer aud_analyzer(config_.analyzer);
    ScriptAnalyzer scr_analyzer(config_.analyzer);
    ExecutableAnalyzer exe_analyzer(config_.analyzer);
    
    auto textures = scan_result->by_category(FileCategory::Texture);
    auto models = scan_result->by_category(FileCategory::Model3D);
    auto audio = scan_result->by_category(FileCategory::Audio);
    auto scripts = scan_result->by_category(FileCategory::Script);
    auto executables = scan_result->by_category(FileCategory::Executable);
    auto libraries = scan_result->by_category(FileCategory::Library);
    executables.insert(executables.end(), libraries.begin(), libraries.end());
    
    ui.show_progress(0.2f, "Analyzing textures...");
    auto tex_result = tex_analyzer.analyze(textures);
    
    ui.show_progress(0.4f, "Analyzing 3D models...");
    auto mdl_result = mdl_analyzer.analyze(models);
    
    ui.show_progress(0.6f, "Analyzing audio...");
    auto aud_result = aud_analyzer.analyze(audio);
    
    ui.show_progress(0.7f, "Analyzing scripts...");
    auto scr_result = scr_analyzer.analyze(scripts);
    
    ui.show_progress(0.8f, "Analyzing executables...");
    auto exe_result = exe_analyzer.analyze(executables);
    
    if (!tex_result || !mdl_result || !aud_result || !scr_result || !exe_result) {
        return make_unexpected(MAKE_ERROR(ErrorCode::ParseError, "Analysis failed"));
    }
    
    ui.show_progress(0.9f, "Aggregating results...");
    AssetAggregator aggregator(config_.analyzer);
    auto agg_result = aggregator.aggregate(
        tex_analyzer.stats(),
        mdl_analyzer.stats(),
        aud_analyzer.stats(),
        scr_analyzer.stats(),
        exe_analyzer.stats()
    );
    
    if (!agg_result) {
        return make_unexpected(agg_result.error());
    }
    
    ui.show_analysis_results(*agg_result);
    
    HardwareDatabase& hw_db = HardwareDatabase::instance();
    auto hw_init = hw_db.initialize(config_.hardware.cache_dir);
    if (!hw_init) {
        LOG_WARN("Failed to initialize hardware database: {}", hw_init.error().message);
    }
    
    ui.show_progress(0.95f, "Calculating requirements...");
    RequirementCalculator calculator(config_.analyzer, hw_db);
    auto req_result = calculator.calculate(*agg_result);
    
    if (!req_result) {
        return make_unexpected(req_result.error());
    }
    
    ui.show_requirements(*req_result);
    ui.show_hardware_recommendations(*req_result);
    
    if (!args.output_file.empty()) {
        ResultsFormatter formatter(config_.output);
        String output;
        switch (args.output_format) {
            case OutputConfig::Format::Json:
                output = formatter.format_json(*req_result);
                break;
            case OutputConfig::Format::Csv:
                output = formatter.format_csv(*req_result);
                break;
            case OutputConfig::Format::Html:
                output = formatter.format_html(*req_result);
                break;
            case OutputConfig::Format::Markdown:
                output = formatter.format_markdown(*req_result);
                break;
            default:
                output = formatter.format_requirements(*req_result);
                break;
        }
        
        auto write_result = formatter.write_to_file(output, args.output_file);
        if (!write_result) {
            LOG_WARN("Failed to write output file: {}", write_result.error().message);
        } else {
            LOG_INFO("Results written to {}", args.output_file.string());
        }
    }
    
    ui.show_progress(1.0f, "Complete!");
    return success();
}

Result<void> Application::run_analyze(const ParsedArgs& args) {
    LOG_INFO("Analyze mode not fully implemented yet");
    return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "Analyze mode not yet implemented"));
}

Result<void> Application::run_interactive(const ParsedArgs& args) {
    TerminalUI ui(config_.output);
    ui.initialize();
    
    InteractiveMode interactive(ui, config_);
    interactive.run();
    
    return success();
}

Result<void> Application::run_update_database(const ParsedArgs& args) {
    LOG_INFO("Updating hardware database...");
    
    HardwareDatabase& hw_db = HardwareDatabase::instance();
    auto init_result = hw_db.initialize(config_.hardware.cache_dir);
    if (!init_result) {
        return make_unexpected(init_result.error());
    }
    
    HardwareUpdater updater(hw_db);
    if (args.force_update) {
        auto result = updater.force_update();
        if (!result) {
            return make_unexpected(result.error());
        }
        LOG_INFO("Update complete. Added: {} CPUs, {} GPUs", 
            result->cpus_added, result->gpus_added);
    } else {
        auto result = updater.check_and_update();
        if (!result) {
            return make_unexpected(result.error());
        }
        if (result->cpus_added > 0 || result->gpus_added > 0) {
            LOG_INFO("Update complete. Added: {} CPUs, {} GPUs", 
                result->cpus_added, result->gpus_added);
        } else {
            LOG_INFO("Database is up to date");
        }
    }
    
    return success();
}

Result<void> Application::run_browse_hardware(const ParsedArgs& args) {
    HardwareDatabase& hw_db = HardwareDatabase::instance();
    auto init_result = hw_db.initialize(config_.hardware.cache_dir);
    if (!init_result) {
        return make_unexpected(init_result.error());
    }
    
    TerminalUI ui(config_.output);
    ui.initialize();
    ui.show_banner();
    
    LOG_INFO("Hardware database: {} CPUs, {} GPUs, {} RAM, {} Storage", 
        hw_db.cpu_count(), hw_db.gpu_count(), hw_db.ram_count(), hw_db.storage_count());
    
    ui.show_info(std::format("Database contains: {} CPUs, {} GPUs, {} RAM modules, {} Storage devices",
        hw_db.cpu_count(), hw_db.gpu_count(), hw_db.ram_count(), hw_db.storage_count()));
    
    return success();
}

Result<void> Application::run_compare(const ParsedArgs& args) {
    LOG_INFO("Compare mode not fully implemented yet");
    return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "Compare mode not yet implemented"));
}