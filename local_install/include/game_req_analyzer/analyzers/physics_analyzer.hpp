#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>

namespace game_req {

struct PhysicsInfo {
    u64 body_count = 0;
    u64 shape_count = 0;
    u64 constraint_count = 0;
    u64 collision_pair_count = 0;
    u64 disk_size = 0;
    u64 estimated_ram = 0;
    String format;           // PhysX, Havok, Bullet, ODE, Newton, PhysX3, custom
    String engine_hint;
    String physics_engine;
    std::vector<String> shape_types;  // Box, Sphere, Capsule, Convex, Mesh, HeightField
    u32 max_bodies_per_scene = 0;
    bool has_continuous_collision = false;
    bool has_vehicle_support = false;
    bool has_character_controller = false;
    bool has_soft_body = false;
    bool has_fluid = false;
    bool has_cloth = false;
    bool has_particles = false;
};

struct PhysicsStats {
    u64 total_scenes = 0;
    u64 total_bodies = 0;
    u64 total_shapes = 0;
    u64 total_constraints = 0;
    u64 total_disk_size = 0;
    u64 total_estimated_ram = 0;
    std::unordered_map<String, u32> format_counts;
    std::unordered_map<String, u32> engine_counts;
    std::unordered_map<String, u32> shape_type_counts;
    u32 max_bodies_per_scene = 0;
    
    [[nodiscard]] String summary() const;
};

class PhysicsAnalyzer {
public:
    explicit PhysicsAnalyzer(const AnalyzerConfig& config);
    
    Result<std::vector<PhysicsInfo>> analyze(const std::vector<FileInfo>& physics_files);
    Result<PhysicsInfo> analyze_single(const FileInfo& physics);
    [[nodiscard]] PhysicsStats stats() const { return stats_; }
    [[nodiscard]] String generate_report() const;

private:
    AnalyzerConfig config_;
    PhysicsStats stats_;
    mutable std::mutex stats_mutex_;
    
    Result<PhysicsInfo> analyze_physx(const Path& path);
    Result<PhysicsInfo> analyze_havok(const Path& path);
    Result<PhysicsInfo> analyze_bullet(const Path& path);
    Result<PhysicsInfo> analyze_ode(const Path& path);
    Result<PhysicsInfo> analyze_newton(const Path& path);
    Result<PhysicsInfo> analyze_physx3(const Path& path);
    Result<PhysicsInfo> analyze_pxd(const Path& path);
    Result<PhysicsInfo> analyze_pxb(const Path& path);
    Result<PhysicsInfo> analyze_generic(const Path& path, FileType type);
    
    String read_file_content(const Path& path) const;
    u64 estimate_ram_usage(const PhysicsInfo& physics) const;
    void detect_physics_engine(PhysicsInfo& physics, StringView content) const;
};

} // namespace game_req