#include <game_req_analyzer/hardware/hardware_database.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/utils/json_utils.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <fstream>
#include <algorithm>

using namespace game_req;

HardwareDatabase& HardwareDatabase::instance() {
    static HardwareDatabase db;
    return db;
}

Result<void> HardwareDatabase::initialize(const Path& cache_dir) {
    cache_dir_ = cache_dir;
    return load_from_cache();
}

Result<void> HardwareDatabase::load_from_cache() {
    // Try to load from cache files
    auto cpu_result = load_cpu_database();
    auto gpu_result = load_gpu_database();
    auto ram_result = load_ram_database();
    auto storage_result = load_storage_database();
    
    if (!cpu_result || !gpu_result || !ram_result || !storage_result) {
        // If loading from cache fails, load default data
        LOG_WARN("Failed to load hardware database from cache, loading default data");
        load_default_data();
    }
    
    build_indices();
    return success();
}

Result<void> HardwareDatabase::save_to_cache() const {
    // Save to cache files (JSON format)
    std::error_code ec;
    if (!fs::exists(cache_dir_)) {
        fs::create_directories(cache_dir_, ec);
        if (ec) {
            return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
                std::format("Failed to create cache directory: {}", ec.message())));
        }
    }
    
    // Save CPUs
    auto cpu_path = cache_dir_ / "cpus.json";
    Json cpu_json;
    for (const auto& cpu : cpus_) {
        Json cpu_obj;
        cpu_obj["name"] = cpu.name;
        cpu_obj["vendor"] = cpu.vendor;
        cpu_obj["architecture"] = cpu.architecture;
        cpu_obj["microarchitecture"] = cpu.microarchitecture;
        cpu_obj["socket"] = cpu.socket;
        cpu_obj["cores"] = cpu.cores;
        cpu_obj["threads"] = cpu.threads;
        cpu_obj["base_clock"] = cpu.base_clock;
        cpu_obj["boost_clock"] = cpu.boost_clock;
        cpu_obj["l1_cache"] = cpu.l1_cache;
        cpu_obj["l2_cache"] = cpu.l2_cache;
        cpu_obj["l3_cache"] = cpu.l3_cache;
        cpu_obj["tdp"] = cpu.tdp;
        cpu_obj["process_node"] = cpu.process_node;
        cpu_obj["instruction_sets"] = cpu.instruction_sets;
        cpu_obj["passmark_st"] = cpu.passmark_st;
        cpu_obj["passmark_mt"] = cpu.passmark_mt;
        cpu_obj["geekbench_st"] = cpu.geekbench_st;
        cpu_obj["geekbench_mt"] = cpu.geekbench_mt;
        cpu_obj["cinebench_r23_st"] = cpu.cinebench_r23_st;
        cpu_obj["cinebench_r23_mt"] = cpu.cinebench_r23_mt;
        cpu_obj["release_year"] = cpu.release_year;
        cpu_obj["release_quarter"] = cpu.release_quarter;
        cpu_obj["category"] = cpu.category;
        cpu_obj["price_usd"] = cpu.price_usd;
        cpu_obj["url"] = cpu.url;
        
        // Convert TimePoint to string
        std::time_t time = std::chrono::system_clock::to_time_t(cpu.last_updated);
        cpu_obj["last_updated"] = std::ctime(&time);
        cpu_obj["data_version"] = cpu.data_version;
        
        cpu_json.push_back(cpu_obj);
    }
    
    auto cpu_result = JsonUtils::write_file(cpu_path, cpu_json, 4);
    if (!cpu_result) {
        return make_unexpected(cpu_result.error());
    }
    
    // Save GPUs
    auto gpu_path = cache_dir_ / "gpus.json";
    Json gpu_json;
    for (const auto& gpu : gpus_) {
        Json gpu_obj;
        gpu_obj["name"] = gpu.name;
        gpu_obj["vendor"] = gpu.vendor;
        gpu_obj["architecture"] = gpu.architecture;
        gpu_obj["codename"] = gpu.codename;
        gpu_obj["vram"] = gpu.vram;
        gpu_obj["vram_type"] = gpu.vram_type;
        gpu_obj["vram_bus_width"] = gpu.vram_bus_width;
        gpu_obj["vram_bandwidth"] = gpu.vram_bandwidth;
        gpu_obj["compute_units"] = gpu.compute_units;
        gpu_obj["cuda_cores"] = gpu.cuda_cores;
        gpu_obj["tensor_cores"] = gpu.tensor_cores;
        gpu_obj["rt_cores"] = gpu.rt_cores;
        gpu_obj["base_clock"] = gpu.base_clock;
        gpu_obj["boost_clock"] = gpu.boost_clock;
        gpu_obj["tdp"] = gpu.tdp;
        gpu_obj["process_node"] = gpu.process_node;
        gpu_obj["passmark_g3d"] = gpu.passmark_g3d;
        gpu_obj["timespy_graphics"] = gpu.timespy_graphics;
        gpu_obj["firestrike_graphics"] = gpu.firestrike_graphics;
        gpu_obj["port_royal"] = gpu.port_royal;
        gpu_obj["dlss_support"] = gpu.dlss_support;
        gpu_obj["fsr_support"] = gpu.fsr_support;
        gpu_obj["xess_support"] = gpu.xess_support;
        gpu_obj["ray_tracing"] = gpu.ray_tracing;
        gpu_obj["dx_version"] = gpu.dx_version;
        gpu_obj["vk_version"] = gpu.vk_version;
        gpu_obj["metal_version"] = gpu.metal_version;
        gpu_obj["opencl_version"] = gpu.opencl_version;
        gpu_obj["release_year"] = gpu.release_year;
        gpu_obj["release_quarter"] = gpu.release_quarter;
        gpu_obj["category"] = gpu.category;
        gpu_obj["form_factor"] = gpu.form_factor;
        gpu_obj["price_usd"] = gpu.price_usd;
        gpu_obj["url"] = gpu.url;
        
        // Convert TimePoint to string
        std::time_t time = std::chrono::system_clock::to_time_t(gpu.last_updated);
        gpu_obj["last_updated"] = std::ctime(&time);
        gpu_obj["data_version"] = gpu.data_version;
        
        gpu_json.push_back(gpu_obj);
    }
    
    auto gpu_result = JsonUtils::write_file(gpu_path, gpu_json, 4);
    if (!gpu_result) {
        return make_unexpected(gpu_result.error());
    }
    
    // Save RAM
    auto ram_path = cache_dir_ / "ram.json";
    Json ram_json;
    for (const auto& ram : ram_) {
        Json ram_obj;
        ram_obj["name"] = ram.name;
        ram_obj["type"] = ram.type;
        ram_obj["capacity"] = ram.capacity;
        ram_obj["speed"] = ram.speed;
        ram_obj["timings"] = ram.timings;
        ram_obj["voltage"] = ram.voltage;
        ram_obj["channels"] = ram.channels;
        ram_obj["ecc"] = ram.ecc;
        ram_obj["registered"] = ram.registered;
        ram_obj["form_factor"] = ram.form_factor;
        ram_obj["release_year"] = ram.release_year;
        ram_obj["price_usd"] = ram.price_usd;
        
        // Convert TimePoint to string
        std::time_t time = std::chrono::system_clock::to_time_t(ram.last_updated);
        ram_obj["last_updated"] = std::ctime(&time);
        
        ram_json.push_back(ram_obj);
    }
    
    auto ram_result = JsonUtils::write_file(ram_path, ram_json, 4);
    if (!ram_result) {
        return make_unexpected(ram_result.error());
    }
    
    // Save Storage
    auto storage_path = cache_dir_ / "storage.json";
    Json storage_json;
    for (const auto& storage : storage_) {
        Json storage_obj;
        storage_obj["name"] = storage.name;
        storage_obj["type"] = storage.type;
        storage_obj["interface"] = storage.interface;
        storage_obj["capacity"] = storage.capacity;
        storage_obj["seq_read"] = storage.seq_read;
        storage_obj["seq_write"] = storage.seq_write;
        storage_obj["rand_read"] = storage.rand_read;
        storage_obj["rand_write"] = storage.rand_write;
        storage_obj["form_factor"] = storage.form_factor;
        storage_obj["controller"] = storage.controller;
        storage_obj["nand_type"] = storage.nand_type;
        storage_obj["tdp"] = storage.tdp;
        storage_obj["release_year"] = storage.release_year;
        storage_obj["price_usd"] = storage.price_usd;
        
        // Convert TimePoint to string
        std::time_t time = std::chrono::system_clock::to_time_t(storage.last_updated);
        storage_obj["last_updated"] = std::ctime(&time);
        
        storage_json.push_back(storage_obj);
    }
    
    auto storage_result = JsonUtils::write_file(storage_path, storage_json, 4);
    if (!storage_result) {
        return make_unexpected(storage_result.error());
    }
    
    return success();
}

Result<void> HardwareDatabase::update_from_sources() {
    // In a real implementation, this would download updated hardware data from online sources
    // For now, we'll just log that we're updating
    LOG_INFO("Checking for hardware database updates...");
    
    // Simulate checking for updates
    // In reality, we'd check timestamps or version numbers from a remote source
    
    // For now, just return success (no update needed)
    return success();
}

const CPUEntry* HardwareDatabase::find_cpu(StringView name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = cpu_index_.find(String(name));
    if (it != cpu_index_.end()) {
        return &cpus_[it->second];
    }
    return nullptr;
}

std::vector<const CPUEntry*> HardwareDatabase::find_cpus_by_score(u32 min_passmark_mt, u32 max_results) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<const CPUEntry*> result;
    for (const auto& cpu : cpus_) {
        if (cpu.passmark_mt >= min_passmark_mt) {
            result.push_back(&cpu);
            if (result.size() >= max_results) {
                break;
            }
        }
    }
    
    // Sort by score descending
    std::sort(result.begin(), result.end(), [](const CPUEntry* a, const CPUEntry* b) {
        return a->passmark_mt > b->passmark_mt;
    });
    
    return result;
}

std::vector<const CPUEntry*> HardwareDatabase::find_cpus_by_cores(u32 min_cores, u32 max_results) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<const CPUEntry*> result;
    for (const auto& cpu : cpus_) {
        if (cpu.cores >= min_cores) {
            result.push_back(&cpu);
            if (result.size() >= max_results) {
                break;
            }
        }
    }
    
    // Sort by cores descending
    std::sort(result.begin(), result.end(), [](const CPUEntry* a, const CPUEntry* b) {
        return a->cores > b->cores;
    });
    
    return result;
}

std::vector<const CPUEntry*> HardwareDatabase::find_best_cpus(u32 budget_usd, u32 max_results) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<const CPUEntry*> result;
    for (const auto& cpu : cpus_) {
        if (budget_usd == 0 || cpu.price_usd <= static_cast<f32>(budget_usd)) {
            result.push_back(&cpu);
        }
    }
    
    // Sort by performance per dollar descending
    std::sort(result.begin(), result.end(), [](const CPUEntry* a, const CPUEntry* b) {
        return a->performance_per_dollar() > b->performance_per_dollar();
    });
    
    if (result.size() > max_results) {
        result.resize(max_results);
    }
    
    return result;
}

std::vector<const CPUEntry*> HardwareDatabase::get_all_cpus() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<const CPUEntry*> result;
    result.reserve(cpus_.size());
    for (const auto& cpu : cpus_) {
        result.push_back(&cpu);
    }
    
    return result;
}

const GPUEntry* HardwareDatabase::find_gpu(StringView name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = gpu_index_.find(String(name));
    if (it != gpu_index_.end()) {
        return &gpus_[it->second];
    }
    return nullptr;
}

std::vector<const GPUEntry*> HardwareDatabase::find_gpus_by_score(u32 min_g3d, u32 max_results) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<const GPUEntry*> result;
    for (const auto& gpu : gpus_) {
        if (gpu.passmark_g3d >= min_g3d) {
            result.push_back(&gpu);
            if (result.size() >= max_results) {
                break;
            }
        }
    }
    
    // Sort by score descending
    std::sort(result.begin(), result.end(), [](const GPUEntry* a, const GPUEntry* b) {
        return a->passmark_g3d > b->passmark_g3d;
    });
    
    return result;
}

std::vector<const GPUEntry*> HardwareDatabase::find_gpus_by_vram(u64 min_vram_mb, u32 max_results) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<const GPUEntry*> result;
    for (const auto& gpu : gpus_) {
        if (gpu.vram >= min_vram_mb) {
            result.push_back(&gpu);
            if (result.size() >= max_results) {
                break;
            }
        }
    }
    
    // Sort by VRAM descending
    std::sort(result.begin(), result.end(), [](const GPUEntry* a, const GPUEntry* b) {
        return a->vram > b->vram;
    });
    
    return result;
}

std::vector<const GPUEntry*> HardwareDatabase::find_best_gpus(u32 budget_usd, u32 max_results) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<const GPUEntry*> result;
    for (const auto& gpu : gpus_) {
        if (budget_usd == 0 || gpu.price_usd <= static_cast<f32>(budget_usd)) {
            result.push_back(&gpu);
        }
    }
    
    // Sort by performance per dollar descending
    std::sort(result.begin(), result.end(), [](const GPUEntry* a, const GPUEntry* b) {
        return a->performance_per_dollar() > b->performance_per_dollar();
    });
    
    if (result.size() > max_results) {
        result.resize(max_results);
    }
    
    return result;
}

std::vector<const GPUEntry*> HardwareDatabase::get_all_gpus() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<const GPUEntry*> result;
    result.reserve(gpus_.size());
    for (const auto& gpu : gpus_) {
        result.push_back(&gpu);
    }
    
    return result;
}

const RAMEntry* HardwareDatabase::find_ram(StringView name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = ram_index_.find(String(name));
    if (it != ram_index_.end()) {
        return &ram_[it->second];
    }
    return nullptr;
}

std::vector<const RAMEntry*> HardwareDatabase::find_ram_by_speed(f32 min_speed, u32 max_results) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<const RAMEntry*> result;
    for (const auto& ram : ram_) {
        if (ram.speed >= min_speed) {
            result.push_back(&ram);
            if (result.size() >= max_results) {
                break;
            }
        }
    }
    
    // Sort by speed descending
    std::sort(result.begin(), result.end(), [](const RAMEntry* a, const RAMEntry* b) {
        return a->speed > b->speed;
    });
    
    return result;
}

std::vector<const RAMEntry*> HardwareDatabase::find_ram_by_capacity(u64 capacity_gb, u32 max_results) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<const RAMEntry*> result;
    for (const auto& ram : ram_) {
        if (ram.capacity >= capacity_gb) {
            result.push_back(&ram);
            if (result.size() >= max_results) {
                break;
            }
        }
    }
    
    // Sort by capacity descending
    std::sort(result.begin(), result.end(), [](const RAMEntry* a, const RAMEntry* b) {
        return a->capacity > b->capacity;
    });
    
    return result;
}

std::vector<const RAMEntry*> HardwareDatabase::get_all_ram() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<const RAMEntry*> result;
    result.reserve(ram_.size());
    for (const auto& ram : ram_) {
        result.push_back(&ram);
    }
    
    return result;
}

const StorageEntry* HardwareDatabase::find_storage(StringView name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = storage_index_.find(String(name));
    if (it != storage_index_.end()) {
        return &storage_[it->second];
    }
    return nullptr;
}

std::vector<const StorageEntry*> HardwareDatabase::find_storage_by_speed(f32 min_seq_read, u32 max_results) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<const StorageEntry*> result;
    for (const auto& storage : storage_) {
        if (storage.seq_read >= min_seq_read) {
            result.push_back(&storage);
            if (result.size() >= max_results) {
                break;
            }
        }
    }
    
    // Sort by sequential read speed descending
    std::sort(result.begin(), result.end(), [](const StorageEntry* a, const StorageEntry* b) {
        return a->seq_read > b->seq_read;
    });
    
    return result;
}

std::vector<const StorageEntry*> HardwareDatabase::find_storage_by_capacity(u64 capacity_gb, u32 max_results) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<const StorageEntry*> result;
    for (const auto& storage : storage_) {
        if (storage.capacity >= capacity_gb) {
            result.push_back(&storage);
            if (result.size() >= max_results) {
                break;
            }
        }
    }
    
    // Sort by capacity descending
    std::sort(result.begin(), result.end(), [](const StorageEntry* a, const StorageEntry* b) {
        return a->capacity > b->capacity;
    });
    
    return result;
}

std::vector<const StorageEntry*> HardwareDatabase::get_all_storage() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<const StorageEntry*> result;
    result.reserve(storage_.size());
    for (const auto& storage : storage_) {
        result.push_back(&storage);
    }
    
    return result;
}

void HardwareDatabase::set_update_callback(std::function<void(f32, StringView)> cb) {
    update_callback_ = cb;
}

Result<void> HardwareDatabase::load_cpu_database() {
    // Try to load from cache file
    auto cpu_path = cache_dir_ / "cpus.json";
    if (!fs::exists(cpu_path)) {
        return make_unexpected(MAKE_ERROR(ErrorCode::FileNotFound, 
            std::format("CPU database file not found: {}", cpu_path.string())));
    }
    
    auto json_result = JsonUtils::parse_file(cpu_path);
    if (!json_result) {
        return make_unexpected(json_result.error());
    }
    
    const Json& j = *json_result;
    cpus_.clear();
    
    for (const auto& item : j) {
        CPUEntry cpu;
        cpu.name = item.value("name", "");
        cpu.vendor = item.value("vendor", "");
        cpu.architecture = item.value("architecture", "");
        cpu.microarchitecture = item.value("microarchitecture", "");
        cpu.socket = item.value("socket", "");
        cpu.cores = item.value("cores", 0u);
        cpu.threads = item.value("threads", 0u);
        cpu.base_clock = item.value("base_clock", 0.0f);
        cpu.boost_clock = item.value("boost_clock", 0.0f);
        cpu.l1_cache = item.value("l1_cache", 0ull);
        cpu.l2_cache = item.value("l2_cache", 0ull);
        cpu.l3_cache = item.value("l3_cache", 0ull);
        cpu.tdp = item.value("tdp", 0u);
        cpu.process_node = item.value("process_node", "");
        cpu.instruction_sets = item.value("instruction_sets", "");
        cpu.passmark_st = item.value("passmark_st", 0u);
        cpu.passmark_mt = item.value("passmark_mt", 0u);
        cpu.geekbench_st = item.value("geekbench_st", 0u);
        cpu.geekbench_mt = item.value("geekbench_mt", 0u);
        cpu.cinebench_r23_st = item.value("cinebench_r23_st", 0u);
        cpu.cinebench_r23_mt = item.value("cinebench_r23_mt", 0u);
        cpu.release_year = item.value("release_year", 0u);
        cpu.release_quarter = item.value("release_quarter", 0u);
        cpu.category = item.value("category", "");
        cpu.price_usd = item.value("price_usd", 0.0f);
        cpu.url = item.value("url", "");
        
        // Parse last_updated string (simplified)
        std::string date_str = item.value("last_updated", "");
        // In a real implementation, we'd parse this properly
        cpu.last_updated = std::chrono::system_clock::now();
        
        cpu.data_version = item.value("data_version", 0u);
        
        cpus_.push_back(cpu);
    }
    
    return success();
}

Result<void> HardwareDatabase::load_gpu_database() {
    // Try to load from cache file
    auto gpu_path = cache_dir_ / "gpus.json";
    if (!fs::exists(gpu_path)) {
        return make_unexpected(MAKE_ERROR(ErrorCode::FileNotFound, 
            std::format("GPU database file not found: {}", gpu_path.string())));
    }
    
    auto json_result = JsonUtils::parse_file(gpu_path);
    if (!json_result) {
        return make_unexpected(json_result.error());
    }
    
    const Json& j = *json_result;
    gpus_.clear();
    
    for (const auto& item : j) {
        GPUEntry gpu;
        gpu.name = item.value("name", "");
        gpu.vendor = item.value("vendor", "");
        gpu.architecture = item.value("architecture", "");
        gpu.codename = item.value("codename", "");
        gpu.vram = item.value("vram", 0ull);
        gpu.vram_type = item.value("vram_type", "");
        gpu.vram_bus_width = item.value("vram_bus_width", 0u);
        gpu.vram_bandwidth = item.value("vram_bandwidth", 0.0f);
        gpu.compute_units = item.value("compute_units", 0u);
        gpu.cuda_cores = item.value("cuda_cores", 0u);
        gpu.tensor_cores = item.value("tensor_cores", 0u);
        gpu.rt_cores = item.value("rt_cores", 0u);
        gpu.base_clock = item.value("base_clock", 0.0f);
        gpu.boost_clock = item.value("boost_clock", 0.0f);
        gpu.tdp = item.value("tdp", 0u);
        gpu.process_node = item.value("process_node", "");
        gpu.passmark_g3d = item.value("passmark_g3d", 0u);
        gpu.timespy_graphics = item.value("timespy_graphics", 0u);
        gpu.firestrike_graphics = item.value("firestrike_graphics", 0u);
        gpu.port_royal = item.value("port_royal", 0u);
        gpu.dlss_support = item.value("dlss_support", false);
        gpu.fsr_support = item.value("fsr_support", false);
        gpu.xess_support = item.value("xess_support", false);
        gpu.ray_tracing = item.value("ray_tracing", false);
        gpu.dx_version = item.value("dx_version", "");
        gpu.vk_version = item.value("vk_version", "");
        gpu.metal_version = item.value("metal_version", "");
        gpu.opencl_version = item.value("opencl_version", "");
        gpu.release_year = item.value("release_year", 0u);
        gpu.release_quarter = item.value("release_quarter", 0u);
        gpu.category = item.value("category", "");
        gpu.form_factor = item.value("form_factor", "");
        gpu.price_usd = item.value("price_usd", 0.0f);
        gpu.url = item.value("url", "");
        
        // Parse last_updated string (simplified)
        std::string date_str = item.value("last_updated", "");
        // In a real implementation, we'd parse this properly
        gpu.last_updated = std::chrono::system_clock::now();
        
        gpu.data_version = item.value("data_version", 0u);
        
        gpus_.push_back(gpu);
    }
    
    return success();
}

Result<void> HardwareDatabase::load_ram_database() {
    // Try to load from cache file
    auto ram_path = cache_dir_ / "ram.json";
    if (!fs::exists(ram_path)) {
        return make_unexpected(MAKE_ERROR(ErrorCode::FileNotFound, 
            std::format("RAM database file not found: {}", ram_path.string())));
    }
    
    auto json_result = JsonUtils::parse_file(ram_path);
    if (!json_result) {
        return make_unexpected(json_result.error());
    }
    
    const Json& j = *json_result;
    ram_.clear();
    
    for (const auto& item : j) {
        RAMEntry ram;
        ram.name = item.value("name", "");
        ram.type = item.value("type", "");
        ram.capacity = item.value("capacity", 0ull);
        ram.speed = item.value("speed", 0.0f);
        ram.timings = item.value("timings", "");
        ram.voltage = item.value("voltage", 0.0f);
        ram.channels = item.value("channels", 0u);
        ram.ecc = item.value("ecc", false);
        ram.registered = item.value("registered", false);
        ram.form_factor = item.value("form_factor", "");
        ram.release_year = item.value("release_year", 0u);
        ram.price_usd = item.value("price_usd", 0.0f);
        
        // Parse last_updated string (simplified)
        std::string date_str = item.value("last_updated", "");
        // In a real implementation, we'd parse this properly
        ram.last_updated = std::chrono::system_clock::now();
        
        ram_.push_back(ram);
    }
    
    return success();
}

Result<void> HardwareDatabase::load_storage_database() {
    // Try to load from cache file
    auto storage_path = cache_dir_ / "storage.json";
    if (!fs::exists(storage_path)) {
        return make_unexpected(MAKE_ERROR(ErrorCode::FileNotFound, 
            std::format("Storage database file not found: {}", storage_path.string())));
    }
    
    auto json_result = JsonUtils::parse_file(storage_path);
    if (!json_result) {
        return make_unexpected(json_result.error());
    }
    
    const Json& j = *json_result;
    storage_.clear();
    
    for (const auto& item : j) {
        StorageEntry storage;
        storage.name = item.value("name", "");
        storage.type = item.value("type", "");
        storage.interface = item.value("interface", "");
        storage.capacity = item.value("capacity", 0ull);
        storage.seq_read = item.value("seq_read", 0.0f);
        storage.seq_write = item.value("seq_write", 0.0f);
        storage.rand_read = item.value("rand_read", 0u);
        storage.rand_write = item.value("rand_write", 0u);
        storage.form_factor = item.value("form_factor", "");
        storage.controller = item.value("controller", "");
        storage.nand_type = item.value("nand_type", "");
        storage.tdp = item.value("tdp", 0u);
        storage.release_year = item.value("release_year", 0u);
        storage.price_usd = item.value("price_usd", 0.0f);
        
        // Parse last_updated string (simplified)
        std::string date_str = item.value("last_updated", "");
        // In a real implementation, we'd parse this properly
        storage.last_updated = std::chrono::system_clock::now();
        
        storage_.push_back(storage);
    }
    
    return success();
}

void HardwareDatabase::build_indices() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    cpu_index_.clear();
    gpu_index_.clear();
    ram_index_.clear();
    storage_index_.clear();
    
    for (size_t i = 0; i < cpus_.size(); ++i) {
        cpu_index_[cpus_[i].name] = i;
    }
    
    for (size_t i = 0; i < gpus_.size(); ++i) {
        gpu_index_[gpus_[i].name] = i;
    }
    
    for (size_t i = 0; i < ram_.size(); ++i) {
        ram_index_[ram_[i].name] = i;
    }
    
    for (size_t i = 0; i < storage_.size(); ++i) {
        storage_index_[storage_[i].name] = i;
    }
}

void HardwareDatabase::load_default_data() {
    // Load some default CPU data
    cpus_.clear();
    cpus_.push_back(CPUEntry{
        .name = "Intel Core i5-12400F",
        .vendor = "Intel",
        .architecture = "x86_64",
        .microarchitecture = "Alder Lake",
        .socket = "LGA 1700",
        .cores = 6,
        .threads = 12,
        .base_clock = 2.5f,
        .boost_clock = 4.4f,
        .l1_cache = 384,
        .l2_cache = 7500,
        .l3_cache = 18000,
        .tdp = 65,
        .process_node = "10nm Enhanced SuperFin",
        .instruction_sets = "SSE4.2, AVX2, AVX-512",
        .passmark_st = 3200,
        .passmark_mt = 20000,
        .geekbench_st = 1800,
        .geekbench_mt = 10000,
        .cinebench_r23_st = 1600,
        .cinebench_r23_mt = 12000,
        .release_year = 2022,
        .release_quarter = 1,
        .category = "Desktop",
        .price_usd = 150.0f,
        .url = "https://www.intel.com/content/www/us/products/docs/processors/core/i5-12400f.html",
        .last_updated = std::chrono::system_clock::now(),
        .data_version = 1
    });
    
    cpus_.push_back(CPUEntry{
        .name = "AMD Ryzen 5 5600X",
        .vendor = "AMD",
        .architecture = "x86_64",
        .microarchitecture = "Zen 3",
        .socket = "AM4",
        .cores = 6,
        .threads = 12,
        .base_clock = 3.7f,
        .boost_clock = 4.6f,
        .l1_cache = 384,
        .l2_cache = 3000,
        .l3_cache = 32000,
        .tdp = 65,
        .process_node = "7nm",
        .instruction_sets = "SSE4.2, AVX2",
        .passmark_st = 3100,
        .passmark_mt = 19000,
        .geekbench_st = 1700,
        .geekbench_mt = 9500,
        .cinebench_r23_st = 1550,
        .cinebench_r23_mt = 11500,
        .release_year = 2020,
        .release_quarter = 4,
        .category = "Desktop",
        .price_usd = 200.0f,
        .url = "https://www.amd.com/en/products/ryzen-5-5600x",
        .last_updated = std::chrono::system_clock::now(),
        .data_version = 1
    });
    
    cpus_.push_back(CPUEntry{
        .name = "Intel Core i7-12700K",
        .vendor = "Intel",
        .architecture = "x86_64",
        .microarchitecture = "Alder Lake",
        .socket = "LGA 1700",
        .cores = 12,
        .threads = 20,
        .base_clock = 3.6f,
        .boost_clock = 5.0f,
        .l1_cache = 1408,
        .l2_cache = 12800,
        .l3_cache = 25000,
        .tdp = 125,
        .process_node = "10nm Enhanced SuperFin",
        .instruction_sets = "SSE4.2, AVX2, AVX-512",
        .passmark_st = 4100,
        .passmark_mt = 28000,
        .geekbench_st = 2200,
        .geekbench_mt = 16000,
        .cinebench_r23_st = 2000,
        .cinebench_r23_mt = 18000,
        .release_year = 2021,
        .release_quarter = 4,
        .category = "Desktop",
        .price_usd = 350.0f,
        .url = "https://www.intel.com/content/www/us/products/docs/processors/core/i7-12700k.html",
        .last_updated = std::chrono::system_clock::now(),
        .data_version = 1
    });
    
    cpus_.push_back(CPUEntry{
        .name = "AMD Ryzen 7 5800X3D",
        .vendor = "AMD",
        .architecture = "x86_64",
        .microarchitecture = "Zen 3",
        .socket = "AM4",
        .cores = 8,
        .threads = 16,
        .base_clock = 3.4f,
        .boost_clock = 4.5f,
        .l1_cache = 512,
        .l2_cache = 4000,
        .l3_cache = 96000,
        .tdp = 105,
        .process_node = "7nm",
        .instruction_sets = "SSE4.2, AVX2",
        .passmark_st = 3500,
        .passmark_mt = 22000,
        .geekbench_st = 1900,
        .geekbench_mt = 12000,
        .cinebench_r23_st = 1700,
        .cinebench_r23_mt = 14000,
        .release_year = 2022,
        .release_quarter = 2,
        .category = "Desktop",
        .price_usd = 450.0f,
        .url = "https://www.amd.com/en/products/ryzen-7-5800x3d",
        .last_updated = std::chrono::system_clock::now(),
        .data_version = 1
    });
    
    // Load some default GPU data
    gpus_.clear();
    gpus_.push_back(GPUEntry{
        .name = "NVIDIA GeForce RTX 3060 12GB",
        .vendor = "NVIDIA",
        .architecture = "Ampere",
        .codename = "GA106",
        .vram = 12288,
        .vram_type = "GDDR6",
        .vram_bus_width = 192,
        .vram_bandwidth = 360.0f,
        .compute_units = 28,
        .cuda_cores = 3584,
        .tensor_cores = 112,
        .rt_cores = 28,
        .base_clock = 1320.0f,
        .boost_clock = 1777.0f,
        .tdp = 170,
        .process_node = "8nm",
        .passmark_g3d = 9000,
        .timespy_graphics = 8500,
        .firestrike_graphics = 12000,
        .port_royal = 4500,
        .dlss_support = true,
        .fsr_support = false,
        .xess_support = false,
        .ray_tracing = true,
        .dx_version = "12.2",
        .vk_version = "1.3",
        .metal_version = "",
        .opencl_version = "3.0",
        .release_year = 2021,
        .release_quarter = 1,
        .category = "Desktop",
        .price_usd = 300.0f,
        .url = "https://www.nvidia.com/GeForce-Graphics-Cards/30-series/rtx-3060",
        .last_updated = std::chrono::system_clock::now(),
        .data_version = 1
    });
    
    gpus_.push_back(GPUEntry{
        .name = "AMD Radeon RX 6700 XT",
        .vendor = "AMD",
        .architecture = "RDNA 2",
        .codename = "Navi 22",
        .vram = 12288,
        .vram_type = "GDDR6",
        .vram_bus_width = 192,
        .vram_bandwidth = 384.0f,
        .compute_units = 40,
        .cuda_cores = 0,
        .tensor_cores = 0,
        .rt_cores = 32,
        .base_clock = 2321.0f,
        .boost_clock = 2581.0f,
        .tdp = 230,
        .process_node = "7nm",
        .passmark_g3d = 11000,
        .timespy_graphics = 10500,
        .firestrike_graphics = 15000,
        .port_royal = 6500,
        .dlss_support = false,
        .fsr_support = true,
        .xess_support = false,
        .ray_tracing = true,
        .dx_version = "12.2",
        .vk_version = "1.3",
        .metal_version = "",
        .opencl_version = "2.2",
        .release_year = 2021,
        .release_quarter = 2,
        .category = "Desktop",
        .price_usd = 400.0f,
        .url = "https://www.amd.com/en/products/radeon-rx-6700-xt",
        .last_updated = std::chrono::system_clock::now(),
        .data_version = 1
    });
    
    gpus_.push_back(GPUEntry{
        .name = "NVIDIA GeForce RTX 3080",
        .vendor = "NVIDIA",
        .architecture = "Ampere",
        .codename = "GA102",
        .vram = 10240,
        .vram_type = "GDDR6X",
        .vram_bus_width = 320,
        .vram_bandwidth = 760.0f,
        .compute_units = 68,
        .cuda_cores = 8704,
        .tensor_cores = 272,
        .rt_cores = 68,
        .base_clock = 1440.0f,
        .boost_clock = 1710.0f,
        .tdp = 320,
        .process_node = "8nm",
        .passmark_g3d = 14000,
        .timespy_graphics = 13500,
        .firestrike_graphics = 20000,
        .port_royal = 10000,
        .dlss_support = true,
        .fsr_support = false,
        .xess_support = false,
        .ray_tracing = true,
        .dx_version = "12.2",
        .vk_version = "1.3",
        .metal_version = "",
        .opencl_version = "3.0",
        .release_year = 2020,
        .release_quarter = 3,
        .category = "Desktop",
        .price_usd = 700.0f,
        .url = "https://www.nvidia.com/GeForce-Graphics-Cards/30-series/rtx-3080",
        .last_updated = std::chrono::system_clock::now(),
        .data_version = 1
    });
    
    // Load some default RAM data
    ram_.clear();
    ram_.push_back(RAMEntry{
        .name = "Corsair Vengeance LPX 16GB (2x8GB) DDR4 3200MHz",
        .type = "DDR4",
        .capacity = 8,
        .speed = 3200,
        .timings = "16-18-18-36",
        .voltage = 1.35f,
        .channels = 2,
        .ecc = false,
        .registered = false,
        .form_factor = "DIMM",
        .release_year = 2020,
        .price_usd = 70.0f,
        .last_updated = std::chrono::system_clock::now()
    });
    
    ram_.push_back(RAMEntry{
        .name = "G.Skill Trident Z5 32GB (2x16GB) DDR5 6000MHz",
        .type = "DDR5",
        .capacity = 16,
        .speed = 6000,
        .timings = "36-38-38-76",
        .voltage = 1.35f,
        .channels = 2,
        .ecc = false,
        .registered = false,
        .form_factor = "DIMM",
        .release_year = 2022,
        .price_usd = 220.0f,
        .last_updated = std::chrono::system_clock::now()
    });
    
    // Load some default Storage data
    storage_.clear();
    storage_.push_back(StorageEntry{
        .name = "Samsung 970 EVO Plus 1TB NVMe SSD",
        .type = "NVMe",
        .interface = "PCIe 3.0 x4",
        .capacity = 1024,
        .seq_read = 3500,
        .seq_write = 3300,
        .rand_read = 620000,
        .rand_write = 560000,
        .form_factor = "M.2 2280",
        .controller = "Samsung",
        .nand_type = "TLC",
        .tdp = 6,
        .release_year = 2020,
        .price_usd = 100.0f,
        .last_updated = std::chrono::system_clock::now()
    });
    
    storage_.push_back(StorageEntry{
        .name = "Western Digital Black SN850 1TB NVMe SSD",
        .type = "NVMe",
        .interface = "PCIe 4.0 x4",
        .capacity = 1024,
        .seq_read = 7000,
        .seq_write = 5100,
        .rand_read = 1000000,
        .rand_write = 950000,
        .form_factor = "M.2 2280",
        .controller = "Western Digital",
        .nand_type = "TLC",
        .tdp = 8,
        .release_year = 2021,
        .price_usd = 130.0f,
        .last_updated = std::chrono::system_clock::now()
    });
    
    storage_.push_back(StorageEntry{
        .name = "Seagate Barracuda 2TB HDD",
        .type = "HDD",
        .interface = "SATA 3",
        .capacity = 2048,
        .seq_read = 190,
        .seq_write = 190,
        .rand_read = 120,
        .rand_write = 120,
        .form_factor = "3.5\"",
        .controller = "Seagate",
        .nand_type = "",
        .tdp = 6,
        .release_year = 2020,
        .price_usd = 45.0f,
        .last_updated = std::chrono::system_clock::now()
    });
}
