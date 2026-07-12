#pragma once
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/hardware/hardware_database.hpp>

namespace game_req {

struct DataSource {
    String name;
    String url;
    String api_key;
    String type;           // "passmark", "geekbench", "techpowerup", "cpubenchmark", "json", "html", "txt", "csv", "embedded"
    bool enabled = true;
    u32 priority = 0;      // Lower = higher priority
    std::chrono::hours update_interval = std::chrono::hours(168); // 1 week default
    TimePoint last_fetch;
    u32 consecutive_failures = 0;
    std::function<Result<void>(const String&, std::vector<CPUEntry>&)> cpu_parser;
    std::function<Result<void>(const String&, std::vector<GPUEntry>&)> gpu_parser;
    std::function<Result<void>(const String&, std::vector<RAMEntry>&)> ram_parser;
    std::function<Result<void>(const String&, std::vector<StorageEntry>&)> storage_parser;
};

struct UpdateStats {
    u32 cpus_added = 0;
    u32 cpus_updated = 0;
    u32 cpus_removed = 0;
    u32 gpus_added = 0;
    u32 gpus_updated = 0;
    u32 gpus_removed = 0;
    u32 ram_added = 0;
    u32 ram_updated = 0;
    u32 storage_added = 0;
    u32 storage_updated = 0;
    u32 sources_succeeded = 0;
    u32 sources_failed = 0;
    std::vector<String> errors;
    Duration total_duration;
    TimePoint timestamp;
};

class HardwareUpdater {
public:
    explicit HardwareUpdater(HardwareDatabase& db);
    ~HardwareUpdater();

    Result<UpdateStats> check_and_update();
    Result<UpdateStats> force_update();
    
    Result<void> add_source(const DataSource& source);
    Result<void> remove_source(StringView name);
    [[nodiscard]] std::vector<DataSource> sources() const;
    
    Result<void> init_default_sources();
    
    // Callback for progress reporting
    void set_progress_callback(std::function<void(f32 progress, StringView status)> callback);

private:
    HardwareDatabase& db_;
    std::vector<DataSource> sources_;
    mutable std::shared_mutex sources_mutex_;
    std::function<void(f32, StringView)> progress_callback_;
    
    // Fetching and parsing
    Result<UpdateStats> fetch_all_sources();
    Result<u32> fetch_cpu_data(const DataSource& source);
    Result<u32> fetch_gpu_data(const DataSource& source);
    Result<u32> fetch_ram_data(const DataSource& source);
    Result<u32> fetch_storage_data(const DataSource& source);
    
    // Parsers for different sources
    Result<std::vector<CPUEntry>> parse_passmark_cpu(const String& html);
    Result<std::vector<GPUEntry>> parse_passmark_gpu(const String& html);
    
    Result<std::vector<CPUEntry>> parse_geekbench_cpu(const String& json);
    Result<std::vector<GPUEntry>> parse_geekbench_gpu(const String& json);
    
    Result<std::vector<CPUEntry>> parse_techpowerup_cpu(const String& html);
    Result<std::vector<GPUEntry>> parse_techpowerup_gpu(const String& html);
    
    Result<std::vector<CPUEntry>> parse_cpubenchmark_net(const String& html);
    Result<std::vector<GPUEntry>> parse_gpubenchmark_net(const String& html);
    
    Result<std::vector<RAMEntry>> parse_ram_data(const String& json);
    Result<std::vector<StorageEntry>> parse_storage_data(const String& json);
    
    // Static embedded data (fallback/initial data)
    std::vector<CPUEntry> get_embedded_cpu_data() const;
    std::vector<GPUEntry> get_embedded_gpu_data() const;
    std::vector<RAMEntry> get_embedded_ram_data() const;
    std::vector<StorageEntry> get_embedded_storage_data() const;
    
    // HTTP utilities
    String fetch_url(const String& url, const String& api_key);
    Result<String> download_with_retry(const String& url, u32 max_retries);
    
    // Merging and deduplication
    void merge_cpu_data(const std::vector<CPUEntry>& new_data, UpdateStats& stats);
    void merge_gpu_data(const std::vector<GPUEntry>& new_data, UpdateStats& stats);
    void merge_ram_data(const std::vector<RAMEntry>& new_data, UpdateStats& stats);
    void merge_storage_data(const std::vector<StorageEntry>& new_data, UpdateStats& stats);
    
    // Comparison helpers
    bool is_new_hardware(const CPUEntry& entry) const;
    bool is_new_hardware(const GPUEntry& entry) const;
    bool is_significant_update(const CPUEntry& old_entry, const CPUEntry& new_entry) const;
    bool is_significant_update(const GPUEntry& old_entry, const GPUEntry& new_entry) const;
    
    // Signature verification for supply chain security
    bool verify_signature(const String& data, const String& signature, const String& public_key) const;
};

} // namespace game_req