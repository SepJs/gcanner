#include <game_req_analyzer/analyzers/requirement_calculator.hpp>
#include <game_req_analyzer/hardware/hardware_database.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_map>

namespace game_req {

RequirementCalculator::RequirementCalculator(const AnalyzerConfig& config, const HardwareDatabase& hw_db)
    : config_(config), hw_db_(hw_db) {}

Result<RequirementResult> RequirementCalculator::calculate(const AggregatedAssets& assets) {
    RequirementResult result;
    result.confidence = 0.0f;
    result.analysis_notes.clear();
    result.warnings.clear();
    result.assumptions.clear();
    
    result.assumptions.push_back("Analysis based on static asset inspection only");
    result.assumptions.push_back("Runtime memory usage estimated from asset data");
    result.assumptions.push_back("Shader complexity inferred from file count and size");
    result.assumptions.push_back("Targeting 1080p60 as baseline for calculations");
    
    // Create default game profile (1080p60, High quality)
    GameProfile profile;
    profile.fps_target = GameProfile::FpsTarget::FPS_60;
    profile.resolution_target = GameProfile::ResolutionTarget::RES_1080p;
    profile.graphics_quality = GameProfile::GraphicsQuality::QUALITY_HIGH;
    
    // Calculate requirements for each level
    calculate_cpu_req(assets, result.minimum, RequirementLevel::Minimum, profile);
    calculate_cpu_req(assets, result.recommended, RequirementLevel::Recommended, profile);
    calculate_cpu_req(assets, result.high, RequirementLevel::High, profile);
    calculate_cpu_req(assets, result.ultra, RequirementLevel::Ultra, profile);
    
    GameProfile rt_profile = profile;
    rt_profile.graphics_quality = GameProfile::GraphicsQuality::QUALITY_RAY_TRACING;
    rt_profile.ray_tracing_enabled = true;
    result.ray_tracing.emplace();
    calculate_cpu_req(assets, *result.ray_tracing, RequirementLevel::RayTracing, rt_profile);
    
    calculate_gpu_req(assets, result.minimum, RequirementLevel::Minimum, profile);
    calculate_gpu_req(assets, result.recommended, RequirementLevel::Recommended, profile);
    calculate_gpu_req(assets, result.high, RequirementLevel::High, profile);
    calculate_gpu_req(assets, result.ultra, RequirementLevel::Ultra, profile);
    calculate_gpu_req(assets, *result.ray_tracing, RequirementLevel::RayTracing, rt_profile);
    
    calculate_ram_req(assets, result.minimum, RequirementLevel::Minimum, profile);
    calculate_ram_req(assets, result.recommended, RequirementLevel::Recommended, profile);
    calculate_ram_req(assets, result.high, RequirementLevel::High, profile);
    calculate_ram_req(assets, result.ultra, RequirementLevel::Ultra, profile);
    calculate_ram_req(assets, *result.ray_tracing, RequirementLevel::RayTracing, rt_profile);
    
    calculate_storage_req(assets, result.minimum, RequirementLevel::Minimum, profile);
    calculate_storage_req(assets, result.recommended, RequirementLevel::Recommended, profile);
    calculate_storage_req(assets, result.high, RequirementLevel::High, profile);
    calculate_storage_req(assets, result.ultra, RequirementLevel::Ultra, profile);
    calculate_storage_req(assets, *result.ray_tracing, RequirementLevel::RayTracing, rt_profile);
    
    calculate_os_req(assets, result.minimum);
    result.recommended.os_minimum = result.minimum.os_minimum;
    result.recommended.os_recommended = result.minimum.os_recommended;
    result.high.os_minimum = result.minimum.os_minimum;
    result.high.os_recommended = result.minimum.os_recommended;
    result.ultra.os_minimum = result.minimum.os_minimum;
    result.ultra.os_recommended = result.minimum.os_recommended;
    result.ray_tracing->os_minimum = result.minimum.os_minimum;
    result.ray_tracing->os_recommended = result.minimum.os_recommended;
    
    calculate_display_req(assets, result.minimum, RequirementLevel::Minimum, profile);
    calculate_display_req(assets, result.recommended, RequirementLevel::Recommended, profile);
    calculate_display_req(assets, result.high, RequirementLevel::High, profile);
    calculate_display_req(assets, result.ultra, RequirementLevel::Ultra, profile);
    calculate_display_req(assets, *result.ray_tracing, RequirementLevel::RayTracing, rt_profile);
    
    calculate_api_req(assets, result.minimum);
    result.recommended.dx_version = result.minimum.dx_version;
    result.recommended.vk_version = result.minimum.vk_version;
    result.recommended.metal_version = result.minimum.metal_version;
    result.high.dx_version = result.minimum.dx_version;
    result.high.vk_version = result.minimum.vk_version;
    result.high.metal_version = result.minimum.metal_version;
    result.ultra.dx_version = result.minimum.dx_version;
    result.ultra.vk_version = result.minimum.vk_version;
    result.ultra.metal_version = result.minimum.metal_version;
    result.ray_tracing->dx_version = result.minimum.dx_version;
    result.ray_tracing->vk_version = result.minimum.vk_version;
    result.ray_tracing->metal_version = result.minimum.metal_version;
    
    // Find matching hardware from database
    find_matching_hardware(result.minimum, RequirementLevel::Minimum);
    find_matching_hardware(result.recommended, RequirementLevel::Recommended);
    find_matching_hardware(result.high, RequirementLevel::High);
    find_matching_hardware(result.ultra, RequirementLevel::Ultra);
    find_matching_hardware(*result.ray_tracing, RequirementLevel::RayTracing);
    
    result.confidence = calculate_confidence(assets);
    
    // Add warnings based on missing data
    if (assets.executables.total_code_size == 0) {
        result.warnings.push_back("No executables found - CPU requirements estimated from assets only");
    }
    if (assets.textures.total_textures == 0) {
        result.warnings.push_back("No textures found - VRAM requirements may be inaccurate");
    }
    if (assets.models.total_models == 0) {
        result.warnings.push_back("No 3D models found - GPU compute requirements estimated from other factors");
    }
    if (assets.scripts.total_scripts == 0) {
        result.warnings.push_back("No scripts found - AI/physics requirements may be underestimated");
    }
    
    // Add bottleneck analysis
    f32 bottleneck_factor = calculate_bottleneck_factor(assets);
    if (bottleneck_factor > 1.5f) {
        result.analysis_notes = "System appears to be GPU-bound";
    } else if (bottleneck_factor < 0.7f) {
        result.analysis_notes = "System appears to be CPU-bound";
    } else {
        result.analysis_notes = "System appears to be well-balanced";
    }
    
    return result;
}

u64 RequirementCalculator::estimate_cpu_requirement(const AggregatedAssets& assets, RequirementLevel level, 
                                                   const GameProfile& profile) {
    // Base CPU score calculation
    f32 base_score = 2000.0f; // Base PassMark single-thread score
    
    // Application complexity factors
    f32 script_factor = 1.0f;
    if (assets.scripts.total_complexity > 0) {
        // Logarithmic scaling to prevent extreme values
        script_factor = 1.0f + std::log10f(1.0f + assets.scripts.total_complexity / 5000.0f) * 0.25f;
    }
    
    f32 physics_factor = 1.0f;
    if (assets.models.total_bones > 0) {
        // More bones = more complex animation/physics
        physics_factor = 1.0f + std::log10f(1.0f + assets.models.total_bones / 500.0f) * 0.2f;
    }
    
    f32 ai_factor = 1.0f;
    if (assets.scripts.total_functions > 0) {
        // More functions = potentially more complex AI/logic
        ai_factor = 1.0f + std::log10f(1.0f + assets.scripts.total_functions / 2000.0f) * 0.15f;
    }
    
    f32 executable_factor = 1.0f;
    if (assets.executables.total_code_size > 0) {
        // Larger executables = more complex game logic
        executable_factor = 1.0f + std::log10f(1.0f + assets.executables.total_code_size / (5.0f * MiB)) * 0.2f;
    }
    
    // Quality and resolution factors
    f32 quality_factor = QUALITY_FACTORS[static_cast<size_t>(profile.graphics_quality)];
    f32 resolution_factor = RESOLUTION_FACTORS[static_cast<size_t>(profile.resolution_target)];
    f32 fps_factor = FPS_TARGETS[static_cast<size_t>(profile.fps_target)] / 60.0f; // Normalize to 60 FPS baseline
    
    // Engine-specific adjustments (simplified)
    f32 engine_factor = 1.0f;
    if (!assets.primary_engine.empty()) {
        // Some engines are more CPU-intensive
        if (assets.primary_engine == "Unity" || assets.primary_engine == "Unreal Engine") {
            engine_factor = 1.2f;
        } else if (assets.primary_engine == "Source" || assets.primary_engine == "Source 2") {
            engine_factor = 0.9f; // Source engines are often more GPU-bound
        }
    }
    
    // Calculate final score
    f32 total_factor = script_factor * physics_factor * ai_factor * executable_factor * 
                      quality_factor * resolution_factor * fps_factor * engine_factor;
    
    u64 estimated_score = static_cast<u64>(base_score * total_factor);
    
    // Apply reasonable bounds
    return std::clamp(estimated_score, u64(800), u64(45000));
}

u64 RequirementCalculator::estimate_gpu_requirement(const AggregatedAssets& assets, RequirementLevel level, 
                                                   const GameProfile& profile) {
    // Base GPU score calculation
    f32 base_score = 1500.0f; // Base PassMark G3D score
    
    // Graphics complexity factors
    f32 vertex_factor = 1.0f;
    if (assets.models.total_vertices > 0) {
        // More vertices = more geometry to process
        vertex_factor = 1.0f + std::log10f(1.0f + assets.models.total_vertices / 500000.0f) * 0.3f;
    }
    
    f32 triangle_factor = 1.0f;
    if (assets.models.total_triangles > 0) {
        // More triangles = more rendering work
        triangle_factor = 1.0f + std::log10f(1.0f + assets.models.total_triangles / 1000000.0f) * 0.25f;
    }
    
    f32 texture_factor = 1.0f;
    if (assets.textures.total_textures > 0) {
        // More textures = more texture sampling
        texture_factor = 1.0f + std::log10f(1.0f + assets.textures.total_textures / 1000.0f) * 0.2f;
    }
    
    f32 shader_factor = 1.0f;
    if (assets.executables.total_code_size > 0 || assets.scripts.total_functions > 0) {
        // Shaders and effects
        shader_factor = 1.0f + 0.3f; // Base shader complexity
    }
    
    // Additional effects
    f32 particle_factor = 1.0f;
    // Particle systems would be detected here if we had particle analyzer
    
    f32 physics_factor = 1.0f;
    if (assets.models.total_bones > 0) {
        // Physics calculations often GPU-accelerated
        physics_factor = 1.0f + std::log10f(1.0f + assets.models.total_bones / 200.0f) * 0.15f;
    }
    
    // Quality and resolution factors
    f32 quality_factor = QUALITY_FACTORS[static_cast<size_t>(profile.graphics_quality)];
    f32 resolution_factor = RESOLUTION_FACTORS[static_cast<size_t>(profile.resolution_target)];
    f32 fps_factor = FPS_TARGETS[static_cast<size_t>(profile.fps_target)] / 60.0f; // Normalize to 60 FPS baseline
    
    // Special features
    f32 rt_factor = 1.0f;
    if (profile.ray_tracing_enabled) {
        rt_factor = 2.5f; // Ray tracing is very GPU-intensive
    }
    
    f32 upscale_factor = 1.0f;
    if (profile.dlss_enabled || profile.fsr_enabled) {
        upscale_factor = 0.7f; // Upscaling reduces GPU load
    }
    
    // Engine-specific adjustments
    f32 engine_factor = 1.0f;
    if (!assets.primary_engine.empty()) {
        // Some engines are more GPU-intensive
        if (assets.primary_engine == "Unreal Engine" || assets.primary_engine == "CryEngine") {
            engine_factor = 1.3f;
        } else if (assets.primary_engine == "Unity") {
            engine_factor = 1.1f;
        }
    }
    
    // Calculate final score
    f32 total_factor = vertex_factor * triangle_factor * texture_factor * shader_factor * 
                      particle_factor * physics_factor * quality_factor * resolution_factor * 
                      fps_factor * rt_factor * upscale_factor * engine_factor;
    
    u64 estimated_score = static_cast<u64>(base_score * total_factor);
    
    // Apply reasonable bounds
    return std::clamp(estimated_score, u64(500), u64(50000));
}

u64 RequirementCalculator::estimate_vram_requirement(const AggregatedAssets& assets, RequirementLevel level, 
                                                    const GameProfile& profile) {
    // Base VRAM calculation from textures and framebuffer
    u64 texture_vram = assets.textures.total_vram;
    u64 model_vram = assets.models.total_vram; // Usually small but not zero
    
    // Framebuffer calculation
    u32 width = 1920, height = 1080;
    switch (profile.resolution_target) {
        case GameProfile::ResolutionTarget::RES_720p: width = 1280; height = 720; break;
        case GameProfile::ResolutionTarget::RES_1080p: width = 1920; height = 1080; break;
        case GameProfile::ResolutionTarget::RES_1440p: width = 2560; height = 1440; break;
        case GameProfile::ResolutionTarget::RES_4K: width = 3840; height = 2160; break;
        case GameProfile::ResolutionTarget::RES_8K: width = 7680; height = 4320; break;
        case GameProfile::ResolutionTarget::RES_VR: width = 2160; height = 2160; break; // Per eye for VR
    }
    
    // Color buffer (RGBA8 = 4 bytes per pixel)
    u64 color_buffer = static_cast<u64>(width) * height * 4;
    // Depth buffer (D24S8 = 4 bytes per pixel)
    u64 depth_buffer = static_cast<u64>(width) * height * 4;
    
    // MSAA multiplier
    u32 msaa_samples = 1;
    if (profile.graphics_quality >= GameProfile::GraphicsQuality::QUALITY_HIGH) {
        msaa_samples = 4;
    } else if (profile.graphics_quality >= GameProfile::GraphicsQuality::QUALITY_ULTRA) {
        msaa_samples = 8;
    }
    
    u64 framebuffer_size = (color_buffer + depth_buffer) * msaa_samples * 2; // *2 for double buffering
    
    // Texture quality multiplier
    f32 texture_quality_multiplier = 1.0f;
    switch (profile.graphics_quality) {
        case GameProfile::GraphicsQuality::QUALITY_LOW: texture_quality_multiplier = 0.5f; break;
        case GameProfile::GraphicsQuality::QUALITY_MEDIUM: texture_quality_multiplier = 0.75f; break;
        case GameProfile::GraphicsQuality::QUALITY_HIGH: texture_quality_multiplier = 1.0f; break;
        case GameProfile::GraphicsQuality::QUALITY_ULTRA: texture_quality_multiplier = 1.5f; break;
        case GameProfile::GraphicsQuality::QUALITY_EXTREME: texture_quality_multiplier = 2.0f; break;
        case GameProfile::GraphicsQuality::QUALITY_RAY_TRACING: texture_quality_multiplier = 2.0f; break;
    }
    
    // Calculate total VRAM needed
    u64 base_vram = (texture_vram * texture_quality_multiplier) + model_vram + framebuffer_size;
    
    // Apply resolution and FPS scaling
    f32 resolution_factor = RESOLUTION_FACTORS[static_cast<size_t>(profile.resolution_target)];
    f32 fps_factor = FPS_TARGETS[static_cast<size_t>(profile.fps_target)] / 60.0f;
    
    // Additional factors for ray tracing and upscaling
    f32 rt_factor = 1.0f;
    if (profile.ray_tracing_enabled) {
        rt_factor = 2.0f; // RT often requires additional buffers
    }
    
    f32 upscale_factor = 1.0f;
    if (profile.dlss_enabled || profile.fsr_enabled) {
        upscale_factor = 0.6f; // Upscaling reduces effective resolution needed
    }
    
    u64 total_vram = static_cast<u64>(static_cast<f32>(base_vram) * 
                                     resolution_factor * fps_factor * rt_factor * upscale_factor);
    
    // Apply reasonable bounds (1GB to 32GB)
    return std::clamp(total_vram / MiB, u64(1024), u64(32768));
}

u64 RequirementCalculator::estimate_ram_requirement(const AggregatedAssets& assets, RequirementLevel level, 
                                                   const GameProfile& profile) {
    // Base RAM calculation
    u64 base_ram = 4 * GiB; // OS baseline
    
    // Asset-based RAM estimation
    u64 asset_ram = 0;
    
    // Textures: typically compressed in VRAM but uncompressed in RAM during loading
    asset_ram += assets.textures.total_disk_size * 0.4f; // Estimated uncompressed size
    
    // Models: vertex data, indices, bone weights, etc.
    asset_ram += assets.models.total_disk_size * 0.3f;
    
    // Audio: decompressed size during playback
    asset_ram += assets.audio.total_disk_size * 0.5f; // Assuming compressed audio
    
    // Scripts: loaded into memory for execution
    asset_ram += assets.scripts.total_ram_estimate;
    
    // Executables and libraries
    asset_ram += assets.executables.total_estimated_ram;
    
    // Game runtime overhead
    u64 game_runtime = static_cast<u64>(asset_ram * 0.5f); // Game engine overhead
    
    // OS overhead (scales with total system load)
    u64 os_overhead = static_cast<u64>((base_ram + asset_ram + game_runtime) * 0.25f);
    
    // Quality and resolution multipliers
    f32 quality_factor = QUALITY_FACTORS[static_cast<size_t>(profile.graphics_quality)];
    f32 resolution_factor = RESOLUTION_FACTORS[static_cast<size_t>(profile.resolution_target)];
    f32 fps_factor = FPS_TARGETS[static_cast<size_t>(profile.fps_target)] / 60.0f;
    
    // Calculate total RAM
    u64 total_ram = base_ram + asset_ram + game_runtime + os_overhead;
    total_ram = static_cast<u64>(static_cast<f32>(total_ram) * 
                                quality_factor * resolution_factor * fps_factor);
    
    // Apply reasonable bounds (4GB to 128GB)
    return std::clamp(total_ram / GiB, u64(4), u64(128));
}

u64 RequirementCalculator::estimate_storage_requirement(const AggregatedAssets& assets, RequirementLevel level, 
                                                     const GameProfile& profile) {
    // Base storage requirement
    u64 base_storage = assets.total_disk_size;
    
    // Additional space needed
    u64 shader_cache = 2 * GiB; // Shader cache
    u64 save_files = 1 * GiB;   // Save games, configs, etc.
    u64 temp_files = base_storage * 0.15f; // Temporary files, swap, etc.
    u64 os_overhead = 25 * GiB; // OS, page file, hibernation, etc.
    
    // Quality and resolution factors (affects install size and cache)
    f32 quality_factor = 1.0f + (static_cast<f32>(profile.graphics_quality) * 0.2f); // Higher quality = larger assets
    f32 resolution_factor = 1.0f + (static_cast<f32>(profile.resolution_target) * 0.15f); // Higher res = larger textures
    
    // Calculate total storage
    u64 total_storage = static_cast<u64>((base_storage + shader_cache + save_files + temp_files) * 
                                        quality_factor * resolution_factor) + os_overhead;
    
    // Apply reasonable bounds (20GB to 2TB)
    return std::clamp(total_storage / GiB, u64(20), u64(2048));
}

void RequirementCalculator::calculate_cpu_req(const AggregatedAssets& assets, HardwareRequirement& req, 
                                             RequirementLevel level, const GameProfile& profile) const {
    u64 passmark_mt = estimate_cpu_requirement(assets, level, profile);
    u64 passmark_st = static_cast<u64>(static_cast<f32>(passmark_mt) * 0.35f); // Typical single-thread to multi-thread ratio
    
    req.cpu_passmark_score = static_cast<u32>(passmark_st);
    req.cpu_passmark_mt_score = static_cast<u32>(passmark_mt);
    
    // Determine core/thread count based on performance level and needs
    if (level <= RequirementLevel::Recommended) {
        req.cpu_cores = std::max(4u, static_cast<u32>(std::ceil(passmark_mt / 5000.0)));
        req.cpu_threads = req.cpu_cores * 2; // Assume SMT
    } else if (level == RequirementLevel::High) {
        req.cpu_cores = std::max(6u, static_cast<u32>(std::ceil(passmark_mt / 4000.0)));
        req.cpu_threads = req.cpu_cores * 2;
    } else if (level == RequirementLevel::Ultra) {
        req.cpu_cores = std::max(8u, static_cast<u32>(std::ceil(passmark_mt / 3500.0)));
        req.cpu_threads = req.cpu_cores * 2;
    } else { // RayTracing
        req.cpu_cores = std::max(8u, static_cast<u32>(std::ceil(passmark_mt / 3000.0)));
        req.cpu_threads = req.cpu_cores * 2;
    }
    
    // Clock speeds - estimate based on performance level
    req.cpu_base_clock = 2.5f + (static_cast<f32>(level) * 0.5f); // 2.5GHz to 4.5GHz
    req.cpu_boost_clock = req.cpu_base_clock + 1.0f + (static_cast<f32>(level) * 0.3f);
    
    // Cache size - scales with core count and performance
    req.cpu_l3_cache = static_cast<u64>(req.cpu_cores) * 2 * 1024; // 2MB per core
    
    // Instruction set - modern CPUs
    req.cpu_instruction_set = "SSE4.2, AVX2";
    if (level >= RequirementLevel::High) {
        req.cpu_instruction_set += ", AVX-512";
    }
    req.cpu_arch = "x86_64";
    
    req.cpu_arch = "x86_64";
}

void RequirementCalculator::calculate_gpu_req(const AggregatedAssets& assets, HardwareRequirement& req, 
                                             RequirementLevel level, const GameProfile& profile) const {
    u64 g3d_score = estimate_gpu_requirement(assets, level, profile);
    u64 vram_mb = estimate_vram_requirement(assets, level, profile);
    
    req.gpu_passmark_score = static_cast<u32>(g3d_score);
    req.gpu_vram = vram_mb;
    
    // Determine GPU specs based on performance level
    if (level <= RequirementLevel::Minimum) {
        req.gpu_compute_units = std::max(10u, static_cast<u32>(g3d_score / 100));
        req.gpu_base_clock = 1200.0f;
        req.gpu_boost_clock = 1500.0f;
    } else if (level == RequirementLevel::Recommended) {
        req.gpu_compute_units = std::max(20u, static_cast<u32>(g3d_score / 80));
        req.gpu_base_clock = 1400.0f;
        req.gpu_boost_clock = 1700.0f;
    } else if (level == RequirementLevel::High) {
        req.gpu_compute_units = std::max(30u, static_cast<u32>(g3d_score / 60));
        req.gpu_base_clock = 1600.0f;
        req.gpu_boost_clock = 1900.0f;
    } else if (level == RequirementLevel::Ultra) {
        req.gpu_compute_units = std::max(40u, static_cast<u32>(g3d_score / 50));
        req.gpu_base_clock = 1800.0f;
        req.gpu_boost_clock = 2100.0f;
    } else { // RayTracing
        req.gpu_compute_units = std::max(50u, static_cast<u32>(g3d_score / 40));
        req.gpu_base_clock = 2000.0f;
        req.gpu_boost_clock = 2300.0f;
    }
    
    // Derive other specs from compute units (simplified)
    req.gpu_cuda_cores = req.gpu_compute_units * 64; // NVIDIA-style
    req.gpu_tensor_cores = std::max(0u, static_cast<u32>(req.gpu_compute_units * 0.25));
    req.gpu_rt_cores = std::max(0u, static_cast<u32>(req.gpu_compute_units * 0.15));
    
    // Memory specs
    if (vram_mb >= 8192) {
        req.gpu_vram_type = "GDDR6";
        req.gpu_vram_bus_width = 256;
        req.gpu_vram_bandwidth = 448.0f; // GB/s
    } else if (vram_mb >= 4096) {
        req.gpu_vram_type = "GDDR6";
        req.gpu_vram_bus_width = 128;
        req.gpu_vram_bandwidth = 224.0f; // GB/s
    } else {
        req.gpu_vram_type = "GDDR6";
        req.gpu_vram_bus_width = 64;
        req.gpu_vram_bandwidth = 112.0f; // GB/s
    }
    
    // Clock speeds
    req.gpu_base_clock = 1200.0f + (static_cast<f32>(level) * 200.0f);
    req.gpu_boost_clock = req.gpu_base_clock + 300.0f + (static_cast<f32>(level) * 100.0f);
    
    // Power and process node (estimate)
    req.gpu_tdp = 75 + (static_cast<u32>(level) * 25); // 75W to 175W
    req.gpu_process_node = "8nm"; // Would be more sophisticated in reality
    
    // Feature flags based on level
    req.gpu_ray_tracing = (level >= RequirementLevel::Ultra);
    req.gpu_dlss_support = (level >= RequirementLevel::Recommended);
    req.gpu_fsr_support = true; // AMD FSR is widely supported
    req.gpu_xess_support = (level >= RequirementLevel::High);
    
    // API versions
    req.gpu_dx_version = "12.2";
    req.gpu_vk_version = "1.3";
    req.gpu_metal_version = "3.1";
    
    // Vendor and architecture (simplified)
    req.gpu_vendor = "NVIDIA/AMD";
    if (level >= RequirementLevel::High) {
        req.gpu_architecture = "Ada Lovelace/RDNA 3";
    } else {
        req.gpu_architecture = "Ampere/RDNA 2";
    }
    
    req.gpu_arch = "PCIe 4.0 x16";
}

void RequirementCalculator::calculate_ram_req(const AggregatedAssets& assets, HardwareRequirement& req, 
                                             RequirementLevel level, const GameProfile& profile) const {
    u64 ram_gb = estimate_ram_requirement(assets, level, profile);
    
    req.ram_capacity = ram_gb * 1024; // Convert to MB
    
    // Determine RAM type and speed based on capacity and level
    if (ram_gb >= 32) {
        req.ram_type = "DDR5";
        req.ram_speed = 5200.0f + (static_cast<f32>(level) * 400.0f); // 5200-7200 MT/s
    } else if (ram_gb >= 16) {
        req.ram_type = (level >= RequirementLevel::High) ? "DDR5" : "DDR4";
        if (req.ram_type == "DDR5") {
            req.ram_speed = 4800.0f + (static_cast<f32>(level) * 300.0f);
        } else {
            req.ram_speed = 3200.0f + (static_cast<f32>(level) * 200.0f);
        }
    } else {
        req.ram_type = "DDR4";
        req.ram_speed = 2666.0f + (static_cast<f32>(level) * 200.0f); // 2666-3466 MT/s
    }
    
    // Channel configuration
    if (ram_gb >= 64) {
        req.ram_channels = 4;
    } else if (ram_gb >= 32) {
        req.ram_channels = (ram_gb % 16 == 0) ? 4 : 2;
    } else {
        req.ram_channels = 2; // Dual channel is standard
    }
    
    req.ram_ecc = false; // Consumer-grade assumption
}

void RequirementCalculator::calculate_storage_req(const AggregatedAssets& assets, HardwareRequirement& req, 
                                                 RequirementLevel level, const GameProfile& profile) const {
    u64 storage_gb = estimate_storage_requirement(assets, level, profile);
    
    req.storage_capacity = storage_gb;
    
    // Determine storage type based on capacity and performance needs
    if (storage_gb >= 2000) {
        // Large capacity - could be HDD for bulk storage, but we'll recommend NVMe for performance
        req.storage_type = "NVMe";
    } else if (storage_gb >= 500) {
        req.storage_type = (level >= RequirementLevel::Recommended) ? "NVMe" : "SATA SSD";
    } else {
        req.storage_type = (level >= RequirementLevel::High) ? "NVMe" : "SATA SSD";
    }
    
    // Speed estimates based on type
    if (req.storage_type == "NVMe") {
        if (storage_gb >= 1000) {
            req.storage_read_speed = 5000.0f + (static_cast<f32>(level) * 1000.0f); // 5000-10000 MB/s
            req.storage_write_speed = 4000.0f + (static_cast<f32>(level) * 800.0f);  // 4000-8000 MB/s
        } else {
            req.storage_read_speed = 3000.0f + (static_cast<f32>(level) * 500.0f);  // 3000-5000 MB/s
            req.storage_write_speed = 2000.0f + (static_cast<f32>(level) * 400.0f);  // 2000-4000 MB/s
        }
        req.storage_direct_storage = (level >= RequirementLevel::High);
    } else if (req.storage_type == "SATA SSD") {
        req.storage_read_speed = 500.0f + (static_cast<f32>(level) * 100.0f); // 500-900 MB/s
        req.storage_write_speed = 450.0f + (static_cast<f32>(level) * 80.0f);   // 450-770 MB/s
        req.storage_direct_storage = false;
    } else { // HDD fallback
        req.storage_read_speed = 150.0f + (static_cast<f32>(level) * 50.0f);   // 150-250 MB/s
        req.storage_write_speed = 120.0f + (static_cast<f32>(level) * 40.0f);   // 120-160 MB/s
        req.storage_direct_storage = false;
    }
}

void RequirementCalculator::calculate_os_req(const AggregatedAssets& assets, HardwareRequirement& req) const {
    // Basic OS requirements - would be more sophisticated in reality
    req.os_minimum = "Windows 10 64-bit (1909+) / Ubuntu 20.04+ / macOS 10.15+";
    req.os_recommended = "Windows 11 64-bit (21H2+) / Ubuntu 22.04+ / macOS 12+";
    req.os_version = "Latest stable";
}

void RequirementCalculator::calculate_display_req(const AggregatedAssets& assets, HardwareRequirement& req, 
                                         RequirementLevel level, const GameProfile& profile) const {
    // This would be implemented if we had a separate display requirements calculation
    // For now, we'll use sensible defaults based on the level
    switch (level) {
        case RequirementLevel::Minimum:
            req.min_resolution_w = 1280;
            req.min_resolution_h = 720;
            req.rec_resolution_w = 1280;
            req.rec_resolution_h = 720;
            req.min_refresh_rate = 30;
            req.rec_refresh_rate = 30;
            break;
        case RequirementLevel::Recommended:
            req.min_resolution_w = 1920;
            req.min_resolution_h = 1080;
            req.rec_resolution_w = 1920;
            req.rec_resolution_h = 1080;
            req.min_refresh_rate = 60;
            req.rec_refresh_rate = 60;
            break;
        case RequirementLevel::High:
            req.min_resolution_w = 1920;
            req.min_resolution_h = 1080;
            req.rec_resolution_w = 2560;
            req.rec_resolution_h = 1440;
            req.min_refresh_rate = 60;
            req.rec_refresh_rate = 144;
            break;
        case RequirementLevel::Ultra:
            req.min_resolution_w = 2560;
            req.min_resolution_h = 1440;
            req.rec_resolution_w = 3840;
            req.rec_resolution_h = 2160;
            req.min_refresh_rate = 100;
            req.rec_refresh_rate = 165;
            break;
        case RequirementLevel::RayTracing:
            req.min_resolution_w = 3840;
            req.min_resolution_h = 2160;
            req.rec_resolution_w = 3840;
            req.rec_resolution_h = 2160;
            req.min_refresh_rate = 60;
            req.rec_refresh_rate = 120;
            break;
    }
    
    // HDR and VRR support based on level
    req.hdr_support = (level >= RequirementLevel::High);
    req.vrr_support = (level >= RequirementLevel::High);
}

void RequirementCalculator::calculate_api_req(const AggregatedAssets& assets, HardwareRequirement& req) const {
    // API versions - would be determined by actual engine requirements in reality
    req.dx_version = "12.2";
    req.vk_version = "1.3";
    req.metal_version = "3.1";
}

void RequirementCalculator::find_matching_hardware(HardwareRequirement& req, RequirementLevel level) const {
    // Clear previous selections
    req.specific_cpus.clear();
    req.specific_gpus.clear();
    
    // Find matching CPUs
    auto cpus = hw_db_.find_cpus_by_score(req.cpu_passmark_mt_score, 5);
    for (const auto* cpu : cpus) {
        req.specific_cpus.push_back(cpu->name);
    }
    
    // Find matching GPUs
    auto gpus = hw_db_.find_gpus_by_score(req.gpu_passmark_score, 5);
    for (const auto* gpu : gpus) {
        req.specific_gpus.push_back(gpu->name);
    }
    
    // Fallbacks if no matches found
    if (req.specific_cpus.empty()) {
        // Provide reasonable defaults based on level
        switch (level) {
            case RequirementLevel::Minimum:
                req.specific_cpus = {"Intel Core i3-12100F", "AMD Ryzen 3 4100", "Intel Pentium Gold G7400"};
                break;
            case RequirementLevel::Recommended:
                req.specific_cpus = {"Intel Core i5-12400F", "AMD Ryzen 5 5600", "Intel Core i5-11400"};
                break;
            case RequirementLevel::High:
                req.specific_cpus = {"Intel Core i7-12700K", "AMD Ryzen 7 5800X", "Intel Core i7-11700K"};
                break;
            case RequirementLevel::Ultra:
                req.specific_cpus = {"Intel Core i9-12900K", "AMD Ryzen 9 5900X", "Intel Core i9-11900K"};
                break;
            case RequirementLevel::RayTracing:
                req.specific_cpus = {"Intel Core i9-13900K", "AMD Ryzen 9 7950X", "Intel Core i9-12900KS"};
                break;
        }
    }
    
    if (req.specific_gpus.empty()) {
        // Provide reasonable defaults based on level
        switch (level) {
            case RequirementLevel::Minimum:
                req.specific_gpus = {"NVIDIA GT 1030", "AMD RX 550", "Intel UHD Graphics 770"};
                break;
            case RequirementLevel::Recommended:
                req.specific_gpus = {"NVIDIA GTX 1650", "AMD RX 6400", "Intel Arc A380"};
                break;
            case RequirementLevel::High:
                req.specific_gpus = {"NVIDIA RTX 3060", "AMD RX 6600 XT", "Intel Arc A750"};
                break;
            case RequirementLevel::Ultra:
                req.specific_gpus = {"NVIDIA RTX 3080", "AMD RX 6800 XT", "Intel Arc A770"};
                break;
            case RequirementLevel::RayTracing:
                req.specific_gpus = {"NVIDIA RTX 4080", "AMD RX 7900 XTX", "NVIDIA RTX 3090 Ti"};
                break;
        }
    }
}

f32 RequirementCalculator::calculate_confidence(const AggregatedAssets& assets) const {
    f32 confidence = 0.0f;
    u32 factors = 0;
    
    // Executables give us direct evidence of code complexity
    if (assets.executables.total_executables > 0) {
        confidence += 0.20f;
        factors++;
    }
    
    #if 0
    // Textures give us evidence of graphical complexity
    if (assets.textures.total_textures > 5) {
        confidence += 0.15f;
        factors++;
    }
    #endif
    
    // Models give us evidence of geometric complexity
    if (assets.models.total_models > 0) {
        confidence += 0.15f;
        factors++;
    }
    
    // Audio gives us evidence of audio complexity
    if (assets.audio.total_files > 0) {
        confidence += 0.10f;
        factors++;
    }
    
    // Scripts give us evidence of gameplay/logic complexity
    if (assets.scripts.total_scripts > 0) {
        confidence += 0.15f;
        factors++;
    }
    
    // Total size gives us evidence of overall scale
    if (assets.total_disk_size > 500 * MiB) { // More than 500MB
        confidence += 0.10f;
        factors++;
    }
    
    // Engine detection gives us specific knowledge
    if (!assets.primary_engine.empty()) {
        confidence += 0.15f;
        factors++;
    }
    
    // Avoid division by zero
    if (factors == 0) {
        return 0.1f; // Minimum confidence
    }
    
    return std::min(confidence, 1.0f);
}

f32 RequirementCalculator::calculate_bottleneck_factor(const AggregatedAssets& assets) const {
    // Simple bottleneck detection based on relative resource usage
    // Ratio of GPU demand to CPU demand (simplified)
    
    f32 cpu_demand = 0.0f;
    f32 gpu_demand = 0.0f;
    
    // CPU demand factors
    if (assets.scripts.total_complexity > 0) {
        cpu_demand += std::log10f(1.0f + assets.scripts.total_complexity / 1000.0f);
    }
    if (assets.executables.total_code_size > 0) {
        cpu_demand += std::log10f(1.0f + assets.executables.total_code_size / (1024*1024)); // MB
    }
    if (assets.models.total_bones > 0) {
        cpu_demand += std::log10f(1.0f + assets.models.total_bones / 100.0f);
    }
    
    // GPU demand factors
    if (assets.models.total_vertices > 0) {
        gpu_demand += std::log10f(1.0f + assets.models.total_vertices / (100*1000)); // K vertices
    }
    if (assets.models.total_triangles > 0) {
        gpu_demand += std::log10f(1.0f + assets.models.total_triangles / (200*1000)); // K triangles
    }
    if (assets.textures.total_vram > 0) {
        gpu_demand += std::log10f(1.0f + assets.textures.total_vram / (256*1024)); // MB
    }
    if (assets.textures.total_textures > 0) {
        gpu_demand += std::log10f(1.0f + assets.textures.total_textures / 100.0f);
    }
    
    // Avoid division by zero
    if (cpu_demand < 0.001f) cpu_demand = 0.001f;
    if (gpu_demand < 0.001f) gpu_demand = 0.001f;
    
    // Return ratio: >1.0 means GPU-bound, <1.0 means CPU-bound
    return gpu_demand / cpu_demand;
}

} // namespace game_req