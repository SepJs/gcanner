#pragma once
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/core/error.hpp>

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/analyzers/asset_aggregator.hpp>
#include <game_req_analyzer/hardware/hardware_database.hpp>

namespace game_req {

enum class RequirementLevel : u8 {
    Minimum = 0,
    Recommended = 1,
    High = 2,
    Ultra = 3,
    RayTracing = 4
};

struct HardwareRequirement {
    // CPU
    String cpu_arch;          // x86_64, arm64, etc.
    u32 cpu_cores = 0;
    u32 cpu_threads = 0;
    f32 cpu_base_clock = 0.0f;    // GHz
    f32 cpu_boost_clock = 0.0f;   // GHz
    u64 cpu_l3_cache = 0;         // KB
    String cpu_instruction_set;   // AVX2, AVX-512, NEON, etc.
    u32 cpu_passmark_score = 0;   // Single thread
    u32 cpu_passmark_mt_score = 0; // Multi thread
    
    // GPU
    String gpu_vendor;        // NVIDIA, AMD, Intel
    String gpu_architecture;  // Ampere, RDNA2, Xe, etc.
    u64 gpu_vram = 0;         // MB
    u32 gpu_compute_units = 0;
    f32 gpu_base_clock = 0.0f;    // MHz
    f32 gpu_boost_clock = 0.0f;   // MHz
    u32 gpu_passmark_score = 0;   // G3D Mark
    u32 gpu_tensor_cores = 0;
    u32 gpu_rt_cores = 0;
    bool gpu_dlss_support = false;
    bool gpu_fsr_support = false;
    bool gpu_xess_support = false;
    bool gpu_ray_tracing = false;
    String gpu_api_version;   // DX12_2, VK 1.3, etc.
    
    // RAM
    u64 ram_capacity = 0;         // MB
    String ram_type;              // DDR4, DDR5, LPDDR5X
    f32 ram_speed = 0.0f;         // MT/s
    u32 ram_channels = 0;
    bool ram_ecc = false;
    
    // Storage
    u64 storage_capacity = 0;     // GB
    String storage_type;          // NVMe, SATA, HDD
    f32 storage_read_speed = 0.0f;  // MB/s
    f32 storage_write_speed = 0.0f; // MB/s
    bool storage_direct_storage = false;
    
    // OS
    String os_minimum;
    String os_recommended;
    String os_version;
    
    // Display
    u32 min_resolution_w = 1920;
    u32 min_resolution_h = 1080;
    u32 rec_resolution_w = 2560;
    u32 rec_resolution_h = 1440;
    u32 min_refresh_rate = 60;
    u32 rec_refresh_rate = 144;
    bool hdr_support = false;
    bool vrr_support = false;
    
    // API
    String dx_version;
    String vk_version;
    String metal_version;
    
    // Additional
    String notes;
    std::vector<String> specific_cpus;
    std::vector<String> specific_gpus;
    
    [[nodiscard]] String to_string() const;
    [[nodiscard]] json to_json() const;
};

struct RequirementResult {
    HardwareRequirement minimum;
    HardwareRequirement recommended;
    HardwareRequirement high;
    HardwareRequirement ultra;
    Opt<HardwareRequirement> ray_tracing;
    
    f32 confidence = 0.0f;  // 0.0 - 1.0
    String analysis_notes;
    std::vector<String> warnings;
    std::vector<String> assumptions;
    
    [[nodiscard]] String to_string() const;
    [[nodiscard]] json to_json() const;
};

class RequirementCalculator {
public:
    explicit RequirementCalculator(const AnalyzerConfig& config, const HardwareDatabase& hw_db);
    
    Result<RequirementResult> calculate(const AggregatedAssets& assets);
    
    // Formulas for requirement calculation
    static u64 estimate_cpu_requirement(const AggregatedAssets& assets, RequirementLevel level);
    static u64 estimate_gpu_requirement(const AggregatedAssets& assets, RequirementLevel level);
    static u64 estimate_vram_requirement(const AggregatedAssets& assets, RequirementLevel level);
    static u64 estimate_ram_requirement(const AggregatedAssets& assets, RequirementLevel level);
    static u64 estimate_storage_requirement(const AggregatedAssets& assets, RequirementLevel level);
    
    // Resolution scaling factors
    static constexpr f32 RESOLUTION_FACTORS[5] = {
        1.0f,   // 1080p
        1.78f,  // 1440p
        2.37f,  // 4K
        4.0f,   // 8K
        1.0f    // VR (per eye)
    };
    
    // Quality scaling factors
    static constexpr f32 QUALITY_FACTORS[5] = {
        0.5f,   // Minimum
        1.0f,   // Recommended
        1.5f,   // High
        2.0f,   // Ultra
        3.0f    // Ray Tracing
    };

private:
    const AnalyzerConfig& config_;
    const HardwareDatabase& hw_db_;
    
    void calculate_cpu_req(const AggregatedAssets& assets, HardwareRequirement& req, RequirementLevel level) const;
    void calculate_gpu_req(const AggregatedAssets& assets, HardwareRequirement& req, RequirementLevel level) const;
    void calculate_ram_req(const AggregatedAssets& assets, HardwareRequirement& req, RequirementLevel level) const;
    void calculate_storage_req(const AggregatedAssets& assets, HardwareRequirement& req, RequirementLevel level) const;
    void calculate_os_req(const AggregatedAssets& assets, HardwareRequirement& req) const;
    void calculate_display_req(const AggregatedAssets& assets, HardwareRequirement& req, RequirementLevel level) const;
    void calculate_api_req(const AggregatedAssets& assets, HardwareRequirement& req) const;
    void find_matching_hardware(HardwareRequirement& req, RequirementLevel level) const;
    
    f32 calculate_confidence(const AggregatedAssets& assets) const;
};

} // namespace game_req