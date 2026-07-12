#pragma once
#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>

namespace game_req {

struct ScriptInfo {
    String language;
    u64 line_count = 0;
    u64 char_count = 0;
    u64 function_count = 0;
    u64 class_count = 0;
    u64 estimated_complexity = 0;
    u64 disk_size = 0;
    u64 estimated_ram = 0;
    std::vector<String> dependencies;
    std::vector<String> api_calls;
    String engine_hint;
    String framework_hint;
    bool is_compiled = false;
    String bytecode_format;
};

struct ScriptStats {
    u64 total_scripts = 0;
    u64 total_lines = 0;
    u64 total_functions = 0;
    u64 total_classes = 0;
    u64 total_complexity = 0;
    u64 total_ram_estimate = 0;
    std::unordered_map<String, u32> language_counts;
    std::unordered_map<String, u32> engine_counts;
    std::unordered_map<String, u32> framework_counts;
    
    [[nodiscard]] String summary() const;
};

class ScriptAnalyzer {
public:
    explicit ScriptAnalyzer(const AnalyzerConfig& config);
    
    Result<std::vector<ScriptInfo>> analyze(const std::vector<FileInfo>& scripts);
    Result<ScriptInfo> analyze_single(const FileInfo& script);
    [[nodiscard]] ScriptStats stats() const { return stats_; }
    [[nodiscard]] String generate_report() const;

private:
    AnalyzerConfig config_;
    ScriptStats stats_;
    mutable std::mutex stats_mutex_;
    
    Result<ScriptInfo> analyze_lua(const Path& path);
    Result<ScriptInfo> analyze_python(const Path& path);
    Result<ScriptInfo> analyze_csharp(const Path& path);
    Result<ScriptInfo> analyze_js(const Path& path);
    Result<ScriptInfo> analyze_ts(const Path& path);
    Result<ScriptInfo> analyze_shader(const Path& path, FileType type);
    Result<ScriptInfo> analyze_angel_script(const Path& path);
    Result<ScriptInfo> analyze_sqf(const Path& path);
    Result<ScriptInfo> analyze_pawn(const Path& path);
    Result<ScriptInfo> analyze_generic(const Path& path, FileType type);
    
    String read_file_content(const Path& path);
    u64 estimate_complexity(const String& content, StringView lang) const;
    u64 estimate_ram(const ScriptInfo& script) const;
    void detect_api_calls(ScriptInfo& script, StringView content) const;
};

} // namespace game_req