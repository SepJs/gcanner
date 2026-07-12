#pragma once
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/core/error.hpp>

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>

namespace game_req {

class PlatformUtils {
public:
    static String os_name();
    static String os_version();
    static String kernel_version();
    static String architecture();
    static u32 cpu_core_count();
    static u32 cpu_thread_count();
    static u64 total_ram();
    static u64 available_ram();
    static u64 page_size();
    
    static Result<Path> executable_path();
    static Result<Path> config_dir();
    static Result<Path> cache_dir();
    static Result<Path> data_dir();
    static Result<Path> temp_dir();
    static Result<Path> home_dir();
    
    static Result<void> make_directory(const Path& path);
    static Result<void> remove_directory(const Path& path);
    static Result<void> copy_file(const Path& src, const Path& dst, bool overwrite = false);
    static Result<void> move_file(const Path& src, const Path& dst);
    static Result<u64> file_size(const Path& path);
    static Result<TimePoint> file_modified_time(const Path& path);
    static Result<TimePoint> file_created_time(const Path& path);
    static Result<u32> file_permissions(const Path& path);
    
    static bool is_admin();
    static bool is_windows();
    static bool is_linux();
    static bool is_macos();
    static bool is_bsd();
    
    static Result<String> read_env(StringView name);
    static Result<void> write_env(StringView name, StringView value);
    
    static Result<void> sleep(Duration d);
    static Result<u64> current_timestamp_ms();
    static Result<u64> current_timestamp_ns();
    
    static Result<void> open_url(StringView url);
    static Result<void> open_folder(const Path& path);
    
    struct CPUInfo {
        String vendor;
        String brand;
        String architecture;
        u32 cores = 0;
        u32 threads = 0;
        f32 base_freq = 0.0f;
        f32 max_freq = 0.0f;
        u64 l1_cache = 0;
        u64 l2_cache = 0;
        u64 l3_cache = 0;
        std::vector<String> features;
    };
    static Result<CPUInfo> cpu_info();
    
    struct GPUInfo {
        String vendor;
        String renderer;
        String version;
        u64 vram = 0;
        String driver_version;
    };
    static Result<std::vector<GPUInfo>> gpu_info();
    
    struct DiskInfo {
        Path mount_point;
        String filesystem;
        u64 total = 0;
        u64 free = 0;
        u64 available = 0;
        bool is_ssd = false;
        bool is_removable = false;
    };
    static Result<std::vector<DiskInfo>> disk_info();
};

} // namespace game_req