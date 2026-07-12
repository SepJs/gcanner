#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>

namespace game_req {

struct ShaderInfo {
    String language;           // HLSL, GLSL, SPIR-V, MSL, Cg, etc.
    String profile;            // vs_5_0, ps_5_0, cs_5_0, etc.
    String entry_point;
    u64 instruction_count = 0;
    u64 register_count = 0;
    u32 temp_register_count = 0;
    u32 constant_buffer_count = 0;
    u32 texture_sampler_count = 0;
    u32 uav_count = 0;
    u32 srv_count = 0;
    u32 cbuffer_count = 0;
    u32 loop_count = 0;
    u32 branch_count = 0;
    u32 texture_load_count = 0;
    u32 math_instruction_count = 0;
    u32 tex_instruction_count = 0;
    u32 flow_control_count = 0;
    u32 barrier_count = 0;
    u64 estimated_cycles = 0;
    u64 disk_size = 0;
    u64 estimated_vram = 0;
    String engine_hint;
    String stage;              // vertex, pixel, compute, geometry, hull, domain, mesh, amplification
    bool is_compiled = false;
    String bytecode_format;    // DXBC, DXIL, SPIR-V, Metal bytecode
};

struct ShaderStats {
    u64 total_shaders = 0;
    u64 total_instructions = 0;
    u64 total_disk_size = 0;
    u64 total_estimated_vram = 0;
    std::unordered_map<String, u32> language_counts;
    std::unordered_map<String, u32> stage_counts;
    std::unordered_map<String, u32> profile_counts;
    std::unordered_map<String, u32> engine_counts;
    u32 max_instructions_per_shader = 0;
    u32 max_registers_per_shader = 0;
    
    [[nodiscard]] String summary() const;
};

class ShaderAnalyzer {
public:
    explicit ShaderAnalyzer(const AnalyzerConfig& config);
    
    Result<std::vector<ShaderInfo>> analyze(const std::vector<FileInfo>& shaders);
    Result<ShaderInfo> analyze_single(const FileInfo& shader);
    [[nodiscard]] ShaderStats stats() const { return stats_; }
    [[nodiscard]] String generate_report() const;

private:
    AnalyzerConfig config_;
    ShaderStats stats_;
    mutable std::mutex stats_mutex_;
    
    Result<ShaderInfo> analyze_hlsl(const Path& path);
    Result<ShaderInfo> analyze_glsl(const Path& path);
    Result<ShaderInfo> analyze_spirv(const Path& path);
    Result<ShaderInfo> analyze_msl(const Path& path);
    Result<ShaderInfo> analyze_cg(const Path& path);
    Result<ShaderInfo> analyze_dxbc(const Path& path);
    Result<ShaderInfo> analyze_dxil(const Path& path);
    Result<ShaderInfo> analyze_metal_bytecode(const Path& path);
    Result<ShaderInfo> analyze_generic(const Path& path, FileType type);
    
    String read_file_content(const Path& path) const;
    u64 estimate_complexity(const String& content, StringView lang) const;
    u64 estimate_vram_usage(const ShaderInfo& shader) const;
    void detect_engine_hints(ShaderInfo& shader, StringView content) const;
    String detect_stage_from_entry(const String& entry_point) const;
};

} // namespace game_req