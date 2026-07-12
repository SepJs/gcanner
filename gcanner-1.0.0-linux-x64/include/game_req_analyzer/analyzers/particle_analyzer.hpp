#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>

namespace game_req {

struct ParticleInfo {
    u64 particle_count = 0;
    u64 emitter_count = 0;
    u64 system_count = 0;
    u64 disk_size = 0;
    u64 estimated_vram = 0;
    u64 estimated_ram = 0;
    String format;
    String engine_hint;
    std::vector<String> particle_types;
    u32 max_particles_per_emitter = 0;
    f32 avg_particle_lifetime = 0.0f;
};

struct ParticleStats {
    u64 total_systems = 0;
    u64 total_emitters = 0;
    u64 total_particles = 0;
    u64 total_disk_size = 0;
    u64 total_estimated_vram = 0;
    u64 total_estimated_ram = 0;
    std::unordered_map<String, u32> format_counts;
    std::unordered_map<String, u32> engine_counts;
    u32 max_particles_per_system = 0;
    
    [[nodiscard]] String summary() const;
};

class ParticleAnalyzer {
public:
    explicit ParticleAnalyzer(const AnalyzerConfig& config);
    
    Result<std::vector<ParticleInfo>> analyze(const std::vector<FileInfo>& particles);
    Result<ParticleInfo> analyze_single(const FileInfo& particle);
    [[nodiscard]] ParticleStats stats() const { return stats_; }
    [[nodiscard]] String generate_report() const;

private:
    AnalyzerConfig config_;
    ParticleStats stats_;
    mutable std::mutex stats_mutex_;
    
    Result<ParticleInfo> analyze_pfx(const Path& path);
    Result<ParticleInfo> analyze_ptx(const Path& path);
    Result<ParticleInfo> analyze_par(const Path& path);
    Result<ParticleInfo> analyze_particle(const Path& path);
    Result<ParticleInfo> analyze_emitter(const Path& path);
    Result<ParticleInfo> analyze_generic(const Path& path, FileType type);
    
    String read_file_content(const Path& path) const;
    u64 estimate_complexity(const String& content, StringView lang) const;
    u64 estimate_vram_usage(const ParticleInfo& particle) const;
    u64 estimate_ram_usage(const ParticleInfo& particle) const;
};

} // namespace game_req