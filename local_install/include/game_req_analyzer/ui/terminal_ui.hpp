#pragma once
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/config.hpp>
#include <game_req_analyzer/analyzers/requirement_calculator.hpp>
#include <game_req_analyzer/scanner/file_scanner.hpp>

namespace game_req {

class TerminalUI {
public:
    explicit TerminalUI(const OutputConfig& config);
    
    void initialize();
    void shutdown();
    
    void show_banner();
    void show_help();
    void show_progress(f32 progress, StringView status);
    void show_scan_results(const ScanResult& result);
    void show_analysis_results(const AggregatedAssets& assets);
    void show_requirements(const RequirementResult& req);
    void show_hardware_recommendations(const RequirementResult& req);
    void show_comparison_table(const std::vector<const CPUEntry*>& cpus, 
                               const std::vector<const GPUEntry*>& gpus);
    void show_error(const Error& err);
    void show_warning(StringView msg);
    void show_info(StringView msg);
    void show_success(StringView msg);
    
    [[nodiscard]] bool confirm(StringView prompt, bool default_yes = true);
    [[nodiscard]] String input(StringView prompt, StringView default_value = "");
    [[nodiscard]] int select_option(const std::vector<String>& options, StringView prompt);
    
    void clear_screen();
    void move_cursor(u32 row, u32 col);
    void save_cursor();
    void restore_cursor();
    [[nodiscard]] std::pair<u32, u32> terminal_size() const;
    
    void set_verbose(bool v) { verbose_ = v; }
    void set_color(bool c) { color_ = c; }

private:
    OutputConfig config_;
    bool verbose_ = false;
    bool color_ = true;
    bool initialized_ = false;
    u32 term_width_ = 80;
    u32 term_height_ = 24;
    
    void detect_terminal_size();
    void print_colored(fmt::color color, StringView text);
    void print_line(char ch = '-', fmt::color color = fmt::color::white);
    void print_box(StringView title, const std::vector<String>& lines);
    String format_bytes(u64 bytes) const;
    String format_number(u64 num) const;
    String format_duration(Duration d) const;
};

class ProgressDisplay {
public:
    ProgressDisplay(TerminalUI& ui, StringView title, u64 total);
    
    void start();
    void update(u64 current, StringView status = "");
    void finish(bool success = true);
    void set_total(u64 total);
    
private:
    TerminalUI& ui_;
    String title_;
    u64 total_ = 0;
    u64 current_ = 0;
    TimePoint start_time_;
    bool started_ = false;
    bool finished_ = false;
    
    void draw_bar(f32 progress);
    String format_eta() const;
};

class ResultsFormatter {
public:
    explicit ResultsFormatter(const OutputConfig& config);
    
    String format_scan_result(const ScanResult& result) const;
    String format_assets(const AggregatedAssets& assets) const;
    String format_requirements(const RequirementResult& req) const;
    String format_json(const RequirementResult& req) const;
    String format_csv(const RequirementResult& req) const;
    String format_markdown(const RequirementResult& req) const;
    String format_html(const RequirementResult& req) const;
    
    Result<void> write_to_file(const String& content, const Path& path) const;

private:
    OutputConfig config_;
    
    String format_hardware_req(const HardwareRequirement& req, RequirementLevel level) const;
    String format_cpu_recommendations(const std::vector<const CPUEntry*>& cpus) const;
    String format_gpu_recommendations(const std::vector<const GPUEntry*>& gpus) const;
    String format_ram_recommendations(const std::vector<const RAMEntry*>& ram) const;
    String format_storage_recommendations(const std::vector<const StorageEntry*>& storage) const;
    String format_bytes(u64 bytes) const;
    String format_number(u64 num) const;
    String format_duration(Duration d) const;
};

} // namespace game_req