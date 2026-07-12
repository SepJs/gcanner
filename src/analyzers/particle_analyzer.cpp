#include <game_req_analyzer/analyzers/particle_analyzer.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <algorithm>
#include <fstream>
#include <regex>

namespace game_req {

ParticleAnalyzer::ParticleAnalyzer(const AnalyzerConfig& config) : config_(config) {}

Result<std::vector<ParticleInfo>> ParticleAnalyzer::analyze(const std::vector<FileInfo>& particles) {
    std::vector<ParticleInfo> results;
    results.reserve(particles.size());
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_systems = 0;
        stats_.total_emitters = 0;
        stats_.total_particles = 0;
        stats_.total_disk_size = 0;
        stats_.total_estimated_vram = 0;
        stats_.total_estimated_ram = 0;
        stats_.format_counts.clear();
        stats_.engine_counts.clear();
        stats_.max_particles_per_system = 0;
    }
    
    for (const auto& particle : particles) {
        auto result = analyze_single(particle);
        if (result) {
            results.push_back(*result);
            
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_systems++;
            stats_.total_emitters += result->emitter_count;
            stats_.total_particles += result->particle_count;
            stats_.total_disk_size += result->disk_size;
            stats_.total_estimated_vram += result->estimated_vram;
            stats_.total_estimated_ram += result->estimated_ram;
            stats_.format_counts[result->format]++;
            if (!result->engine_hint.empty()) stats_.engine_counts[result->engine_hint]++;
            if (result->particle_count > stats_.max_particles_per_system) {
                stats_.max_particles_per_system = result->particle_count;
            }
        } else {
            LOG_WARN("Failed to analyze particle system {}: {}", particle.path.string(), result.error().message);
        }
    }
    
    return results;
}

Result<ParticleInfo> ParticleAnalyzer::analyze_single(const FileInfo& particle) {
    ParticleInfo info;
    info.disk_size = particle.size;
    
    try {
        switch (particle.type) {
            case FileType::PFX:
                return analyze_pfx(particle.path);
            case FileType::PTX:
                return analyze_ptx(particle.path);
            case FileType::PAR:
                return analyze_par(particle.path);
            case FileType::PARTICLE:
                return analyze_particle(particle.path);
            case FileType::EMITTER:
                return analyze_emitter(particle.path);
            default:
                return analyze_generic(particle.path, particle.type);
        }
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Particle analysis failed: {}", e.what())));
    }
}

String ParticleAnalyzer::read_file_content(const Path& path) const {
    std::ifstream file(path);
    if (!file) return "";
    std::string content;
    file.seekg(0, std::ios::end);
    content.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return content;
}

Result<ParticleInfo> ParticleAnalyzer::analyze_pfx(const Path& path) {
    ParticleInfo info;
    info.format = "PFX";
    String content = read_file_content(path);
    info.disk_size = content.size();
    
    // PFX is used by Unity's legacy particle system
    if (content.find("Unity") != String::npos) {
        info.engine_hint = "Unity";
    }
    
    // Count particle systems and emitters
    std::regex system_regex(R"(ParticleSystem\s*{)");
    info.system_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), system_regex),
        std::sregex_iterator()
    );
    
    std::regex emitter_regex(R"(emission\s*{)");
    info.emitter_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), emitter_regex),
        std::sregex_iterator()
    );
    
    // Estimate particle count from maxParticles
    std::regex max_particles_regex(R"(maxParticles\s*:\s*(\d+))");
    std::smatch match;
    u64 total_particles = 0;
    std::string::const_iterator search_start(content.cbegin());
    while (std::regex_search(search_start, content.cend(), match, max_particles_regex)) {
        try {
            total_particles += std::stoull(match[1].str());
        } catch (...) {}
        search_start = match.suffix().first;
    }
    info.particle_count = total_particles > 0 ? total_particles : (info.emitter_count * 100);
    
    // Particle types
    if (content.find("ShapeModule") != String::npos) info.particle_types.push_back("Shape");
    if (content.find("VelocityModule") != String::npos) info.particle_types.push_back("Velocity");
    if (content.find("ColorModule") != String::npos) info.particle_types.push_back("Color");
    if (content.find("SizeModule") != String::npos) info.particle_types.push_back("Size");
    if (content.find("RotationModule") != String::npos) info.particle_types.push_back("Rotation");
    
    info.estimated_vram = estimate_vram_usage(info);
    info.estimated_ram = estimate_ram_usage(info);
    
    return info;
}

Result<ParticleInfo> ParticleAnalyzer::analyze_ptx(const Path& path) {
    ParticleInfo info;
    info.format = "PTX";
    String content = read_file_content(path);
    info.disk_size = content.size();
    
    // PTX is used by some engines for particle effects
    if (content.find("Unreal") != String::npos || content.find("UE4") != String::npos) {
        info.engine_hint = "Unreal Engine";
    }
    
    // Try to parse as text or binary
    if (content.size() > 100 && std::isprint(static_cast<unsigned char>(content[0]))) {
        // Text-based format
        std::regex particle_regex(R"(particle\s+system|emitter\s+type)");
        info.system_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), particle_regex),
            std::sregex_iterator()
        );
    } else {
        // Binary format - estimate from file size
        info.system_count = std::max(1UL, info.disk_size / (64 * 1024));
    }
    
    info.emitter_count = info.system_count * 2; // Rough estimate
    info.particle_count = info.emitter_count * 500; // Rough estimate
    
    info.estimated_vram = estimate_vram_usage(info);
    info.estimated_ram = estimate_ram_usage(info);
    
    return info;
}

Result<ParticleInfo> ParticleAnalyzer::analyze_par(const Path& path) {
    ParticleInfo info;
    info.format = "PAR";
    String content = read_file_content(path);
    info.disk_size = content.size();
    
    // PAR is used by some engines (e.g., older Unity, custom)
    if (content.find("Particle") != String::npos && content.find("Emitter") != String::npos) {
        std::regex sys_regex(R"(particle\s+system|system\s+{)");
        info.system_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), sys_regex),
            std::sregex_iterator()
        );
        
        std::regex emit_regex(R"(emitter\s+{)");
        info.emitter_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), emit_regex),
            std::sregex_iterator()
        );
    } else {
        info.system_count = 1;
    }
    
    info.emitter_count = std::max(1UL, info.emitter_count);
    info.particle_count = info.emitter_count * 200;
    
    info.estimated_vram = estimate_vram_usage(info);
    info.estimated_ram = estimate_ram_usage(info);
    
    return info;
}

Result<ParticleInfo> ParticleAnalyzer::analyze_particle(const Path& path) {
    ParticleInfo info;
    info.format = "PARTICLE";
    info.disk_size = fs::file_size(path);
    
    // Generic particle file
    info.system_count = 1;
    info.emitter_count = 1;
    info.particle_count = 100;
    
    info.estimated_vram = estimate_vram_usage(info);
    info.estimated_ram = estimate_ram_usage(info);
    
    return info;
}

Result<ParticleInfo> ParticleAnalyzer::analyze_emitter(const Path& path) {
    ParticleInfo info;
    info.format = "EMITTER";
    info.disk_size = fs::file_size(path);
    
    // Generic emitter file
    info.system_count = 1;
    info.emitter_count = 1;
    info.particle_count = 50;
    
    info.estimated_vram = estimate_vram_usage(info);
    info.estimated_ram = estimate_ram_usage(info);
    
    return info;
}

Result<ParticleInfo> ParticleAnalyzer::analyze_generic(const Path& path, FileType type) {
    ParticleInfo info;
    info.disk_size = fs::file_size(path);
    info.format = "Unknown";
    
    String content = read_file_content(path);
    if (!content.empty() && content.size() < 1024 * 1024) {
        info.system_count = 1;
        info.emitter_count = 2;
        info.particle_count = 200;
    } else {
        info.system_count = 1;
        info.emitter_count = 1;
        info.particle_count = 100;
    }
    
    info.estimated_vram = estimate_vram_usage(info);
    info.estimated_ram = estimate_ram_usage(info);
    
    return info;
}

u64 ParticleAnalyzer::estimate_vram_usage(const ParticleInfo& particle) const {
    // Particle systems use VRAM for:
    // - Particle buffer (position, velocity, color, size, rotation, lifetime)
    // - Texture atlases for particle sprites
    // - Compute shaders for simulation
    
    u64 vram = particle.particle_count * 64; // ~64 bytes per particle (position, velocity, color, size, rotation, lifetime, age)
    vram += particle.emitter_count * 4 * 1024; // Emitter data
    vram += particle.system_count * 16 * 1024; // System data
    
    // Texture atlas estimate (assume 1024x1024 RGBA8 = 4MB)
    vram += 4 * 1024 * 1024;
    
    return vram;
}

u64 ParticleAnalyzer::estimate_ram_usage(const ParticleInfo& particle) const {
    // CPU-side particle data for simulation
    u64 ram = particle.particle_count * 48; // ~48 bytes per particle (simulation data)
    ram += particle.emitter_count * 2 * 1024; // Emitter CPU data
    ram += particle.system_count * 8 * 1024; // System CPU data
    
    return ram;
}

String ParticleAnalyzer::generate_report() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    std::stringstream ss;
    ss << "Particle Analysis Report\n";
    ss << "========================\n";
    ss << "Total Systems: " << stats_.total_systems << "\n";
    ss << "Total Emitters: " << stats_.total_emitters << "\n";
    ss << "Total Particles: " << StringUtils::format_number(stats_.total_particles) << "\n";
    ss << "Total Disk Size: " << StringUtils::format_bytes(stats_.total_disk_size) << "\n";
    ss << "Estimated VRAM: " << StringUtils::format_bytes(stats_.total_estimated_vram) << "\n";
    ss << "Estimated RAM: " << StringUtils::format_bytes(stats_.total_estimated_ram) << "\n";
    ss << "Max Particles/System: " << stats_.max_particles_per_system << "\n\n";
    
    if (!stats_.format_counts.empty()) {
        ss << "Format Distribution:\n";
        for (const auto& [fmt, count] : stats_.format_counts) {
            ss << "  " << fmt << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.engine_counts.empty()) {
        ss << "Engine Detection:\n";
        for (const auto& [engine, count] : stats_.engine_counts) {
            ss << "  " << engine << ": " << count << "\n";
        }
    }
    
    return ss.str();
}

String ParticleStats::summary() const {
    std::stringstream ss;
    ss << "Particle Systems: " << total_systems 
       << ", Emitters: " << total_emitters 
       << ", Particles: " << StringUtils::format_number(total_particles)
       << ", Disk: " << StringUtils::format_bytes(total_disk_size)
       << ", VRAM: " << StringUtils::format_bytes(total_estimated_vram)
       << ", RAM: " << StringUtils::format_bytes(total_estimated_ram);
    return ss.str();
}

} // namespace game_req