#include <game_req_analyzer/utils/platform_utils.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <thread>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <versionhelpers.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#else
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <fstream>
#endif

namespace game_req {

String PlatformUtils::os_name() {
#ifdef _WIN32
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#else
    return "Linux";
#endif
}

String PlatformUtils::os_version() {
#ifdef _WIN32
    OSVERSIONINFOEXW osvi = { sizeof(osvi) };
    if (GetVersionExW(reinterpret_cast<OSVERSIONINFOW*>(&osvi))) {
        return std::to_string(osvi.dwMajorVersion) + "." + std::to_string(osvi.dwMinorVersion) + "." + std::to_string(osvi.dwBuildNumber);
    }
    return "Unknown";
#elif defined(__APPLE__)
    size_t size = 0;
    sysctlbyname("kern.osrelease", nullptr, &size, nullptr, 0);
    String version(size, '\0');
    sysctlbyname("kern.osrelease", version.data(), &size, nullptr, 0);
    return version;
#else
    std::ifstream file("/etc/os-release");
    if (file) {
        String line;
        while (std::getline(file, line)) {
            if (line.find("VERSION_ID=") == 0) {
                return line.substr(11);
            }
        }
    }
    struct utsname uts;
    if (uname(&uts) == 0) {
        return uts.release;
    }
    return "Unknown";
#endif
}

String PlatformUtils::kernel_version() {
    struct utsname uts;
    if (uname(&uts) == 0) {
        return uts.release;
    }
    return "Unknown";
}

String PlatformUtils::architecture() {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: return "x86_64";
        case PROCESSOR_ARCHITECTURE_ARM64: return "arm64";
        case PROCESSOR_ARCHITECTURE_IA64: return "ia64";
        default: return "Unknown";
    }
#else
    struct utsname uts;
    if (uname(&uts) == 0) {
        return uts.machine;
    }
    return "Unknown";
#endif
}

u32 PlatformUtils::cpu_core_count() {
    return std::thread::hardware_concurrency();
}

u32 PlatformUtils::cpu_thread_count() {
    return std::thread::hardware_concurrency();
}

u64 PlatformUtils::total_ram() {
#ifdef _WIN32
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return statex.ullTotalPhys;
#elif defined(__APPLE__)
    int64_t memsize = 0;
    size_t size = sizeof(memsize);
    sysctlbyname("hw.memsize", &memsize, &size, nullptr, 0);
    return memsize;
#else
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return info.totalram * info.mem_unit;
    }
    return 0;
#endif
}

u64 PlatformUtils::available_ram() {
#ifdef _WIN32
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return statex.ullAvailPhys;
#elif defined(__APPLE__)
    vm_statistics64_data_t vmstat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vmstat), &count) == KERN_SUCCESS) {
        return static_cast<u64>(vmstat.free_count) * 4096;
    }
    return 0;
#else
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return info.freeram * info.mem_unit;
    }
    return 0;
#endif
}

u64 PlatformUtils::page_size() {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
#else
    return sysconf(_SC_PAGESIZE);
#endif
}

Result<Path> PlatformUtils::executable_path() {
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return Path(buffer);
#elif defined(__APPLE__)
    char buffer[PATH_MAX];
    uint32_t size = PATH_MAX;
    if (_NSGetExecutablePath(buffer, &size) == 0) {
        return Path(buffer);
    }
    return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Buffer too small"));
#else
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return Path(buffer);
    }
    return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to read executable path"));
#endif
}

Result<Path> PlatformUtils::config_dir() {
#ifdef _WIN32
    wchar_t* path = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path) == S_OK) {
        Path p(path);
        CoTaskMemFree(path);
        return p / "GameReqAnalyzer";
    }
#elif defined(__APPLE__)
    char* home = getenv("HOME");
    if (home) {
        return Path(home) / "Library" / "Application Support" / "GameReqAnalyzer";
    }
#else
    const char* xdg = getenv("XDG_CONFIG_HOME");
    if (xdg) {
        return Path(xdg) / "game-req-analyzer";
    }
    char* home = getenv("HOME");
    if (home) {
        return Path(home) / ".config" / "game-req-analyzer";
    }
#endif
    return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Could not determine config directory"));
}

Result<Path> PlatformUtils::cache_dir() {
#ifdef _WIN32
    wchar_t* path = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path) == S_OK) {
        Path p(path);
        CoTaskMemFree(path);
        return p / "GameReqAnalyzer" / "Cache";
    }
#elif defined(__APPLE__)
    char* home = getenv("HOME");
    if (home) {
        return Path(home) / "Library" / "Caches" / "GameReqAnalyzer";
    }
#else
    const char* xdg = getenv("XDG_CACHE_HOME");
    if (xdg) {
        return Path(xdg) / "game-req-analyzer";
    }
    char* home = getenv("HOME");
    if (home) {
        return Path(home) / ".cache" / "game-req-analyzer";
    }
#endif
    return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Could not determine cache directory"));
}

Result<Path> PlatformUtils::data_dir() {
#ifdef _WIN32
    wchar_t* path = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path) == S_OK) {
        Path p(path);
        CoTaskMemFree(path);
        return p / "GameReqAnalyzer";
    }
#elif defined(__APPLE__)
    char* home = getenv("HOME");
    if (home) {
        return Path(home) / "Library" / "Application Support" / "GameReqAnalyzer";
    }
#else
    const char* xdg = getenv("XDG_DATA_HOME");
    if (xdg) {
        return Path(xdg) / "game-req-analyzer";
    }
    char* home = getenv("HOME");
    if (home) {
        return Path(home) / ".local" / "share" / "game-req-analyzer";
    }
#endif
    return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Could not determine data directory"));
}

Result<Path> PlatformUtils::temp_dir() {
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    GetTempPathW(MAX_PATH, buffer);
    return Path(buffer);
#else
    const char* tmp = getenv("TMPDIR");
    if (tmp) return Path(tmp);
    return Path("/tmp");
#endif
}

Result<Path> PlatformUtils::home_dir() {
    char* home = getenv("HOME");
    if (home) return Path(home);
#ifdef _WIN32
    char* userprofile = getenv("USERPROFILE");
    if (userprofile) return Path(userprofile);
#endif
    return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Could not determine home directory"));
}

Result<void> PlatformUtils::make_directory(const Path& path) {
    std::error_code ec;
    fs::create_directories(path, ec);
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return success();
}

Result<void> PlatformUtils::remove_directory(const Path& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return success();
}

Result<void> PlatformUtils::copy_file(const Path& src, const Path& dst, bool overwrite) {
    std::error_code ec;
    fs::copy_file(src, dst, overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none, ec);
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return success();
}

Result<void> PlatformUtils::move_file(const Path& src, const Path& dst) {
    std::error_code ec;
    fs::rename(src, dst, ec);
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return success();
}

Result<u64> PlatformUtils::file_size(const Path& path) {
    std::error_code ec;
    u64 size = fs::file_size(path, ec);
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return size;
}

Result<TimePoint> PlatformUtils::file_modified_time(const Path& path) {
    std::error_code ec;
    auto ftime = fs::last_write_time(path, ec);
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return ftime;
}

Result<TimePoint> PlatformUtils::file_created_time(const Path& path) {
    std::error_code ec;
    auto ftime = fs::last_write_time(path, ec); // Fallback
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return ftime;
}

Result<u32> PlatformUtils::file_permissions(const Path& path) {
    std::error_code ec;
    auto perms = fs::status(path, ec).permissions();
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return static_cast<u32>(perms & fs::perms::mask);
}

bool PlatformUtils::is_admin() {
#ifdef _WIN32
    BOOL isAdmin = FALSE;
    PSID administratorsGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &administratorsGroup)) {
        CheckTokenMembership(nullptr, administratorsGroup, &isAdmin);
        FreeSid(administratorsGroup);
    }
    return isAdmin;
#else
    return geteuid() == 0;
#endif
}

bool PlatformUtils::is_windows() {
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

bool PlatformUtils::is_linux() {
#if defined(__linux__)
    return true;
#else
    return false;
#endif
}

bool PlatformUtils::is_macos() {
#ifdef __APPLE__
    return true;
#else
    return false;
#endif
}

bool PlatformUtils::is_bsd() {
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
    return true;
#else
    return false;
#endif
}

Result<String> PlatformUtils::read_env(StringView name) {
    const char* value = std::getenv(name.data());
    if (value) return String(value);
    return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Environment variable not found"));
}

Result<void> PlatformUtils::write_env(StringView name, StringView value) {
#ifdef _WIN32
    return SetEnvironmentVariableW(String(name).c_str(), String(value).c_str()) ? success() : make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to set environment variable"));
#else
    return setenv(name.data(), value.data(), 1) == 0 ? success() : make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to set environment variable"));
#endif
}

Result<void> PlatformUtils::sleep(Duration d) {
    std::this_thread::sleep_for(d);
    return success();
}

Result<u64> PlatformUtils::current_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

Result<u64> PlatformUtils::current_timestamp_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

Result<void> PlatformUtils::open_url(StringView url) {
#ifdef _WIN32
    ShellExecuteW(nullptr, L"open", String(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return success();
#elif defined(__APPLE__)
    return system(("open " + String(url)).c_str()) == 0 ? success() : make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to open URL"));
#else
    return system(("xdg-open " + String(url)).c_str()) == 0 ? success() : make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to open URL"));
#endif
}

Result<void> PlatformUtils::open_folder(const Path& path) {
#ifdef _WIN32
    ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return success();
#elif defined(__APPLE__)
    return system(("open " + path.string()).c_str()) == 0 ? success() : make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to open folder"));
#else
    return system(("xdg-open " + path.string()).c_str()) == 0 ? success() : make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to open folder"));
#endif
}

PlatformUtils::CPUInfo PlatformUtils::cpu_info() {
    CPUInfo info;
    info.vendor = "Unknown";
    info.brand = "Unknown";
    info.architecture = architecture();
    info.cores = cpu_core_count();
    info.threads = cpu_thread_count();
    return info;
}

std::vector<PlatformUtils::GPUInfo> PlatformUtils::gpu_info() {
    return {};
}

std::vector<PlatformUtils::DiskInfo> PlatformUtils::disk_info() {
    std::vector<DiskInfo> disks;
#ifdef _WIN32
    DWORD drives = GetLogicalDrives();
    for (char drive = 'A'; drive <= 'Z'; ++drive) {
        if (drives & (1 << (drive - 'A'))) {
            DiskInfo info;
            info.mount_point = Path(String(1, drive) + ":\\");
            info.total = 0; // Would need GetDiskFreeSpaceEx
            info.free = 0;
            disks.push_back(info);
        }
    }
#else
    // Simplified - would need to parse /proc/mounts or use statvfs
#endif
    return disks;
}

} // namespace game_req
