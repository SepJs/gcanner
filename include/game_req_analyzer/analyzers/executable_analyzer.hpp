#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>

namespace game_req {

struct ExecutableInfo {
    String path;
    String architecture;  // x86, x64, arm, etc.
    String format;        // PE, ELF, Mach-O
    String os;            // Windows, Linux, macOS
    String subsystem;     // Console, GUI, etc.
    u64 image_size = 0;
    u32 subsystem_version = 0;
    u32 dll_characteristics = 0;
    u64 entry_point = 0;
    u64 image_base = 0;
    u32 section_alignment = 0;
    u32 file_alignment = 0;
    u16 major_linker_version = 0;
    u16 minor_linker_version = 0;
    u16 major_os_version = 0;
    u16 minor_os_version = 0;
    u16 major_image_version = 0;
    u16 minor_image_version = 0;
    u16 major_subsystem_version = 0;
    u16 minor_subsystem_version = 0;
    u32 win32_version_value = 0;
    u32 size_of_image = 0;
    u32 size_of_headers = 0;
    u32 checksum = 0;
    u16 machine = 0;
    u16 number_of_sections = 0;
    u32 time_date_stamp = 0;
    u32 pointer_to_symbol_table = 0;
    u32 number_of_symbols = 0;
    u16 size_of_optional_header = 0;
    u16 characteristics = 0;
    bool is_dll = false;
    bool is_executable = false;
    bool has_symbols = false;
    bool has_relocations = false;
    bool is_packaged = false;
    String compiler;      // e.g., MSVC, GCC, Clang
    String linker;        // e.g., link.exe, ld
    String libraries;     // Detected libraries (dll imports, etc.)
    String entry_point_symbol;
    std::vector<String> dependencies; // DLL/so/dylib dependencies
    std::vector<String> exports;      // Exported functions
    std::vector<String> imports;      // Imported functions
};

struct ExecutableStats {
    u64 total_executables = 0;
    std::unordered_map<String, u32> architecture_counts;
    std::unordered_map<String, u32> format_counts;
    std::unordered_map<String, u32> os_counts;
    std::unordered_map<String, u32> subsystem_counts;
    u32 max_image_size = 0;
    u32 min_image_size = std::numeric_limits<u32>::max();
    f32 avg_image_size = 0.0f;
    std::unordered_map<String, u32> compiler_counts;
    std::unordered_map<String, u32> linker_counts;

    [[nodiscard]] String summary() const;
};

class ExecutableAnalyzer {
public:
    explicit ExecutableAnalyzer(const AnalyzerConfig& config);

    Result<std::vector<ExecutableInfo>> analyze(const std::vector<FileInfo>& executables);
    Result<ExecutableInfo> analyze_single(const FileInfo& executable);
    [[nodiscard]] ExecutableStats stats() const { return stats_; }
    [[nodiscard]] String generate_report() const;

private:
    AnalyzerConfig config_;
    ExecutableStats stats_;
    std::mutex stats_mutex_;

    Result<ExecutableInfo> analyze_pe(const Path& path);
    Result<ExecutableInfo> analyze_elf(const Path& path);
    Result<ExecutableInfo> analyze_macho(const Path& path);
    Result<ExecutableInfo> analyze_generic(const Path& path, FileType type);
};

} // namespace game_req
