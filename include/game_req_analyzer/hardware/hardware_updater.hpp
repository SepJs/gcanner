#pragma once
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/core/error.hpp>

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/hardware/hardware_database.hpp>

namespace game_req {

struct DataSource {
    String name;
    String url;
    String api_key;
    String format;        // json, csv, html, xml
    String selector;      // CSS/XPath/JSONPath selector
    u32 update_interval_hours = 168; // 1 week
    bool requires_auth = false;
    u32 rate_limit_rpm = 60;
    TimePoint last_fetch;
    bool enabled = true;
    u32 priority = 0;
};

struct UpdateStats {
    u32 cpus_added = 0;
    u32 cpus_updated = 0;
    u32 gpus_added = 0;
    u32 gpus_updated = 0;
    u32 ram_added = 0;
    u32 ram_updated = 0;
    u32 storage_added = 0;
    u32 storage_updated = 0;
    u32 errors = 0;
    Duration total_time;
    TimePoint start_time;
    TimePoint end_time;
};

class HardwareUpdater {
public:
    explicit HardwareUpdater(HardwareDatabase& db);
    
    Result<UpdateStats> check_and_update();
    Result<UpdateStats> force_update();
    Result<void> add_source(const DataSource& source);
    Result<void> remove_source(StringView name);
    [[nodiscard]] std::vector<DataSource> sources() const;
    [[nodiscard]] bool is_updating() const { return updating_.load(); }
    void cancel_update();
    
    void set_progress_callback(std::function<void(f32, StringView)> cb);

private:
    HardwareDatabase& db_;
    std::vector<DataSource> sources_;
    std::atomic<bool> updating_{false};
    std::atomic<bool> cancelled_{false};
    std::mutex sources_mutex_;
    std::function<void(f32, StringView)> progress_cb_;
    
    Result<void> init_default_sources();
    Result<UpdateStats> fetch_all_sources();
    Result<u32> fetch_cpu_data(const DataSource& source);
    Result<u32> fetch_gpu_data(const DataSource& source);
    Result<u32> fetch_ram_data(const DataSource& source);
    Result<u32> fetch_storage_data(const DataSource& source);
    
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
    
    String fetch_url(const String& url, const String& api_key = "");
    Result<String> download_with_retry(const String& url, u32 max_retries = 3);
    
    void merge_cpu_data(const std::vector<CPUEntry>& new_data, UpdateStats& stats);
    void merge_gpu_data(const std::vector<GPUEntry>& new_data, UpdateStats& stats);
    void merge_ram_data(const std::vector<RAMEntry>& new_data, UpdateStats& stats);
    void merge_storage_data(const std::vector<StorageEntry>& new_data, UpdateStats& stats);
    
    bool is_new_hardware(const CPUEntry& entry) const;
    bool is_new_hardware(const GPUEntry& entry) const;
    bool is_significant_update(const CPUEntry& old_entry, const CPUEntry& new_entry) const;
    bool is_significant_update(const GPUEntry& old_entry, const GPUEntry& new_entry) const;
};

} // namespace game_req