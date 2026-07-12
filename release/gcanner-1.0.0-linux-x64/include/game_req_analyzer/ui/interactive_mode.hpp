#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/ui/terminal_ui.hpp>

namespace game_req {

class InteractiveMode {
public:
    InteractiveMode(TerminalUI& ui, const AppConfig& config);
    
    void run();
    void run_single_scan(const Path& path);
    
private:
    TerminalUI& ui_;
    const AppConfig& config_;
    bool running_ = true;
    
    void show_main_menu();
    void handle_scan_game();
    void handle_analyze_folder();
    void handle_hardware_browser();
    void handle_settings();
    void handle_update_database();
    void handle_export_results();
    void handle_compare_hardware();
    
    void hardware_browser_menu();
    void browse_cpus();
    void browse_gpus();
    void browse_ram();
    void browse_storage();
    void search_hardware();
    
    void settings_menu();
    void configure_scan();
    void configure_analysis();
    void configure_output();
    void configure_hardware();
    
    Path select_game_folder();
    Path select_output_file();
    
    void print_separator();
    void wait_for_key();
};

} // namespace game_req