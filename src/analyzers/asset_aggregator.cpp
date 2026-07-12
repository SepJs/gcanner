#include <game_req_analyzer/analyzers/asset_aggregator.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace game_req {

AssetAggregator::AssetAggregator(const AnalyzerConfig& config) : config_(config) {}

Result<AggregatedAssets> AssetAggregator::aggregate(
    const TextureStats& tex,
    const ModelStats& mdl,
    const AudioStats& aud,
    const ScriptStats& scr,
    const ExecutableStats& exe,
    const ShaderStats& shd,
    const ParticleStats& par,
    const PhysicsStats& phy,
    const AIStats& ai,
    const NetworkStats& net) {
    AggregatedAssets assets;
    assets.textures = tex;
    assets.models = mdl;
    assets.audio = aud;
    assets.scripts = scr;
    assets.executables = exe;
    assets.shaders = shd;
    assets.particles = par;
    assets.physics = phy;
    assets.ai = ai;
    assets.network = net;
    
    assets.total_disk_size = tex.total_disk_size + mdl.total_disk_size + 
                            aud.total_disk_size + scr.total_ram_estimate + 
                            exe.total_estimated_ram + shd.total_disk_size +
                            par.total_disk_size + phy.total_disk_size +
                            ai.total_disk_size + net.total_disk_size;
    assets.total_vram = tex.total_vram + mdl.total_vram + shd.total_estimated_vram + par.total_estimated_vram;
    assets.total_ram_estimate = estimate_ram_usage();
    assets.total_cpu_work = estimate_cpu_work();
    assets.total_gpu_work = estimate_gpu_work();
    
    calculate_ratios();
    detect_engines();
    
    aggregated_ = assets;
    return assets;
}

String AssetAggregator::generate_full_report() const {
    std::stringstream ss;
    ss << "=== Asset Aggregation Report ===\n\n";
    
    ss << "--- Texture Statistics ---\n";
    ss << "  Total Textures: " << aggregated_.textures.total_textures << "\n";
    ss << "  Total Disk Size: " << StringUtils::format_bytes(aggregated_.textures.total_disk_size) << "\n";
    ss << "  Estimated VRAM: " << StringUtils::format_bytes(aggregated_.textures.total_vram) << "\n";
    ss << "  Max Resolution: " << aggregated_.textures.max_width << "x" << aggregated_.textures.max_height << "\n";
    ss << "  Avg Compression: " << std::fixed << std::setprecision(2) << aggregated_.textures.avg_compression_ratio << "x\n\n";
    
    ss << "--- Model Statistics ---\n";
    ss << "  Total Models: " << aggregated_.models.total_models << "\n";
    ss << "  Total Vertices: " << StringUtils::format_number(aggregated_.models.total_vertices) << "\n";
    ss << "  Total Triangles: " << StringUtils::format_number(aggregated_.models.total_triangles) << "\n";
    ss << "  Total Meshes: " << aggregated_.models.total_meshes << "\n";
    ss << "  Total Materials: " << aggregated_.models.total_materials << "\n";
    ss << "  Total Bones: " << aggregated_.models.total_bones << "\n";
    ss << "  Total Disk Size: " << StringUtils::format_bytes(aggregated_.models.total_disk_size) << "\n";
    ss << "  Estimated VRAM: " << StringUtils::format_bytes(aggregated_.models.total_vram) << "\n\n";
    
    ss << "--- Audio Statistics ---\n";
    ss << "  Total Files: " << aggregated_.audio.total_files << "\n";
    ss << "  Total Duration: " << (aggregated_.audio.total_duration_ms / 1000) << "s\n";
    ss << "  Total Disk Size: " << StringUtils::format_bytes(aggregated_.audio.total_disk_size) << "\n";
    ss << "  Estimated RAM: " << StringUtils::format_bytes(aggregated_.audio.total_estimated_ram) << "\n\n";
    
    ss << "--- Script Statistics ---\n";
    ss << "  Total Scripts: " << aggregated_.scripts.total_scripts << "\n";
    ss << "  Total Lines: " << StringUtils::format_number(aggregated_.scripts.total_lines) << "\n";
    ss << "  Total Functions: " << StringUtils::format_number(aggregated_.scripts.total_functions) << "\n";
    ss << "  Total Classes: " << StringUtils::format_number(aggregated_.scripts.total_classes) << "\n";
    ss << "  Total Complexity: " << StringUtils::format_number(aggregated_.scripts.total_complexity) << "\n";
    ss << "  Estimated RAM: " << StringUtils::format_bytes(aggregated_.scripts.total_ram_estimate) << "\n\n";
    
    ss << "--- Executable Statistics ---\n";
    ss << "  Total Executables: " << aggregated_.executables.total_executables << "\n";
    ss << "  Total Code Size: " << StringUtils::format_bytes(aggregated_.executables.total_code_size) << "\n";
    ss << "  Total Data Size: " << StringUtils::format_bytes(aggregated_.executables.total_data_size) << "\n";
    ss << "  Estimated RAM: " << StringUtils::format_bytes(aggregated_.executables.total_estimated_ram) << "\n\n";
    
    ss << "--- Shader Statistics ---\n";
    ss << "  Total Shaders: " << aggregated_.shaders.total_shaders << "\n";
    ss << "  Total Instructions: " << StringUtils::format_number(aggregated_.shaders.total_instructions) << "\n";
    ss << "  Total Disk Size: " << StringUtils::format_bytes(aggregated_.shaders.total_disk_size) << "\n";
    ss << "  Estimated VRAM: " << StringUtils::format_bytes(aggregated_.shaders.total_estimated_vram) << "\n\n";
    
    ss << "--- Particle Statistics ---\n";
    ss << "  Total Systems: " << aggregated_.particles.total_systems << "\n";
    ss << "  Total Emitters: " << aggregated_.particles.total_emitters << "\n";
    ss << "  Total Particles: " << StringUtils::format_number(aggregated_.particles.total_particles) << "\n";
    ss << "  Total Disk Size: " << StringUtils::format_bytes(aggregated_.particles.total_disk_size) << "\n";
    ss << "  Estimated VRAM: " << StringUtils::format_bytes(aggregated_.particles.total_estimated_vram) << "\n";
    ss << "  Estimated RAM: " << StringUtils::format_bytes(aggregated_.particles.total_estimated_ram) << "\n\n";
    
    ss << "--- Physics Statistics ---\n";
    ss << "  Total Scenes: " << aggregated_.physics.total_scenes << "\n";
    ss << "  Total Bodies: " << StringUtils::format_number(aggregated_.physics.total_bodies) << "\n";
    ss << "  Total Shapes: " << StringUtils::format_number(aggregated_.physics.total_shapes) << "\n";
    ss << "  Total Constraints: " << StringUtils::format_number(aggregated_.physics.total_constraints) << "\n";
    ss << "  Total Disk Size: " << StringUtils::format_bytes(aggregated_.physics.total_disk_size) << "\n";
    ss << "  Estimated RAM: " << StringUtils::format_bytes(aggregated_.physics.total_estimated_ram) << "\n\n";
    
    ss << "--- AI Statistics ---\n";
    ss << "  Total AI Systems: " << aggregated_.ai.total_ai_systems << "\n";
    ss << "  Total Agents: " << StringUtils::format_number(aggregated_.ai.total_agents) << "\n";
    ss << "  Total BT Nodes: " << StringUtils::format_number(aggregated_.ai.total_bt_nodes) << "\n";
    ss << "  Total States: " << StringUtils::format_number(aggregated_.ai.total_states) << "\n";
    ss << "  Total NavMeshes: " << aggregated_.ai.total_navmeshes << "\n";
    ss << "  Total Disk Size: " << StringUtils::format_bytes(aggregated_.ai.total_disk_size) << "\n";
    ss << "  Estimated RAM: " << StringUtils::format_bytes(aggregated_.ai.total_estimated_ram) << "\n\n";
    
    ss << "--- Network Statistics ---\n";
    ss << "  Total Network Systems: " << aggregated_.network.total_systems << "\n";
    ss << "  Total Connections: " << StringUtils::format_number(aggregated_.network.total_connections) << "\n";
    ss << "  Total Message Types: " << StringUtils::format_number(aggregated_.network.total_message_types) << "\n";
    ss << "  Total Disk Size: " << StringUtils::format_bytes(aggregated_.network.total_disk_size) << "\n";
    ss << "  Estimated RAM: " << StringUtils::format_bytes(aggregated_.network.total_estimated_ram) << "\n";
    ss << "  Estimated Bandwidth: " << StringUtils::format_bytes(aggregated_.network.total_estimated_bandwidth) << "/sec\n\n";
    
    ss << "--- Aggregated Totals ---\n";
    ss << "  Total Disk Size: " << StringUtils::format_bytes(aggregated_.total_disk_size) << "\n";
    ss << "  Total VRAM Estimate: " << StringUtils::format_bytes(aggregated_.total_vram) << "\n";
    ss << "  Total RAM Estimate: " << StringUtils::format_bytes(aggregated_.total_ram_estimate) << "\n";
    ss << "  CPU Work Estimate: " << StringUtils::format_number(aggregated_.total_cpu_work) << "\n";
    ss << "  GPU Work Estimate: " << StringUtils::format_number(aggregated_.total_gpu_work) << "\n\n";
    
    ss << "--- Ratios ---\n";
    ss << "  Texture/Model: " << std::fixed << std::setprecision(2) << aggregated_.texture_to_model_ratio << "\n";
    ss << "  Audio/Total: " << std::fixed << std::setprecision(2) << aggregated_.audio_to_total_ratio * 100 << "%\n";
    ss << "  Script/Total: " << std::fixed << std::setprecision(2) << aggregated_.script_to_total_ratio * 100 << "%\n";
    ss << "  Executable/Total: " << std::fixed << std::setprecision(2) << aggregated_.executable_to_total_ratio * 100 << "%\n";
    ss << "  Shader/Total: " << std::fixed << std::setprecision(2) << aggregated_.shader_to_total_ratio * 100 << "%\n";
    ss << "  Particle/Total: " << std::fixed << std::setprecision(2) << aggregated_.particle_to_total_ratio * 100 << "%\n";
    ss << "  Physics/Total: " << std::fixed << std::setprecision(2) << aggregated_.physics_to_total_ratio * 100 << "%\n";
    ss << "  AI/Total: " << std::fixed << std::setprecision(2) << aggregated_.ai_to_total_ratio * 100 << "%\n";
    ss << "  Network/Total: " << std::fixed << std::setprecision(2) << aggregated_.network_to_total_ratio * 100 << "%\n\n";
    
    ss << "--- Engine Detection ---\n";
    ss << "  Primary Engine: " << (aggregated_.primary_engine.empty() ? "Unknown" : aggregated_.primary_engine) << "\n";
    ss << "  Primary Renderer: " << (aggregated_.primary_renderer.empty() ? "Unknown" : aggregated_.primary_renderer) << "\n";
    ss << "  Primary Audio: " << (aggregated_.primary_audio.empty() ? "Unknown" : aggregated_.primary_audio) << "\n";
    ss << "  Primary Physics: " << (aggregated_.primary_physics.empty() ? "Unknown" : aggregated_.primary_physics) << "\n";
    ss << "  Primary Scripting: " << (aggregated_.primary_scripting.empty() ? "Unknown" : aggregated_.primary_scripting) << "\n";
    ss << "  Primary Shader Lang: " << (aggregated_.primary_shader_lang.empty() ? "Unknown" : aggregated_.primary_shader_lang) << "\n";
    
    return ss.str();
}

void AssetAggregator::calculate_ratios() {
    if (aggregated_.models.total_disk_size > 0) {
        aggregated_.texture_to_model_ratio = static_cast<f32>(aggregated_.textures.total_disk_size) / 
                                             static_cast<f32>(aggregated_.models.total_disk_size);
    }
    
    u64 total = aggregated_.total_disk_size;
    if (total > 0) {
        aggregated_.audio_to_total_ratio = static_cast<f32>(aggregated_.audio.total_disk_size) / total;
        aggregated_.script_to_total_ratio = static_cast<f32>(aggregated_.scripts.total_ram_estimate) / total;
        aggregated_.executable_to_total_ratio = static_cast<f32>(aggregated_.executables.total_estimated_ram) / total;
        aggregated_.shader_to_total_ratio = static_cast<f32>(aggregated_.shaders.total_disk_size) / total;
        aggregated_.particle_to_total_ratio = static_cast<f32>(aggregated_.particles.total_disk_size) / total;
        aggregated_.physics_to_total_ratio = static_cast<f32>(aggregated_.physics.total_disk_size) / total;
        aggregated_.ai_to_total_ratio = static_cast<f32>(aggregated_.ai.total_disk_size) / total;
        aggregated_.network_to_total_ratio = static_cast<f32>(aggregated_.network.total_disk_size) / total;
    }
}

void AssetAggregator::detect_engines() {
    std::unordered_map<String, u32> engine_votes;
    
    for (const auto& [engine, count] : aggregated_.models.engine_counts) {
        engine_votes[engine] += count * 10;
    }
    
    for (const auto& [engine, count] : aggregated_.scripts.engine_counts) {
        engine_votes[engine] += count * 5;
    }
    
    for (const auto& [engine, count] : aggregated_.executables.engine_counts) {
        engine_votes[engine] += count * 20;
    }
    
    if (!engine_votes.empty()) {
        auto best = std::max_element(engine_votes.begin(), engine_votes.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        aggregated_.primary_engine = best->first;
    }
    
    if (aggregated_.primary_engine.find("Unreal") != String::npos) {
        aggregated_.primary_renderer = "Unreal Engine Renderer";
        aggregated_.primary_audio = "Unreal Audio Engine";
        aggregated_.primary_physics = "Chaos Physics";
        aggregated_.primary_scripting = "Blueprints / C++";
    } else if (aggregated_.primary_engine.find("Unity") != String::npos) {
        aggregated_.primary_renderer = "Unity Render Pipeline (URP/HDRP)";
        aggregated_.primary_audio = "Unity Audio / FMOD";
        aggregated_.primary_physics = "PhysX / Box2D";
        aggregated_.primary_scripting = "C# / IL2CPP";
    } else if (aggregated_.primary_engine.find("Godot") != String::npos) {
        aggregated_.primary_renderer = "Godot Renderer";
        aggregated_.primary_audio = "Godot Audio";
        aggregated_.primary_physics = "Godot Physics / Bullet";
        aggregated_.primary_scripting = "GDScript / C#";
    } else if (aggregated_.primary_engine.find("CryEngine") != String::npos || 
               aggregated_.primary_engine.find("CRYENGINE") != String::npos) {
        aggregated_.primary_renderer = "CryEngine Renderer";
        aggregated_.primary_audio = "CryAudio";
        aggregated_.primary_physics = "CryPhysics";
        aggregated_.primary_scripting = "C++ / Lua / Schematyc";
    } else if (aggregated_.primary_engine.find("Frostbite") != String::npos) {
        aggregated_.primary_renderer = "Frostbite Renderer";
        aggregated_.primary_audio = "Frostbite Audio";
        aggregated_.primary_physics = "Frostbite Physics";
        aggregated_.primary_scripting = "C++ / Frostbite Script";
    } else if (aggregated_.primary_engine.find("Source") != String::npos) {
        aggregated_.primary_renderer = "Source Engine Renderer";
        aggregated_.primary_audio = "OpenAL / XAudio2";
        aggregated_.primary_physics = "Havok / VPhysics";
        aggregated_.primary_scripting = "VScript / C++";
    } else if (!aggregated_.scripts.language_counts.empty()) {
        auto best_lang = std::max_element(aggregated_.scripts.language_counts.begin(), 
                                          aggregated_.scripts.language_counts.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        
        if (best_lang->first == "Lua") {
            aggregated_.primary_scripting = "Lua";
            aggregated_.primary_engine = "Lua-based (Love2D, Corona, Defold, Custom)";
        } else if (best_lang->first == "C#") {
            aggregated_.primary_scripting = "C#";
            if (aggregated_.primary_engine.empty()) aggregated_.primary_engine = "C#/.NET based";
        } else if (best_lang->first == "Python") {
            aggregated_.primary_scripting = "Python";
            if (aggregated_.primary_engine.empty()) aggregated_.primary_engine = "Python-based (Panda3D, Godot, Custom)";
        } else if (best_lang->first == "JavaScript" || best_lang->first == "TypeScript") {
            aggregated_.primary_scripting = "JavaScript/TypeScript";
            if (aggregated_.primary_engine.empty()) aggregated_.primary_engine = "Web-based (Three.js, Babylon.js, Custom)";
        }
    }
}

u64 AssetAggregator::estimate_cpu_work() const {
    u64 work = 0;
    
    work += aggregated_.scripts.total_complexity;
    work += aggregated_.models.total_bones * 1000;
    work += aggregated_.models.total_meshes * 500;
    work += aggregated_.audio.total_files * 100;
    work += aggregated_.executables.total_code_size / 1024;
    work += aggregated_.shaders.total_instructions / 10;
    work += aggregated_.particles.total_particles / 100;
    work += aggregated_.physics.total_bodies * 100;
    work += aggregated_.physics.total_constraints * 50;
    work += aggregated_.ai.total_agents * 500;
    work += aggregated_.ai.total_bt_nodes * 10;
    work += aggregated_.ai.total_states * 50;
    work += aggregated_.network.total_connections * 1000;
    work += aggregated_.network.total_message_types * 100;
    
    return work;
}

u64 AssetAggregator::estimate_gpu_work() const {
    u64 work = 0;
    
    work += aggregated_.models.total_vertices * 2;
    work += aggregated_.models.total_triangles * 3;
    work += aggregated_.textures.total_textures * 1000;
    work += aggregated_.models.total_materials * 5000;
    work += aggregated_.shaders.total_instructions * 5;
    work += aggregated_.particles.total_particles * 20;
    work += aggregated_.physics.total_shapes * 50;
    
    return work;
}

u64 AssetAggregator::estimate_ram_usage() const {
    u64 ram = 0;
    
    ram += aggregated_.textures.total_disk_size * 0.3;
    ram += aggregated_.models.total_disk_size * 0.2;
    ram += aggregated_.audio.total_disk_size * 0.1;
    ram += aggregated_.scripts.total_ram_estimate;
    ram += aggregated_.executables.total_estimated_ram;
    ram += aggregated_.shaders.total_estimated_vram;
    ram += aggregated_.particles.total_estimated_ram;
    ram += aggregated_.physics.total_estimated_ram;
    ram += aggregated_.ai.total_estimated_ram;
    ram += aggregated_.network.total_estimated_ram;
    ram += 2 * GiB;
    
    return ram;
}

} // namespace game_req
