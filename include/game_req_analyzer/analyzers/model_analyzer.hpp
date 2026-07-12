#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>

namespace game_req {

struct Bounds {
    f32 min_x = 0, min_y = 0, min_z = 0;
    f32 max_x = 0, max_y = 0, max_z = 0;
    [[nodiscard]] f32 width() const { return max_x - min_x; }
    [[nodiscard]] f32 height() const { return max_y - min_y; }
    [[nodiscard]] f32 depth() const { return max_z - min_z; }
};

struct ModelInfo {
    u64 vertex_count = 0;
    u64 triangle_count = 0;
    u64 mesh_count = 0;
    u64 material_count = 0;
    u64 bone_count = 0;
    u64 morph_target_count = 0;
    u32 lod_count = 1;
    u64 vertex_buffer_size = 0;
    u64 index_buffer_size = 0;
    u64 texture_count = 0;
    u64 estimated_vram = 0;
    u64 disk_size = 0;
    String format;
    String engine_hint;
    std::vector<String> animations;
    std::vector<String> materials;
    Bounds bounds{};
    String error_message;  // For error reporting
    u64 normal_count = 0;
    u64 texcoord_count = 0;
    u64 face_count = 0;
    bool is_compiled = false;
};

struct ModelStats {
    u64 total_models = 0;
    u64 total_vertices = 0;
    u64 total_triangles = 0;
    u64 total_meshes = 0;
    u64 total_materials = 0;
    u64 total_bones = 0;
    u64 total_vram = 0;
    u64 total_disk_size = 0;
    std::unordered_map<String, u32> format_counts;
    std::unordered_map<String, u32> engine_counts;
    u32 max_vertices_per_model = 0;
    u32 max_triangles_per_model = 0;
    
    [[nodiscard]] String summary() const;
};

class ModelAnalyzer {
public:
    explicit ModelAnalyzer(const AnalyzerConfig& config);
    
    Result<std::vector<ModelInfo>> analyze(const std::vector<FileInfo>& models);
    Result<ModelInfo> analyze_single(const FileInfo& model);
    [[nodiscard]] ModelStats stats() const { return stats_; }
    [[nodiscard]] String generate_report() const;

private:
    AnalyzerConfig config_;
    ModelStats stats_;
    mutable std::mutex stats_mutex_;
    
    Result<ModelInfo> analyze_fbx(const Path& path);
    Result<ModelInfo> analyze_obj(const Path& path);
    Result<ModelInfo> analyze_gltf(const Path& path);
    Result<ModelInfo> analyze_glb(const Path& path);
    Result<ModelInfo> analyze_collada(const Path& path);
    Result<ModelInfo> analyze_usd(const Path& path);
    Result<ModelInfo> analyze_usdz(const Path& path);
    Result<ModelInfo> analyze_blend(const Path& path);
    Result<ModelInfo> analyze_max(const Path& path);
    Result<ModelInfo> analyze_ma(const Path& path);
    Result<ModelInfo> analyze_mb(const Path& path);
    Result<ModelInfo> analyze_x(const Path& path);
    Result<ModelInfo> analyze_mdl(const Path& path);
    Result<ModelInfo> analyze_md2(const Path& path);
    Result<ModelInfo> analyze_md3(const Path& path);
    Result<ModelInfo> analyze_md5(const Path& path);
    Result<ModelInfo> analyze_nif(const Path& path);
    Result<ModelInfo> analyze_hkx(const Path& path);
    Result<ModelInfo> analyze_gr2(const Path& path);
    Result<ModelInfo> analyze_smd(const Path& path);
    Result<ModelInfo> analyze_dmx(const Path& path);
    Result<ModelInfo> analyze_generic(const Path& path, FileType type);
    
    u64 estimate_vram(const ModelInfo& model) const;
    String detect_engine(const ModelInfo& model, const Path& path) const;
};

} // namespace game_req