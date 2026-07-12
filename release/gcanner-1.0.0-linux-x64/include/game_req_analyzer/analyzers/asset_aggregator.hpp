#pragma once
#include <game_req_analyzer/core/error.hpp>

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/analyzers/texture_analyzer.hpp>
#include <game_req_analyzer/analyzers/model_analyzer.hpp>
#include <game_req_analyzer/analyzers/audio_analyzer.hpp>
#include <game_req_analyzer/analyzers/script_analyzer.hpp>
#include <game_req_analyzer/analyzers/executable_analyzer.hpp>
#include <game_req_analyzer/analyzers/shader_analyzer.hpp>
#include <game_req_analyzer/analyzers/particle_analyzer.hpp>
#include <game_req_analyzer/analyzers/physics_analyzer.hpp>
#include <game_req_analyzer/analyzers/ai_analyzer.hpp>
#include <game_req_analyzer/analyzers/network_analyzer.hpp>

namespace game_req {

struct AggregatedAssets {
    TextureStats textures;
    ModelStats models;
    AudioStats audio;
    ScriptStats scripts;
    ExecutableStats executables;
    ShaderStats shaders;
    ParticleStats particles;
    PhysicsStats physics;
    AIStats ai;
    NetworkStats network;
    
    u64 total_disk_size = 0;
    u64 total_vram = 0;
    u64 total_ram_estimate = 0;
    u64 total_cpu_work = 0;
    u64 total_gpu_work = 0;
    
    f32 texture_to_model_ratio = 0.0f;
    f32 audio_to_total_ratio = 0.0f;
    f32 script_to_total_ratio = 0.0f;
    f32 executable_to_total_ratio = 0.0f;
    f32 shader_to_total_ratio = 0.0f;
    f32 particle_to_total_ratio = 0.0f;
    f32 physics_to_total_ratio = 0.0f;
    f32 ai_to_total_ratio = 0.0f;
    f32 network_to_total_ratio = 0.0f;
    
    String primary_engine;
    String primary_renderer;
    String primary_audio;
    String primary_physics;
    String primary_scripting;
    String primary_shader_lang;
    
    [[nodiscard]] String summary() const;
};

class AssetAggregator {
public:
    explicit AssetAggregator(const AnalyzerConfig& config);
    
    Result<AggregatedAssets> aggregate(
        const TextureStats& tex,
        const ModelStats& mdl,
        const AudioStats& aud,
        const ScriptStats& scr,
        const ExecutableStats& exe,
        const ShaderStats& shd,
        const ParticleStats& par,
        const PhysicsStats& phy,
        const AIStats& ai,
        const NetworkStats& net
    );
    
    [[nodiscard]] AggregatedAssets stats() const { return aggregated_; }
    [[nodiscard]] String generate_full_report() const;

private:
    AnalyzerConfig config_;
    AggregatedAssets aggregated_;
    
    void calculate_ratios();
    void detect_engines();
    u64 estimate_cpu_work() const;
    u64 estimate_gpu_work() const;
    u64 estimate_ram_usage() const;
};

} // namespace game_req