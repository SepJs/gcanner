#include <game_req_analyzer/analyzers/requirement_calculator.hpp>
#include <game_req_analyzer/hardware/hardware_database.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <algorithm>
#include <cmath>
#include <numeric>

using namespace game_req;

RequirementCalculator::RequirementCalculator(const AnalyzerConfig& config, const HardwareDatabase& hw_db)
    : config_(config), hw_db_(hw_db) {}

RequirementCalculator::~RequirementCalculator() = default;

Result<RequirementResult> RequirementCalculator::calculate(const AggregatedAssets& assets) {
    RequirementResult result;
    result.confidence = 0.0f;
    result.analysis_notes.clear();
    result.warnings.clear();
    result.assumptions.clear();
    
    result.assumptions.push_back("Analysis based on static asset inspection only");
    result.assumptions.push_back("Runtime memory usage estimated from asset data");
    result.assumptions.push_back("Shader complexity inferred from file count and size");
    
    calculate_cpu_req(assets, result.minimum, RequirementLevel::Minimum);
    calculate_cpu_req(assets, result.recommended, RequirementLevel::Recommended);
    calculate_cpu_req(assets, result.high, RequirementLevel::High);
    calculate_cpu_req(assets, result.ultra, RequirementLevel::Ultra);
    calculate_cpu_req(assets, *result.ray_tracing.emplace(), RequirementLevel::RayTracing);
    
    calculate_gpu_req(assets, result.minimum, RequirementLevel::Minimum);
    calculate_gpu_req(assets, result.recommended, RequirementLevel::Recommended);
    calculate_gpu_req(assets, result.high, RequirementLevel::High);
    calculate_gpu_req(assets, result.ultra, RequirementLevel::Ultra);
    calculate_gpu_req(assets, *result.ray_tracing, RequirementLevel::RayTracing);
    
    calculate_ram_req(assets, result.minimum, RequirementLevel::Minimum);
    calculate_ram_req(assets, result.recommended, RequirementLevel::Recommended);
    calculate_ram_req(assets, result.high, RequirementLevel::High);
    calculate_ram_req(assets, result.ultra, RequirementLevel::Ultra);
    calculate_ram_req(assets, *result.ray_tracing, RequirementLevel::RayTracing);
    
    calculate_storage_req(assets, result.minimum, RequirementLevel::Minimum);
    calculate_storage_req(assets, result.recommended, RequirementLevel::Recommended);
    calculate_storage_req(assets, result.high, RequirementLevel::High);
    calculate_storage_req(assets, result.ultra, RequirementLevel::Ultra);
    calculate_storage_req(assets, *result.ray_tracing, RequirementLevel::RayTracing);
    
    calculate_os_req(assets, result.minimum);
    result.recommended.os_minimum = result.minimum.os_minimum;
    result.recommended.os_recommended = result.minimum.os_recommended;
    result.high.os_minimum = result.minimum.os_minimum;
    result.high.os_recommended = result.minimum.os_recommended;
    result.ultra.os_minimum = result.minimum.os_minimum;
    result.ultra.os_recommended = result.minimum.os_recommended;
    result.ray_tracing->os_minimum = result.minimum.os_minimum;
    result.ray_tracing->os_recommended = result.minimum.os_recommended;
    
    calculate_display_req(assets, result.minimum, RequirementLevel::Minimum);
    calculate_display_req(assets, result.recommended, RequirementLevel::Recommended);
    calculate_display_req(assets, result.high, RequirementLevel::High);
    calculate_display_req(assets, result.ultra, RequirementLevel::Ultra);
    calculate_display_req(assets, *result.ray_tracing, RequirementLevel::RayTracing);
    
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
    
    find_matching_hardware(result.minimum, RequirementLevel::Minimum);
    find_matching_hardware(result.recommended, RequirementLevel::Recommended);
    find_matching_hardware(result.high, RequirementLevel::High);
    find_matching_hardware(result.ultra, RequirementLevel::Ultra);
    find_matching_hardware(*result.ray_tracing, RequirementLevel::RayTracing);
    
    result.confidence = calculate_confidence(assets);
    
    if (assets.executables.total_code_size == 0) {
        result.warnings.push_back("No executables found - CPU requirements estimated from assets only");
    }
    if (assets.textures.total_vram == 0) {
        result.warnings.push_back("No textures found - VRAM requirements may be inaccurate");
    }
    if (assets.models.total_vertices == 0) {
        result.warnings.push_back("No 3D models found - GPU compute requirements estimated from other factors");
    }
    
    return result;
}

u64 RequirementCalculator::estimate_cpu_requirement(const AggregatedAssets& assets, RequirementLevel level) {
    f32 quality_factor = QUALITY_FACTORS[static_cast<u8>(level)];
    
    u64 base_score = 2000;
    
    f32 script_factor = 1.0f;
    if (assets.scripts.total_complexity > 0) {
        script_factor = 1.0f + std::log10f(1.0f + assets.scripts.total_complexity / 10000.0f) * 0.3f;
    }
    
    f32 physics_factor = 1.0f;
    if (assets.models.total_bones > 0) {
        physics_factor = 1.0f + std::log10f(1.0f + assets.models.total_bones / 1000.0f) * 0.2f;
    }
    
    f32 ai_factor = 1.0f;
    if (assets.scripts.total_functions > 0) {
        ai_factor = 1.0f + std::log10f(1.0f + assets.scripts.total_functions / 5000.0f) * 0.15f;
    }
    
    f32 executable_factor = 1.0f;
    if (assets.executables.total_code_size > 0) {
        executable_factor = 1.0f + std::log10f(1.0f + assets.executables.total_code_size / (10.0f * MiB)) * 0.25f;
    }
    
    f32 total_factor = script_factor * physics_factor * ai_factor * executable_factor * quality_factor;
    u64 estimated_score = static_cast<u64>(base_score * total_factor);
    
    return std::clamp(estimated_score, u64(1000), u64(50000));
}

u64 RequirementCalculator::estimate_gpu_requirement(const AggregatedAssets& assets, RequirementLevel level) {
    f32 quality_factor = QUALITY_FACTORS[static_cast<u8>(level)];
    
    u64 base_score = 1500;
    
    f32 vertex_factor = 1.0f;
    if (assets.models.total_vertices > 0) {
        vertex_factor = 1.0f + std::log10f(1.0f + assets.models.total_vertices / 1000000.0f) * 0.4f;
    }
    
    f32 triangle_factor = 1.0f;
    if (assets.models.total_triangles > 0) {
        triangle_factor = 1.0f + std::log10f(1.0f + assets.models.total_triangles / 2000000.0f) * 0.35f;
    }
    
    f32 shader_factor = 1.0f;
    if (assets.executables.total_code_size > 0 || assets.scripts.total_functions > 0) {
        shader_factor = 1.0f + 0.2f;
    }
    
    f32 particle_factor = 1.0f;
    f32 total_factor = vertex_factor * triangle_factor * shader_factor * particle_factor * quality_factor;
    u64 estimated_score = static_cast<u64>(base_score * total_factor);
    
    return std::clamp(estimated_score, u64(800), u64(60000));
}

u64 RequirementCalculator::estimate_vram_requirement(const AggregatedAssets& assets, RequirementLevel level) {
    f32 quality_factor = QUALITY_FACTORS[static_cast<u8>(level)];
    
    u64 texture_vram = assets.textures.total_vram;
    u64 model_vram = assets.models.total_vram;
    u64 framebuffer_vram = 0;
    
    u32 width = 1920, height = 1080;
    switch (level) {
        case RequirementLevel::Minimum: width = 1920; height = 1080; break;
        case RequirementLevel::Recommended: width = 2560; height = 1440; break;
        case RequirementLevel::High: width = 3440; height = 1440; break;
        case RequirementLevel::Ultra: width = 3840; height = 2160; break;
        case RequirementLevel::RayTracing: width = 3840; height = 2160; break;
    }
    
    u64 color_buffer = static_cast<u64>(width) * height * 16;
    u64 depth_buffer = static_cast<u64>(width) * height * 4;
    u64 msaa_factor = (level >= RequirementLevel::High) ? 4 : 1;
    framebuffer_vram = (color_buffer + depth_buffer) * msaa_factor * 2;
    
    u64 total_vram = (texture_vram + model_vram) * quality_factor + framebuffer_vram;
    total_vram = static_cast<u64>(total_vram * 1.3f);
    
    return std::clamp(total_vram / MiB, u64(1024), u64(32768));
}

u64 RequirementCalculator::estimate_ram_requirement(const AggregatedAssets& assets, RequirementLevel level) {
    f32 quality_factor = QUALITY_FACTORS[static_cast<u8>(level)];
    
    u64 base_ram = 4 * GiB;
    
    u64 asset_ram = assets.textures.total_disk_size * 0.3f;
    asset_ram += assets.models.total_disk_size * 0.2f;
    asset_ram += assets.audio.total_disk_size * 0.1f;
    asset_ram += assets.scripts.total_ram_estimate;
    asset_ram += assets.executables.total_estimated_ram;
    
    u64 os_ram = 2 * GiB;
    u64 game_ram = static_cast<u64>(asset_ram * quality_factor * 1.5f);
    
    u64 total_ram = base_ram + os_ram + game_ram;
    
    return std::clamp(total_ram / GiB, u64(4), u64(128));
}

u64 RequirementCalculator::estimate_storage_requirement(const AggregatedAssets& assets, RequirementLevel level) {
    f32 quality_factor = QUALITY_FACTORS[static_cast<u8>(level)];
    
    u64 base_storage = assets.total_disk_size;
    
    u64 shader_cache = 2 * GiB;
    u64 save_files = 512 * MiB;
    u64 temp_files = base_storage * 0.1f;
    u64 os_overhead = 20 * GiB;
    
    u64 total_storage = static_cast<u64>((base_storage + shader_cache + save_files + temp_files) * quality_factor) + os_overhead;
    
    return std::clamp(total_storage / GiB, u64(20), u64(500));
}

void RequirementCalculator::calculate_cpu_req(const AggregatedAssets& assets, HardwareRequirement& req, RequirementLevel level) const {
    u64 passmark_mt = estimate_cpu_requirement(assets, level);
    u64 passmark_st = static_cast<u64>(passmark_mt * 0.35f);
    
    req.cpu_passmark_score = static_cast<u32>(passmark_st);
    req.cpu_passmark_mt_score = static_cast<u32>(passmark_mt);
    req.cpu_cores = (level <= RequirementLevel::Recommended) ? 4 : 6;
    req.cpu_threads = req.cpu_cores * 2;
    req.cpu_base_clock = (level <= RequirementLevel::Recommended) ? 3.0f : 3.5f;
    req.cpu_boost_clock = req.cpu_base_clock + 1.0f;
    req.cpu_l3_cache = (level <= RequirementLevel::Recommended) ? 8 * 1024 : 16 * 1024;
    req.cpu_instruction_set = "AVX2, SSE4.2";
    if (level >= RequirementLevel::High) req.cpu_instruction_set += ", AVX-512";
    req.cpu_arch = "x86_64";
    
    req.specific_cpus.clear();
}

void RequirementCalculator::calculate_gpu_req(const AggregatedAssets& assets, HardwareRequirement& req, RequirementLevel level) const {
    u64 g3d_score = estimate_gpu_requirement(assets, level);
    u64 vram_mb = estimate_vram_requirement(assets, level);
    
    req.gpu_passmark_score = static_cast<u32>(g3d_score);
    req.gpu_vram = vram_mb;
    req.gpu_vram_type = (level >= RequirementLevel::High) ? "GDDR6" : "GDDR5";
    req.gpu_vram_bus_width = (vram_mb >= 8192) ? 256 : 128;
    req.gpu_vram_bandwidth = (level >= RequirementLevel::High) ? 448.0f : 224.0f;
    req.gpu_compute_units = static_cast<u32>(g3d_score / 50);
    req.gpu_base_clock = 1500.0f;
    req.gpu_boost_clock = 1800.0f;
    req.gpu_ray_tracing = (level >= RequirementLevel::Ultra);
    req.gpu_dlss_support = (level >= RequirementLevel::Recommended);
    req.gpu_fsr_support = true;
    req.gpu_xess_support = (level >= RequirementLevel::High);
    req.gpu_tensor_cores = (level >= RequirementLevel::High) ? 128 : 0;
    req.gpu_rt_cores = (level >= RequirementLevel::Ultra) ? 32 : 0;
    req.gpu_api_version = "DX12_2, VK 1.3";
    req.gpu_vendor = "NVIDIA/AMD/Intel";
    req.gpu_architecture = (level >= RequirementLevel::High) ? "Ada Lovelace/RDNA 3" : "Ampere/RDNA 2";
    
    req.specific_gpus.clear();
}

void RequirementCalculator::calculate_ram_req(const AggregatedAssets& assets, HardwareRequirement& req, RequirementLevel level) const {
    u64 ram_gb = estimate_ram_requirement(assets, level);
    
    req.ram_capacity = ram_gb * 1024;
    req.ram_type = (level >= RequirementLevel::High) ? "DDR5" : "DDR4";
    req.ram_speed = (level >= RequirementLevel::High) ? 5600.0f : 3200.0f;
    req.ram_channels = (ram_gb >= 32) ? 4 : 2;
    req.ram_ecc = false;
}

void RequirementCalculator::calculate_storage_req(const AggregatedAssets& assets, HardwareRequirement& req, RequirementLevel level) const {
    u64 storage_gb = estimate_storage_requirement(assets, level);
    
    req.storage_capacity = storage_gb;
    req.storage_type = (level >= RequirementLevel::Recommended) ? "NVMe" : "SATA SSD";
    req.storage_read_speed = (level >= RequirementLevel::Recommended) ? 5000.0f : 500.0f;
    req.storage_write_speed = (level >= RequirementLevel::Recommended) ? 4000.0f : 450.0f;
    req.storage_direct_storage = (level >= RequirementLevel::High);
}

void RequirementCalculator::calculate_os_req(const AggregatedAssets& assets, HardwareRequirement& req) const {
    req.os_minimum = "Windows 10 64-bit (21H2+) / Ubuntu 20.04+ / macOS 12+";
    req.os_recommended = "Windows 11 64-bit / Ubuntu 22.04+ / macOS 13+";
    req.os_version = "Latest stable";
}

void RequirementCalculator::calculate_display_req(const AggregatedAssets& assets, HardwareRequirement& req, RequirementLevel level) const {
    switch (level) {
        case RequirementLevel::Minimum:
            req.min_resolution_w = 1920;
            req.min_resolution_h = 1080;
            req.rec_resolution_w = 1920;
            req.rec_resolution_h = 1080;
            req.min_refresh_rate = 60;
            req.rec_refresh_rate = 60;
            break;
        case RequirementLevel::Recommended:
            req.min_resolution_w = 1920;
            req.min_resolution_h = 1080;
            req.rec_resolution_w = 2560;
            req.rec_resolution_h = 1440;
            req.min_refresh_rate = 60;
            req.rec_refresh_rate = 144;
            break;
        case RequirementLevel::High:
            req.min_resolution_w = 2560;
            req.min_resolution_h = 1440;
            req.rec_resolution_w = 3440;
            req.rec_resolution_h = 1440;
            req.min_refresh_rate = 100;
            req.rec_refresh_rate = 165;
            req.hdr_support = true;
            req.vrr_support = true;
            break;
        case RequirementLevel::Ultra:
            req.min_resolution_w = 3840;
            req.min_resolution_h = 2160;
            req.rec_resolution_w = 3840;
            req.rec_resolution_h = 2160;
            req.min_refresh_rate = 120;
            req.rec_refresh_rate = 144;
            req.hdr_support = true;
            req.vrr_support = true;
            break;
        case RequirementLevel::RayTracing:
            req.min_resolution_w = 3840;
            req.min_resolution_h = 2160;
            req.rec_resolution_w = 3840;
            req.rec_resolution_h = 2160;
            req.min_refresh_rate = 60;
            req.rec_refresh_rate = 120;
            req.hdr_support = true;
            req.vrr_support = true;
            break;
    }
}

void RequirementCalculator::calculate_api_req(const AggregatedAssets& assets, HardwareRequirement& req) const {
    req.dx_version = "12.2";
    req.vk_version = "1.3";
    req.metal_version = "3.0";
}

void RequirementCalculator::find_matching_hardware(HardwareRequirement& req, RequirementLevel level) const {
    auto cpus = hw_db_.find_cpus_by_score(req.cpu_passmark_mt_score, 5);
    for (const auto* cpu : cpus) {
        req.specific_cpus.push_back(cpu->name);
    }
    
    auto gpus = hw_db_.find_gpus_by_score(req.gpu_passmark_score, 5);
    for (const auto* gpu : gpus) {
        req.specific_gpus.push_back(gpu->name);
    }
    
    if (req.specific_cpus.empty()) {
        if (level <= RequirementLevel::Recommended) {
            req.specific_cpus = {"Intel Core i5-12400F", "AMD Ryzen 5 5600X", "Intel Core i5-11400F"};
        } else if (level <= RequirementLevel::High) {
            req.specific_cpus = {"Intel Core i7-12700K", "AMD Ryzen 7 7700X", "Intel Core i5-13600K"};
        } else {
            req.specific_cpus = {"Intel Core i9-13900K", "AMD Ryzen 9 7950X", "Intel Core i7-14700K"};
        }
    }
    
    if (req.specific_gpus.empty()) {
        if (level == RequirementLevel::Minimum) {
            req.specific_gpus = {"NVIDIA GTX 1660 Super", "AMD RX 6600", "Intel Arc A750"};
        } else if (level == RequirementLevel::Recommended) {
            req.specific_gpus = {"NVIDIA RTX 3060 12GB", "AMD RX 6700 XT", "NVIDIA RTX 4060"};
        } else if (level == RequirementLevel::High) {
            req.specific_gpus = {"NVIDIA RTX 4070", "AMD RX 7800 XT", "NVIDIA RTX 3080"};
        } else if (level == RequirementLevel::Ultra) {
            req.specific_gpus = {"NVIDIA RTX 4080", "AMD RX 7900 XTX", "NVIDIA RTX 3090 Ti"};
        } else {
            req.specific_gpus = {"NVIDIA RTX 4090", "NVIDIA RTX 4080", "AMD RX 7900 XTX"};
        }
    }
}

f32 RequirementCalculator::calculate_confidence(const AggregatedAssets& assets) const {
    f32 confidence = 0.0f;
    u32 factors = 0;
    
    if (assets.executables.total_executables > 0) {
        confidence += 0.25f;
        factors++;
    }
    
    if (assets.textures.total_textures > 10) {
        confidence += 0.2f;
        factors++;
    }
    
    if (assets.models.total_models > 5) {
        confidence += 0.2f;
        factors++;
    }
    
    if (assets.scripts.total_scripts > 20) {
        confidence += 0.15f;
        factors++;
    }
    
    if (assets.audio.total_files > 10) {
        confidence += 0.1f;
        factors++;
    }
    
    if (assets.total_disk_size > 1 * GiB) {
        confidence += 0.1f;
        factors++;
    }
    
    return std::clamp(confidence, 0.1f, 1.0f);
}
