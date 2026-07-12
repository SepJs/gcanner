#include <game_req_analyzer/hardware/hardware_updater.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/utils/hash_utils.hpp>
#include <game_req_analyzer/utils/time_utils.hpp>
#include <game_req_analyzer/hardware/hardware_database.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <map>
#include <set>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>

using namespace game_req;

// Helper function for CURL write callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Helper function for CURL header callback (to get HTTP status code)
static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    std::string header(buffer);
    if (header.find("HTTP/") != std::string::npos) {
        std::istringstream iss(header);
        std::string http_version;
        int status_code;
        std::string status_text;
        if (iss >> http_version >> status_code >> status_text) {
            *(static_cast<int*>(userdata)) = status_code;
        }
    }
    return size * nitems;
}

HardwareUpdater::HardwareUpdater(HardwareDatabase& db) : db_(db) {
    // Initialize CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // Initialize default sources
    init_default_sources();
}

HardwareUpdater::~HardwareUpdater() {
    // Cleanup CURL
    curl_global_cleanup();
}

Result<UpdateStats> HardwareUpdater::check_and_update() {
    // Check if any sources need updating
    bool needs_update = false;
    {
        std::shared_lock<std::shared_mutex> lock(sources_mutex_);
        for (const auto& source : sources_) {
            if (!source.enabled) continue;
            
            auto now = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::hours>(now - source.last_fetch);
            if (elapsed >= source.update_interval) {
                needs_update = true;
                break;
            }
        }
    }
    
    if (!needs_update) {
        // No updates needed
        UpdateStats stats;
        stats.timestamp = std::chrono::system_clock::now();
        return stats;
    }
    
    // Perform update
    return force_update();
}

Result<UpdateStats> HardwareUpdater::force_update() {
    auto start_time = std::chrono::system_clock::now();
    UpdateStats stats;
    stats.start_time = start_time;
    
    try {
        // Fetch data from all enabled sources
        auto fetch_result = fetch_all_sources();
        if (!fetch_result) {
            return make_unexpected(fetch_result.error());
        }
        stats = *fetch_result;
        
        stats.end_time = std::chrono::system_clock::now();
        stats.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            stats.end_time - stats.start_time);
            
        return stats;
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InternalError, 
            std::format("Update failed: {}", e.what())));
    }
}

Result<void> HardwareUpdater::add_source(const DataSource& source) {
    std::unique_lock<std::shared_mutex> lock(sources_mutex_);
    
    // Check if source with this name already exists
    for (auto& s : sources_) {
        if (s.name == source.name) {
            s = source; // Update existing
            return success();
        }
    }
    
    // Add new source
    sources_.push_back(source);
    return success();
}

Result<void> HardwareUpdater::remove_source(StringView name) {
    std::unique_lock<std::shared_mutex> lock(sources_mutex_);
    
    auto it = std::remove_if(sources_.begin(), sources_.end(),
        [&name](const DataSource& s) { return s.name == name; });
    
    if (it != sources_.end()) {
        sources_.erase(it, sources_.end());
        return success();
    }
    
    return make_unexpected(MAKE_ERROR(ErrorCode::NotFound, 
        std::format("Data source not found: {}", name)));
}

std::vector<DataSource> HardwareUpdater::sources() const {
    std::shared_lock<std::shared_mutex> lock(sources_mutex_);
    return sources_;
}

Result<void> HardwareUpdater::init_default_sources() {
    std::unique_lock<std::shared_mutex> lock(sources_mutex_);
    
    // Clear existing sources
    sources_.clear();
    
    // Add default sources with example URLs
    // Note: These are example URLs - in reality you'd need to find actual data sources
    
    // PassMark CPU benchmark source (example)
    DataSource passmark_cpu;
    passmark_cpu.name = "PassMark CPU";
    passmark_cpu.url = "https://example.com/passmark_cpu.txt"; // Placeholder URL
    passmark_cpu.type = "txt";
    passmark_cpu.enabled = true;
    passmark_cpu.priority = 1; // High priority
    passmark_cpu.update_interval = std::chrono::hours(168); // Weekly
    passmark_cpu.cpu_parser = [this](const String& data, std::vector<CPUEntry>& cpus) {
        return parse_passmark_cpu(data, cpus);
    };
    sources_.push_back(passmark_cpu);
    
    // PassMark GPU benchmark source (example)
    DataSource passmark_gpu;
    passmark_gpu.name = "PassMark GPU";
    passmark_gpu.url = "https://example.com/passmark_gpu.txt"; // Placeholder URL
    passmark_gpu.type = "txt";
    passmark_gpu.enabled = true;
    passmark_gpu.priority = 1; // High priority
    passmark_gpu.update_interval = std::chrono::hours(168); // Weekly
    passmark_gpu.gpu_parser = [this](const String& data, std::vector<GPUEntry>& gpus) {
        return passmark_gpu_parser(data, gpus);
    };
    sources_.push_back(passmark_gpu);
    
    // TechPowerUp GPU Database (example)
    DataSource techpowerup_gpu;
    techpowerup_gpu.name = "TechPowerUp GPU";
    techpowerup_gpu.url = "https://example.com/techpowerup_gpu.html"; // Placeholder URL
    techpowerup_gpu.type = "html";
    techpowerup_gpu.enabled = true;
    techpowerup_gpu.priority = 2; // Medium priority
    techpowerup_gpu.update_interval = std::chrono::hours(336); // Bi-weekly
    techpowerup_gpu.gpu_parser = [this](const String& data, std::vector<GPUEntry>& gpus) {
        return parse_techpowerup_gpu(data, gpus);
    };
    sources_.push_back(techpowerup_gpu);
    
    // UserBenchmark CPU source (example)
    DataSource userbenchmark_cpu;
    userbenchmark_cpu.name = "UserBenchmark CPU";
    userbenchmark_cpu.url = "https://example.com/userbenchmark_cpu.txt"; // Placeholder URL
    userbenchmark_cpu.type = "txt";
    userbenchmark_cpu.enabled = true;
    userbenchmark_cpu.priority = 3; // Lower priority (less reliable)
    userbenchmark_cpu.update_interval = std::chrono::hours(168); // Weekly
    userbenchmark_cpu.cpu_parser = [this](const String& data, std::vector<CPUEntry>& cpus) {
        return parse_userbenchmark_cpu(data, cpus);
    };
    sources_.push_back(userbenchmark_cpu);
    
    // Add some fallback embedded data sources
    DataSource embedded_cpu;
    embedded_cpu.name = "Embedded CPU Data";
    embedded_cpu.type = "embedded";
    embedded_cpu.enabled = true;
    embedded_cpu.priority = 10; // Lowest priority - fallback only
    embedded_cpu.cpu_parser = [this](const String& data, std::vector<CPUEntry>& cpus) {
        // For embedded data, we ignore the input data and return our built-in data
        cpus = get_embedded_cpu_data();
        return success();
    };
    sources_.push_back(embedded_cpu);
    
    DataSource embedded_gpu;
    embedded_gpu.name = "Embedded GPU Data";
    embedded_gpu.type = "embedded";
    embedded_gpu.enabled = true;
    embedded_gpu.priority = 10; // Lowest priority - fallback only
    embedded_gpu.gpu_parser = [this](const String& data, std::vector<GPUEntry>& gpus) {
        // For embedded data, we ignore the input data and return our built-in data
        gpus = get_embedded_gpu_data();
        return success();
    };
    sources_.push_back(embedded_gpu);
    
    DataSource embedded_ram;
    embedded_ram.name = "Embedded RAM Data";
    embedded_ram.type = "embedded";
    embedded_ram.enabled = true;
    embedded_ram.priority = 10; // Lowest priority - fallback only
    embedded_ram.ram_parser = [this](const String& data, std::vector<RAMEntry>& ram) {
        // For embedded data, we ignore the input data and return our built-in data
        ram = get_embedded_ram_data();
        return success();
    };
    sources_.push_back(embedded_ram);
    
    DataSource embedded_storage;
    embedded_storage.name = "Embedded Storage Data";
    embedded_storage.type = "embedded";
    embedded_storage.enabled = true;
    embedded_storage.priority = 10; // Lowest priority - fallback only
    embedded_storage.storage_parser = [this](const String& data, std::vector<StorageEntry>& storage) {
        // For embedded data, we ignore the input data and return our built-in data
        storage = get_embedded_storage_data();
        return success();
    };
    sources_.push_back(embedded_storage);
    
    return success();
}

// Callback for progress reporting
void HardwareUpdater::set_progress_callback(std::function<void(f32 progress, StringView status)> callback) {
    progress_callback_ = callback;
}

// Fetching and parsing
Result<UpdateStats> HardwareUpdater::fetch_all_sources() {
    auto start_time = std::chrono::system_clock::now();
    UpdateStats stats;
    
    // Sort sources by priority (lower number = higher priority)
    std::vector<DataSource> sorted_sources;
    {
        std::shared_lock<std::shared_mutex> lock(sources_mutex_);
        sorted_sources = sources_;
    }
    
    std::sort(sorted_sources.begin(), sorted_sources.end(),
        [](const DataSource& a, const DataSource& b) {
            return a.priority < b.priority;
        });
    
    size_t total_sources = sorted_sources.size();
    size_t processed_sources = 0;
    
    for (auto& source : sorted_sources) {
        if (!source.enabled) {
            processed_sources++;
            continue;
        }
        
        // Update progress
        if (progress_callback_) {
            f32 progress = static_cast<f32>(processed_sources) / static_cast<f32>(total_sources);
            progress_callback_(progress, std::format("Processing {}...", source.name));
        }
        
        try {
            // Check if it's time to update this source
            auto now = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::hours>(now - source.last_fetch);
            
            if (source.enabled && (source.last_fetch == TimePoint::min() || elapsed >= source.update_interval)) {
                // Fetch data from source
                String data;
                Result<String> fetch_result;
                
                if (source.type == "embedded") {
                    // For embedded sources, we don't actually fetch - the parser handles it
                    data = "";
                    fetch_result = success(String{});
                } else {
                    // Fetch the data using curl
                    fetch_result = download_with_retry(source.url, 3);
                    if (!fetch_result) {
                        // Try to get more detailed error info
                        LOG_WARN("Failed to fetch from {}: {}", source.url, fetch_result.error().message);
                    }
                    data = *fetch_result;
                }
                
                if (fetch_result) {
                    // Parse the data based on type
                    bool parsed = false;
                    
                    if (source.type == "txt" || source.type == "csv") {
                        if (!source.cpu_parser) {
                            return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, 
                                std::format("No CPU parser defined for source: {}", source.name)));
                        }
                        auto parse_result = source.cpu_parser(data, /*output*/ std::vector<CPUEntry>{});
                        if (!parse_result) {
                            // Try GPU parser if CPU parser failed
                            if (source.gpu_parser) {
                                auto gpu_result = source.gpu_parser(data, /*output*/ std::vector<GPUEntry>{});
                                if (!gpu_result) {
                                    // Try RAM parser
                                    if (source.ram_parser) {
                                        auto ram_result = source.ram_parser(data, /*output*/ std::vector<RAMEntry>{});
                                        if (!ram_result) {
                                            // Try storage parser
                                            if (source.storage_parser) {
                                                auto storage_result = source.storage_parser(data, /*output*/ std::vector<StorageEntry>{});
                                                if (!storage_result) {
                                                    return make_unexpected(MAKE_ERROR(ErrorCode::ParseError, 
                                                        std::format("All parsers failed for source: {}", source.name)));
                                                } else {
                                                    merge_storage_data(*storage_result, stats);
                                                    parsed = true;
                                                }
                                            } else {
                                                return make_unexpected(MAKE_ERROR(ErrorCode::ParseError, 
                                                    std::format("No suitable parser found for source: {}", source.name)));
                                            }
                                        } else {
                                            merge_ram_data(*ram_result, stats);
                                            parsed = true;
                                        }
                                    } else {
                                        merge_gpu_data(*gpu_result, stats);
                                        parsed = true;
                                    }
                                } else {
                                    merge_cpu_data(*parse_result, stats);
                                    parsed = true;
                                }
                            } else {
                                return make_unexpected(MAKE_ERROR(ErrorCode::ParseError, 
                                    std::format("No suitable parser found for source: {}", source.name)));
                            }
                        } else {
                            merge_cpu_data(*parse_result, stats);
                            parsed = true;
                        }
                    } else if (source.type == "html") {
                        // HTML parsing would be more complex - for now, we'll skip or use simplified parsing
                        if (source.gpu_parser) {
                            auto result = source.gpu_parser(data, /*output*/ std::vector<GPUEntry>{});
                            if (result) {
                                merge_gpu_data(*result, stats);
                                parsed = true;
                            } else {
                                LOG_WARN("HTML parsing failed for source: {}", source.name);
                            }
                        }
                    }
                    // Add other types as needed
                    
                    // Update last fetch time only if we got data or it's an embedded source
                    if (parsed || source.type == "embedded") {
                        source.last_fetch = std::chrono::system_clock::now();
                        source.consecutive_failures = 0;
                        stats.sources_succeeded++;
                    } else {
                        source.consecutive_failures++;
                        if (source.consecutive_failures >= 3) {
                            source.enabled = false; // Disable after too many failures
                            LOG_WARN("Disabling source {} due to repeated failures", source.name);
                        }
                        stats.sources_failed++;
                        stats.errors.push_back(std::format("Failed to parse data from {}", source.name));
                    }
                } else {
                    // Fetch failed
                    source.consecutive_failures++;
                    if (source.consecutive_failures >= 3) {
                        source.enabled = false; // Disable after too many failures
                        LOG_WARN("Disabling source {} due to repeated failures", source.name);
                    }
                    stats.sources_failed++;
                    stats.errors.push_back(fetch_result.error().message);
                }
            } else {
                // Not time to update yet
                stats.sources_succeeded++; // Count as success since we didn't need to update
            }
        } catch (const std::exception& e) {
            source.consecutive_failures++;
            if (source.consecutive_failures >= 3) {
                source.enabled = false; // Disable after too many failures
                LOG_WARN("Disabling source {} due to repeated failures", source.name);
            }
            stats.sources_failed++;
            stats.errors.push_back(std::format("Exception processing source {}: {}", source.name, e.what()));
        }
        
        processed_sources++;
    }
    
    // Final progress update
    if (progress_callback_) {
        progress_callback_(1.0f, "Update complete");
    }
    
    stats.end_time = std::chrono::system_clock::now();
    stats.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        stats.end_time - start_time);
        
    return stats;
}

Result<String> HardwareUpdater::download_with_retry(const String& url, u32 max_retries) {
    // Try to download the URL with retries
    for (u32 attempt = 0; attempt < max_retries; ++attempt) {
        Result<String> result = fetch_url(url, "");
        if (result) {
            return result;
        }
        
        // Wait before retrying (exponential backoff)
        if (attempt < max_retries - 1) {
            std::this_thread::sleep_for(std::chrono::seconds(1 << attempt));
        }
    }
    
    // All attempts failed
    return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
        std::format("Failed to download {} after {} attempts", url, max_retries)));
}

// Simplified fetch_url for demonstration - in reality would use libcurl
String HardwareUpdater::fetch_url(const String& url, const String& api_key) {
    CURL* curl = curl_easy_init();
    std::string readBuffer;
    long http_code = 0;
    
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &http_code);
        
        // Set timeout
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        
        // Set user agent
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "GameReqAnalyzer/1.0");
        
        // Follow redirects
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        
        // Set API key if provided (as header)
        if (!api_key.empty()) {
            struct curl_slist* headers = nullptr;
            std::string auth_header = "Authorization: Bearer " + api_key;
            headers = curl_slist_append(headers, auth_header.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
        
        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        
        // Clean up
        if (!api_key.empty()) {
            // We would need to track the headers list to free it properly
            // For simplicity, we're leaking it here - in production code, track and free it
        }
        
        curl_easy_cleanup(curl);
        
        if (res == CURLE_OK && http_code == 200) {
            return readBuffer;
        } else {
            LOG_WARN("HTTP request failed: URL={}, Code={}, Error={}", url, http_code, curl_easy_strerror(res));
            return "";
        }
    } else {
        LOG_WARN("Failed to initialize CURL");
        return "";
    }
}

// Parsers for different sources
Result<std::vector<CPUEntry>> HardwareUpdater::parse_passmark_cpu(const String& data, std::vector<CPUEntry>& cpus) {
    // Parse PassMark CPU data format (example format)
    // In reality, PassMark data would be in a specific format
    // For now, we'll return empty to indicate we need actual implementation
    
    LOG_WARN("PassMark CPU parser not fully implemented - returning empty result");
    return std::vector<CPUEntry>{};
}

Result<std::vector<GPUEntry>> HardwareUpdater::passmark_gpu_parser(const String& data, std::vector<GPUEntry>& gpus) {
    // Parse PassMark GPU data format
    LOG_WARN("PassMark GPU parser not fully implemented - returning empty result");
    return std::vector<GPUEntry>{};
}

Result<std::vector<CPUEntry>> HardwareUpdater::parse_geekbench_cpu(const String& json, std::vector<CPUEntry>& cpus) {
    // Parse Geekbench JSON data
    LOG_WARN("Geekbench CPU parser not fully implemented - returning empty result");
    return std::vector<CPUEntry>{};
}

Result<std::vector<GPUEntry>> HardwareUpdater::parse_geekbench_gpu(const String& json, std::vector<GPUEntry>& gpus) {
    // Parse Geekbench JSON data for GPU
    LOG_WARN("Geekbench GPU parser not fully implemented - returning empty result");
    return std::vector<GPUEntry>{};
}

Result<std::vector<CPUEntry>> HardwareUpdater::parse_techpowerup_cpu(const String& html, std::vector<CPUEntry>& cpus) {
    // Parse TechPowerUp HTML data for CPU
    LOG_WARN("TechPowerUp CPU parser not fully implemented - returning empty result");
    return std::vector<CPUEntry>{};
}

Result<std::vector<GPUEntry>> HardwareUpdater::parse_techpowerup_gpu(const String& html, std::vector<GPUEntry>& gpus) {
    // Parse TechPowerUp HTML data for GPU
    LOG_WARN("TechPowerUp GPU parser not fully implemented - returning empty result");
    return std::vector<GPUEntry>{};
}

Result<std::vector<CPUEntry>> HardwareUpdater::parse_cpubenchmark_net(const String& html, std::vector<CPUEntry>& cpus) {
    // Parse CPU Benchmark.net data
    LOG_WARN("CPU Benchmark.net parser not fully implemented - returning empty result");
    return std::vector<CPUEntry>{};
}

Result<std::vector<GPUEntry>> HardwareUpdater::parse_gpubenchmark_net(const String& html, std::vector<GPUEntry>& gpus) {
    // Parse GPU Benchmark.net data
    LOG_WARN("GPU Benchmark.net parser not fully implemented - returning empty result");
    return std::vector<GPUEntry>{};
}

Result<std::vector<RAMEntry>> HardwareUpdater::parse_ram_data(const String& json, std::vector<RAMEntry>& ram) {
    // Parse RAM data (JSON format expected)
    LOG_WARN("RAM parser not fully implemented - returning empty result");
    return std::vector<RAMEntry>{};
}

Result<std::vector<StorageEntry>> HardwareUpdater::parse_storage_data(const String& json, std::vector<StorageEntry>& storage) {
    // Parse storage data (JSON format expected)
    LOG_WARN("Storage parser not fully implemented - returning empty result");
    return std::vector<StorageEntry>{};
}

// Static embedded data (fallback/initial data)
std::vector<CPUEntry> HardwareUpdater::get_embedded_cpu_data() const {
    // Return the same data as in the hardware_database.cpp load_default_data function
    // This ensures we have a baseline even if online updates fail
    
    std::vector<CPUEntry> cpus;
    cpus.push_back(CPUEntry{
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
    
    cpus.push_back(CPUEntry{
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
    
    // Add a few more for variety
    cpus.push_back(CPUEntry{
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
    
    return cpus;
}

std::vector<GPUEntry> HardwareUpdater::get_embedded_gpu_data() const {
    // Return the same data as in the hardware_database.cpp load_default_data function
    std::vector<GPUEntry> gpus;
    gpus.push_back(GPUEntry{
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
    
    gpus.push_back(GPUEntry{
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
    
    return gpus;
}

std::vector<RAMEntry> HardwareUpdater::get_embedded_ram_data() const {
    // Return the same data as in the hardware_database.cpp load_default_data function
    std::vector<RAMEntry> ram;
    ram.push_back(RAMEntry{
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
    
    ram.push_back(RAMEntry{
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
    
    return ram;
}

std::vector<StorageEntry> HardwareUpdater::get_embedded_storage_data() const {
    // Return the same data as in the hardware_database.cpp load_default_data function
    std::vector<StorageEntry> storage;
    storage.push_back(StorageEntry{
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
    
    storage.push_back(StorageEntry{
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
    
    return storage;
}

// Merging and deduplication
void HardwareUpdater::merge_cpu_data(const std::vector<CPUEntry>& new_data, UpdateStats& stats) {
    std::unique_lock<std::shared_mutex> lock(db_.mutex_);
    
    // Create a set of existing CPU names for quick lookup
    std::set<String> existing_names;
    for (const auto& cpu : db_.cpus_) {
        existing_names.insert(cpu.name);
    }
    
    // Process new data
    for (const auto& cpu : new_data) {
        if (existing_names.find(cpu.name) == existing_names.end()) {
            // New CPU
            db_.cpus_.push_back(cpu);
            stats.cpus_added++;
            existing_names.insert(cpu.name);
        } else {
            // Existing CPU - check if we need to update
            auto it = std::find_if(db_.cpus_.begin(), db_.cpus_.end(),
                [&cpu](const CPUEntry& existing) { return existing.name == cpu.name; });
            
            if (it != db_.cpus_.end()) {
                // Check if data is significantly different
                if (is_significant_update(*it, cpu)) {
                    *it = cpu;
                    stats.cpus_updated++;
                }
            }
        }
    }
    
    // Remove CPUs that are no longer in the new data (optional - usually we keep historical data)
    // For now, we don't remove old entries to maintain historical data
}

void HardwareUpdater::merge_gpu_data(const std::vector<GPUEntry>& new_data, UpdateStats& stats) {
    std::unique_lock<std::shared_mutex> lock(db_.mutex_);
    
    // Create a set of existing GPU names for quick lookup
    std::set<String> existing_names;
    for (const auto& gpu : db_.gpus_) {
        existing_names.insert(gpu.name);
    }
    
    // Process new data
    for (const auto& gpu : new_data) {
        if (existing_names.find(gpu.name) == existing_names.end()) {
            // New GPU
            db_.gpus_.push_back(gpu);
            stats.gpus_added++;
            existing_names.insert(gpu.name);
        } else {
            // Existing GPU - check if we need to update
            auto it = std::find_if(db_.gpus_.begin(), db_.gpus_.end(),
                [&gpu](const GPUEntry& existing) { return existing.name == gpu.name; });
            
            if (it != db_.gpus_.end()) {
                // Check if data is significantly different
                if (is_significant_update(*it, gpu)) {
                    *it = gpu;
                    stats.gpus_updated++;
                }
            }
        }
    }
    
    // Remove GPUs that are no longer in the new data (optional)
}

void HardwareUpdater::merge_ram_data(const std::vector<RAMEntry>& new_data, UpdateStats& stats) {
    std::unique_lock<std::shared_mutex> lock(db_.mutex_);
    
    // Create a set of existing RAM names for quick lookup
    std::set<String> existing_names;
    for (const auto& ram : db_.ram_) {
        existing_names.insert(ram.name);
    }
    
    // Process new data
    for (const auto& ram : new_data) {
        if (existing_names.find(ram.name) == existing_names.end()) {
            // New RAM
            db_.ram_.push_back(ram);
            stats.ram_added++;
            existing_names.insert(ram.name);
        } else {
            // Existing RAM - check if we need to update
            auto it = std::find_if(db_.ram_.begin(), db_.ram_.end(),
                [&ram](const RAMEntry& existing) { return existing.name == ram.name; });
            
            if (it != db_.ram_.end()) {
                // Check if data is significantly different
                if (is_significant_update(*it, ram)) {
                    *it = ram;
                    stats.ram_updated++;
                }
            }
        }
    }
}

void HardwareUpdater::merge_storage_data(const std::vector<StorageEntry>& new_data, UpdateStats& stats) {
    std::unique_lock<std::shared_mutex> lock(db_.mutex_);
    
    // Create a set of existing storage names for quick lookup
    std::set<String> existing_names;
    for (const auto& storage : db_.storage_) {
        existing_names.insert(storage.name);
    }
    
    // Process new data
    for (const auto& storage : new_data) {
        if (existing_names.find(storage.name) == existing_names.end()) {
            // New storage
            db_.storage_.push_back(storage);
            stats.storage_added++;
            existing_names.insert(storage.name);
        } else {
            // Existing storage - check if we need to update
            auto it = std::find_if(db_.storage_.begin(), db_.storage_.end(),
                [&storage](const StorageEntry& existing) { return existing.name == storage.name; });
            
            if (it != db_.storage_.end()) {
                // Check if data is significantly different
                if (is_significant_update(*it, storage)) {
                    *it = storage;
                    stats.storage_updated++;
                }
            }
        }
    }
}

// Comparison helpers
bool HardwareUpdater::is_new_hardware(const CPUEntry& entry) const {
    std::shared_lock<std::shared_mutex> lock(db_.mutex_);
    return std::none_of(db_.cpus_.begin(), db_.cpus_.end(),
        [&entry](const CPUItem& existing) { return existing.name == entry.name; });
}

bool HardwareUpdater::is_new_hardware(const GPUEntry& entry) const {
    std::shared_lock<std::shared_mutex> lock(db_.mutex_);
    return std::none_of(db_.gpus_.begin(), db_.gpus_.end(),
        [&entry](const GPUItem& existing) { return existing.name == entry.name; });
}

bool HardwareUpdater::is_significant_update(const CPUEntry& old_entry, const CPUEntry& new_entry) const {
    // Consider it significant if any of these metrics changed significantly
    const int score_threshold = 100; // 100 point difference in PassMark score
    const float price_threshold = 20.0f; // $20 price difference
    const int year_threshold = 1; // 1 year difference
    
    bool score_changed = std::abs(static_cast<int>(new_entry.passmark_st) - 
                                 static_cast<int>(old_entry.passmark_st)) > score_threshold ||
                         std::abs(static_cast<int>(new_entry.passmark_mt) - 
                                 static_cast<int>(old_entry.passmark_mt)) > score_threshold;
    
    bool price_changed = std::abs(new_entry.price_usd - old_entry.price_usd) > price_threshold;
    
    bool year_changed = std::abs(new_entry.release_year - old_entry.release_year) > year_threshold;
    
    return score_changed || price_changed || year_changed;
}

bool HardwareUpdater::is_significant_update(const GPUEntry& old_entry, const GPUEntry& new_entry) const {
    // Consider it significant if any of these metrics changed significantly
    const int score_threshold = 100; // 100 point difference in PassMark score
    const int vram_threshold = 1024; // 1GB VRAM difference
    const float price_threshold = 20.0f; // $20 price difference
    const int year_threshold = 1; // 1 year difference
    
    bool score_changed = std::abs(static_cast<int>(new_entry.passmark_g3d) - 
                                 static_cast<int>(old_entry.passmark_g3d)) > score_threshold;
    
    bool vram_changed = std::abs(static_cast<int>(new_entry.vram) - 
                                static_cast<int>(old_entry.vram)) > vram_threshold;
    
    bool price_changed = std::abs(new_entry.price_usd - old_entry.price_usd) > price_threshold;
    
    bool year_changed = std::abs(new_entry.release_year - old_entry.release_year) > year_threshold;
    
    return score_changed || vram_changed || price_changed || year_changed;
}

// Signature verification for supply chain security
bool HardwareUpdater::verify_signature(const String& data, const String& signature, const String& public_key) const {
    // In a real implementation, we would use a cryptographic library like OpenSSL
    // to verify the signature. For now, we'll return true to indicate that
    // signature verification would be implemented in production.
    LOG_WARN("Signature verification not implemented - would need cryptographic library");
    return true; // Placeholder - in reality would verify the signature
}

} // namespace game_req