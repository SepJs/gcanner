#include <game_req_analyzer/hardware/hardware_updater.hpp>
#include <game_req_analyzer/core/logger.hpp>

using namespace game_req;

HardwareUpdater::HardwareUpdater(HardwareDatabase& db) : db_(db) {}

HardwareUpdater::~HardwareUpdater() = default;

Result<UpdateStats> HardwareUpdater::check_and_update() {
    UpdateStats stats;
    return stats;
}

Result<UpdateStats> HardwareUpdater::force_update() {
    UpdateStats stats;
    return stats;
}

Result<void> HardwareUpdater::add_source(const DataSource& source) {
    return success();
}

Result<void> HardwareUpdater::remove_source(StringView name) {
    return success();
}

std::vector<DataSource> HardwareUpdater::sources() const {
    return {};
}

Result<void> HardwareUpdater::init_default_sources() {
    return success();
}

Result<UpdateStats> HardwareUpdater::fetch_all_sources() {
    UpdateStats stats;
    return stats;
}

Result<u32> HardwareUpdater::fetch_cpu_data(const DataSource& source) {
    return 0;
}

Result<u32> HardwareUpdater::fetch_gpu_data(const DataSource& source) {
    return 0;
}

Result<u32> HardwareUpdater::fetch_ram_data(const DataSource& source) {
    return 0;
}

Result<u32> HardwareUpdater::fetch_storage_data(const DataSource& source) {
    return 0;
}

Result<std::vector<CPUEntry>> HardwareUpdater::parse_passmark_cpu(const String& html) {
    return std::vector<CPUEntry>{};
}

Result<std::vector<GPUEntry>> HardwareUpdater::parse_passmark_gpu(const String& html) {
    return std::vector<GPUEntry>{};
}

Result<std::vector<CPUEntry>> HardwareUpdater::parse_geekbench_cpu(const String& json) {
    return std::vector<CPUEntry>{};
}

Result<std::vector<GPUEntry>> HardwareUpdater::parse_geekbench_gpu(const String& json) {
    return std::vector<GPUEntry>{};
}

Result<std::vector<CPUEntry>> HardwareUpdater::parse_techpowerup_cpu(const String& html) {
    return std::vector<CPUEntry>{};
}

Result<std::vector<GPUEntry>> HardwareUpdater::parse_techpowerup_gpu(const String& html) {
    return std::vector<GPUEntry>{};
}

Result<std::vector<CPUEntry>> HardwareUpdater::parse_cpubenchmark_net(const String& html) {
    return std::vector<CPUEntry>{};
}

Result<std::vector<GPUEntry>> HardwareUpdater::parse_gpubenchmark_net(const String& html) {
    return std::vector<GPUEntry>{};
}

Result<std::vector<RAMEntry>> HardwareUpdater::parse_ram_data(const String& json) {
    return std::vector<RAMEntry>{};
}

Result<std::vector<StorageEntry>> HardwareUpdater::parse_storage_data(const String& json) {
    return std::vector<StorageEntry>{};
}

String HardwareUpdater::fetch_url(const String& url, const String& api_key) {
    return "";
}

Result<String> HardwareUpdater::download_with_retry(const String& url, u32 max_retries) {
    return "";
}

void HardwareUpdater::merge_cpu_data(const std::vector<CPUEntry>& new_data, UpdateStats& stats) {}

void HardwareUpdater::merge_gpu_data(const std::vector<GPUEntry>& new_data, UpdateStats& stats) {}

void HardwareUpdater::merge_ram_data(const std::vector<RAMEntry>& new_data, UpdateStats& stats) {}

void HardwareUpdater::merge_storage_data(const std::vector<StorageEntry>& new_data, UpdateStats& stats) {}

bool HardwareUpdater::is_new_hardware(const CPUEntry& entry) const {
    return false;
}

bool HardwareUpdater::is_new_hardware(const GPUEntry& entry) const {
    return false;
}

bool HardwareUpdater::is_significant_update(const CPUEntry& old_entry, const CPUEntry& new_entry) const {
    return false;
}

bool HardwareUpdater::is_significant_update(const GPUEntry& old_entry, const GPUEntry& new_entry) const {
    return false;
}
