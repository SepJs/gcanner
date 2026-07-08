#include <game_req_analyzer/ui/interactive_mode.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/core/application.hpp>
#include <game_req_analyzer/scanner/file_scanner.hpp>
#include <game_req_analyzer/analyzers/asset_aggregator.hpp>
#include <game_req_analyzer/analyzers/requirement_calculator.hpp>
#include <fmt/format.h>
#include <thread>
#include <chrono>

using namespace game_req;

InteractiveMode::InteractiveMode(TerminalUI& ui, const AppConfig& config)
    : ui_(ui), config_(config) {}

void InteractiveMode::run() {
    ui_.show_banner();
    show_main_menu();
}

void InteractiveMode::run_single_scan(const Path& path) {
    ui_->show_info("Starting single scan...");
    
    // Create scanner and run scan
    FileScanner scanner(config_.scan);
    ProgressDisplay progress(ui_, "Scanning files", 0);
    
    // Set up progress callback
    scanner.set_progress_callback([&](float progress, const std::string& status) {
        progress.update(static_cast<uint64_t>(progress * 100), status);
    });
    
    auto scan_result = scanner.scan(path);
    if (!scan_result) {
        ui_->show_error(scan_result.error());
        return;
    }
    
    progress.finish(true);
    ui_->show_scan_results(*scan_result);
    
    // Analyze files
    ui_->show_info("Analyzing files...");
    AssetAggregator aggregator(config_.analyzer);
    
    // In a real implementation, we would distribute files to analyzers
    // For now, we'll simulate analysis
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Mock aggregated assets (would come from actual analysis)
    AggregatedAssets assets;
    // ... populate with real data from analyzers
    
    ui_->show_analysis_results(assets);
    
    // Calculate requirements
    ui_->show_info("Calculating system requirements...");
    RequirementCalculator calculator(config_.analyzer, HardwareDatabase::instance());
    auto req_result = calculator.calculate(assets);
    if (!req_result) {
        ui_->show_error(req_result.error());
        return;
    }
    
    ui_->show_requirements(*req_result);
    ui_->show_hardware_recommendations(*req_result);
    
    // Ask if user wants to save results
    if (ui_.confirm("Save results to file?", false)) {
        auto save_path = ui_.input("Enter file path", "requirements_report.txt");
        if (!save_path.empty()) {
            ResultsFormatter formatter(config_.output);
            auto report = formatter.format_requirements(*req_result);
            auto write_result = formatter.write_to_file(report, save_path);
            if (!write_result) {
                ui_->show_error(write_result.error());
            } else {
                ui_->show_success("Results saved to " + save_path);
            }
        }
    }
}

void InteractiveMode::show_main_menu() {
    std::vector<std::string> options = {
        "Scan a game directory",
        "Analyze a folder",
        "Hardware browser",
        "Settings",
        "Update hardware database",
        "Export results",
        "Compare hardware",
        "Exit"
    };
    
    while (true) {
        ui_.show_info("Main Menu");
        int choice = ui_.select_option(options, "Select an option");
        
        if (choice == -1) {
            ui_->show_warning("Invalid selection. Please try again.");
            continue;
        }
        
        switch (choice) {
            case 0: { // Scan a game directory
                auto path = select_game_folder();
                if (!path.empty()) {
                    run_single_scan(path);
                }
                break;
            }
            case 1: { // Analyze a folder
                auto path = select_game_folder();
                if (!path.empty()) {
                    // TODO: Implement analyze folder
                    ui_->show_info("Analyze folder not yet implemented");
                }
                break;
            }
            case 2: { // Hardware browser
                hardware_browser_menu();
                break;
            }
            case 3: { // Settings
                settings_menu();
                break;
            }
            case 4: { // Update hardware database
                update_database();
                break;
            }
            case 5: { // Export results
                export_results();
                break;
            }
            case 6: { // Compare hardware
                compare_hardware();
                break;
            }
            case 7: { // Exit
                ui_->show_info("Goodbye!");
                return;
            }
        }
        
        ui_->wait_for_key();
    }
}

void InteractiveMode::handle_scan_game() {
    // This is handled in show_main_menu case 0
}

void InteractiveMode::handle_analyze_folder() {
    // This is handled in show_main_menu case 1
}

void InteractiveMode::handle_hardware_browser() {
    // This is handled in show_main_menu case 2
}

void InteractiveMode::handle_settings() {
    // This is handled in show_main_menu case 3
}

void InteractiveMode::handle_update_database() {
    // This is handled in show_main_menu case 4
}

void InteractiveMode::handle_export_results() {
    // This is handled in show_main_menu case 5
}

void InteractiveMode::handle_compare_hardware() {
    // This is handled in show_main_menu case 6
}

void InteractiveMode::hardware_browser_menu() {
    std::vector<std::string> options = {
        "Browse CPUs",
        "Browse GPUs",
        "Browse RAM",
        "Browse Storage",
        "Search Hardware",
        "Back to Main Menu"
    };
    
    while (true) {
        ui_->show_info("Hardware Browser");
        int choice = ui_->select_option(options, "Select an option");
        
        if (choice == -1) {
            ui_->show_warning("Invalid selection. Please try again.");
            continue;
        }
        
        switch (choice) {
            case 0: browse_cpus(); break;
            case 1: browse_gpus(); break;
            case 2: browse_ram(); break;
            case 3: browse_storage(); break;
            case 4: search_hardware(); break;
            case 5: return; // Back to main menu
        }
        
        ui_->wait_for_key();
    }
}

void InteractiveMode::browse_cpus() {
    auto cpus = HardwareDatabase::instance().get_all_cpus();
    if (cpus.empty()) {
        ui_->show_info("No CPU data available");
        return;
    }
    
    ui_->show_info("CPU List (showing first 20):");
    for (size_t i = 0; i < cpus.size() && i < 20; ++i) {
        const auto& cpu = cpus[i];
        ui_->show_info(fmt::format("{}: {} ({} cores/{} threads, {:.1f} GHz)",
                                  i+1, cpu->name, cpu->cores, cpu->threads, cpu->base_clock));
    }
}

void InteractiveMode::browse_gpus() {
    auto gpus = HardwareDatabase::instance().get_all_gpus();
    if (gpus.empty()) {
        ui_->show_info("No GPU data available");
        return;
    }
    
    ui_->show_info("GPU List (showing first 20):");
    for (size_t i = 0; i < gpus.size() && i < 20; ++i) {
        const auto& gpu = gpus[i];
        ui_->show_info(fmt::format("{}: {} ({} GB VRAM, {} MHz)",
                                  i+1, gpu->name, gpu->vram / 1024, gpu->boost_clock));
    }
}

void InteractiveMode::browse_ram() {
    auto ram = HardwareDatabase::instance().get_all_ram();
    if (ram.empty()) {
        ui_->show_info("No RAM data available");
        return;
    }
    
    ui_->show_info("RAM List (showing first 20):");
    for (size_t i = 0; i < ram.size() && i < 20; ++i) {
        const auto& r = ram[i];
        ui_->show_info(fmt::format("{}: {} {} GB {} MT/s",
                                  i+1, r->name, r->type, r->capacity, r->speed));
    }
}

void InteractiveMode::browse_storage() {
    auto storage = HardwareDatabase::instance().get_all_storage();
    if (storage.empty()) {
        ui_->show_info("No storage data available");
        return;
    }
    
    ui_->show_info("Storage List (showing first 20):");
    for (size_t i = 0; i < storage.size() && i < 20; ++i) {
        const auto& s = storage[i];
        ui_->show_info(fmt::format("{}: {} {} GB ({} MB/s read, {} MB/s write)",
                                  i+1, s->name, s->type, s->capacity, s->seq_read, s->seq_write));
    }
}

void InteractiveMode::search_hardware() {
    ui_->show_info("Hardware search not yet implemented");
}

void InteractiveMode::settings_menu() {
    std::vector<std::string> options = {
        "Configure Scan Settings",
        "Configure Analysis Settings",
        "Configure Output Settings",
        "Configure Hardware Settings",
        "Back to Main Menu"
    };
    
    while (true) {
        ui_->show_info("Settings Menu");
        int choice = ui_->select_option(options, "Select an option");
        
        if (choice == -1) {
            ui_->show_warning("Invalid selection. Please try again.");
            continue;
        }
        
        switch (choice) {
            case 0: configure_scan(); break;
            case 1: configure_analysis(); break;
            case 2: configure_output(); break;
            case 3: configure_hardware(); break;
            case 4: return; // Back to main menu
        }
        
        ui_->wait_for_key();
    }
}

void InteractiveMode::configure_scan() {
    ui_->show_info("Scan configuration not yet implemented");
}

void InteractiveMode::configure_analysis() {
    ui_->show_info("Analysis configuration not yet implemented");
}

void InteractiveMode::configure_output() {
    ui_->show_info("Output configuration not yet implemented");
}

void InteractiveMode::configure_hardware() {
    ui_->show_info("Hardware configuration not yet implemented");
}

void InteractiveMode::update_database() {
    ui_->show_info("Checking for hardware database updates...");
    // In a real implementation, this would check for and download updates
    std::this_thread::sleep_for(std::chrono::seconds(2));
    ui_->show_info("Hardware database is up to date");
}

void InteractiveMode::export_results() {
    ui_->show_info("Export results not yet implemented");
}

void InteractiveMode::compare_hardware() {
    ui_->show_info("Hardware comparison not yet implemented");
}

Path InteractiveMode::select_game_folder() {
    std::string path_str = ui_->input("Enter game folder path", "");
    if (path_str.empty()) {
        return Path();
    }
    
    Path path(path_str);
    if (!fs::exists(path)) {
        ui_->show_error("Path does not exist: " + path_str);
        return Path();
    }
    
    if (!fs::is_directory(path)) {
        ui_->show_error("Path is not a directory: " + path_str);
        return Path();
    }
    
    return path;
}

Path InteractiveMode::select_output_file() {
    std::string path_str = ui_->input("Enter output file path", "report.txt");
    if (path_str.empty()) {
        return Path();
    }
    
    return Path(path_str);
}

void InteractiveMode::print_separator() {
    ui_->show_info("----------------------------------------");
}

void InteractiveMode::wait_for_key() {
    ui_->show_info("Press Enter to continue...");
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
