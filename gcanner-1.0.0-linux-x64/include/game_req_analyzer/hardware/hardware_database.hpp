#pragma once
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/core/error.hpp>

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>

namespace game_req {

struct CPUEntry {
    String name;
    String vendor;           // Intel, AMD, ARM, Apple, Qualcomm
    String architecture;     // x86_64, arm64, etc.
    String microarchitecture; // Zen 4, Raptor Lake, etc.
    String socket;
    u32 cores = 0;
    u32 threads = 0;
    f32 base_clock = 0.0f;   // GHz
    f32 boost_clock = 0.0f;  // GHz
    u64 l1_cache = 0;        // KB
    u64 l2_cache = 0;        // KB
    u64 l3_cache = 0;        // KB
    u32 tdp = 0;             // Watts
    String process_node;     // 5nm, 7nm, etc.
    String instruction_sets; // AVX2, AVX-512, NEON, etc.
    u32 passmark_st = 0;     // Single thread score
    u32 passmark_mt = 0;     // Multi thread score
    u32 geekbench_st = 0;
    u32 geekbench_mt = 0;
    u32 cinebench_r23_st = 0;
    u32 cinebench_r23_mt = 0;
    u32 release_year = 0;
    u32 release_quarter = 0;
    String category;         // Desktop, Mobile, Server, Workstation
    f32 price_usd = 0.0f;
    String url;
    TimePoint last_updated;
    u32 data_version = 0;
    
    [[nodiscard]] f32 performance_per_watt() const {
        return tdp > 0 ? static_cast<f32>(passmark_mt) / tdp : 0.0f;
    }
    [[nodiscard]] f32 performance_per_dollar() const {
        return price_usd > 0 ? static_cast<f32>(passmark_mt) / price_usd : 0.0f;
    }
};

struct GPUEntry {
    String name;
    String vendor;           // NVIDIA, AMD, Intel, Apple, Qualcomm
    String architecture;     // Ada Lovelace, RDNA 3, Xe, etc.
    String codename;         // AD102, NAV31, etc.
    u64 vram = 0;            // MB
    String vram_type;        // GDDR6, GDDR6X, HBM2, etc.
    u32 vram_bus_width = 0;  // bits
    f32 vram_bandwidth = 0.0f; // GB/s
    u32 compute_units = 0;   // SMs, CUs, XEs
    u32 cuda_cores = 0;
    u32 tensor_cores = 0;
    u32 rt_cores = 0;
    f32 base_clock = 0.0f;   // MHz
    f32 boost_clock = 0.0f;  // MHz
    u32 tdp = 0;             // Watts
    String process_node;
    u32 passmark_g3d = 0;    // G3D Mark score
    u32 timespy_graphics = 0;
    u32 firestrike_graphics = 0;
    u32 port_royal = 0;      // Ray tracing benchmark
    bool dlss_support = false;
    bool fsr_support = false;
    bool xess_support = false;
    bool ray_tracing = false;
    String dx_version;       // 12.2, 12.1, etc.
    String vk_version;       // 1.3, 1.2, etc.
    String metal_version;
    String opencl_version;
    u32 release_year = 0;
    u32 release_quarter = 0;
    String category;         // Desktop, Mobile, Workstation, Server
    String form_factor;      // Full, SFF, MXM, Integrated
    f32 price_usd = 0.0f;
    String url;
    TimePoint last_updated;
    u32 data_version = 0;
    
    [[nodiscard]] f32 tflops_fp32() const {
        return static_cast<f32>(cuda_cores) * boost_clock * 2.0f / 1000.0f;
    }
    [[nodiscard]] f32 performance_per_watt() const {
        return tdp > 0 ? static_cast<f32>(passmark_g3d) / tdp : 0.0f;
    }
    [[nodiscard]] f32 performance_per_dollar() const {
        return price_usd > 0 ? static_cast<f32>(passmark_g3d) / price_usd : 0.0f;
    }
};

struct RAMEntry {
    String name;
    String type;             // DDR4, DDR5, LPDDR5, LPDDR5X, GDDR6, etc.
    u64 capacity = 0;        // GB per module
    f32 speed = 0.0f;        // MT/s
    String timings;          // CL-TRCD-TRP-TRAS
    f32 voltage = 0.0f;      // V
    u32 channels = 0;
    bool ecc = false;
    bool registered = false;
    String form_factor;      // DIMM, SO-DIMM, CAMM2
    u32 release_year = 0;
    f32 price_usd = 0.0f;
    TimePoint last_updated;
    
    [[nodiscard]] f32 bandwidth_gbps() const {
        return speed * 8.0f / 1000.0f; // Approximate
    }
};

struct StorageEntry {
    String name;
    String type;             // NVMe, SATA SSD, HDD
    String interface;        // PCIe 4.0 x4, PCIe 5.0 x4, SATA 3, etc.
    u64 capacity = 0;        // GB
    f32 seq_read = 0.0f;     // MB/s
    f32 seq_write = 0.0f;    // MB/s
    u32 rand_read = 0;       // IOPS
    u32 rand_write = 0;      // IOPS
    String form_factor;      // M.2 2280, M.2 2230, 2.5", U.2, etc.
    String controller;
    String nand_type;        // TLC, QLC, SLC, 3D NAND
    u32 tdp = 0;
    u32 release_year = 0;
    f32 price_usd = 0.0f;
    TimePoint last_updated;
    
    [[nodiscard]] f32 price_per_gb() const {
        return capacity > 0 ? price_usd / static_cast<f32>(capacity) : 0.0f;
    }
};

class HardwareDatabase {
public:
    static HardwareDatabase& instance();
    
    Result<void> initialize(const Path& cache_dir);
    Result<void> load_from_cache();
    Result<void> save_to_cache() const;
    Result<void> update_from_sources();
    
    // CPU queries
    [[nodiscard]] const CPUEntry* find_cpu(StringView name) const;
    [[nodiscard]] std::vector<const CPUEntry*> find_cpus_by_score(u32 min_passmark_mt, u32 max_results = 10) const;
    [[nodiscard]] std::vector<const CPUEntry*> find_cpus_by_cores(u32 min_cores, u32 max_results = 10) const;
    [[nodiscard]] std::vector<const CPUEntry*> find_best_cpus(u32 budget_usd = 0, u32 max_results = 10) const;
    [[nodiscard]] std::vector<const CPUEntry*> get_all_cpus() const;
    
    // GPU queries
    [[nodiscard]] const GPUEntry* find_gpu(StringView name) const;
    [[nodiscard]] std::vector<const GPUEntry*> find_gpus_by_score(u32 min_g3d, u32 max_results = 10) const;
    [[nodiscard]] std::vector<const GPUEntry*> find_gpus_by_vram(u64 min_vram_mb, u32 max_results = 10) const;
    [[nodiscard]] std::vector<const GPUEntry*> find_best_gpus(u32 budget_usd = 0, u32 max_results = 10) const;
    [[nodiscard]] std::vector<const GPUEntry*> get_all_gpus() const;
    
    // RAM queries
    [[nodiscard]] const RAMEntry* find_ram(StringView name) const;
    [[nodiscard]] std::vector<const RAMEntry*> find_ram_by_speed(f32 min_speed, u32 max_results = 10) const;
    [[nodiscard]] std::vector<const RAMEntry*> find_ram_by_capacity(u64 capacity_gb, u32 max_results = 10) const;
    [[nodiscard]] std::vector<const RAMEntry*> get_all_ram() const;
    
    // Storage queries
    [[nodiscard]] const StorageEntry* find_storage(StringView name) const;
    [[nodiscard]] std::vector<const StorageEntry*> find_storage_by_speed(f32 min_seq_read, u32 max_results = 10) const;
    [[nodiscard]] std::vector<const StorageEntry*> find_storage_by_capacity(u64 capacity_gb, u32 max_results = 10) const;
    [[nodiscard]] std::vector<const StorageEntry*> get_all_storage() const;
    
    // Statistics
    [[nodiscard]] u32 cpu_count() const { return cpus_.size(); }
    [[nodiscard]] u32 gpu_count() const { return gpus_.size(); }
    [[nodiscard]] u32 ram_count() const { return ram_.size(); }
    [[nodiscard]] u32 storage_count() const { return storage_.size(); }
    [[nodiscard]] TimePoint last_update() const { return last_update_; }
    
    void set_update_callback(std::function<void(f32 progress, StringView status)> cb);

private:
    HardwareDatabase() = default;
    
    std::vector<CPUEntry> cpus_;
    std::vector<GPUEntry> gpus_;
    std::vector<RAMEntry> ram_;
    std::vector<StorageEntry> storage_;
    
    std::unordered_map<String, u32> cpu_index_;
    std::unordered_map<String, u32> gpu_index_;
    std::unordered_map<String, u32> ram_index_;
    std::unordered_map<String, u32> storage_index_;
    
    TimePoint last_update_;
    u32 data_version_ = 0;
    Path cache_dir_;
    std::function<void(f32, StringView)> update_callback_;
    mutable std::shared_mutex mutex_;
    
    Result<void> load_cpu_database();
    Result<void> load_gpu_database();
    Result<void> load_ram_database();
    Result<void> load_storage_database();
    Result<void> download_and_parse(const String& url, String& output);
    void load_default_data();
    void build_indices();
};

} // namespace game_req