#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>

namespace game_req {

struct ScanStats {
    std::atomic<u64> files_scanned{0};
    std::atomic<u64> total_size{0};
    std::atomic<u64> archives_found{0};
    std::atomic<u64> archives_extracted{0};
    std::atomic<u64> errors{0};
    std::atomic<u32> current_depth{0};
    TimePoint start_time;
    TimePoint end_time;
    
    [[nodiscard]] Duration elapsed() const {
        return end_time - start_time;
    }
    [[nodiscard]] f64 files_per_second() const {
        auto dur = elapsed();
        if (dur.count() == 0) return 0.0;
        return files_scanned / (dur.count() / 1e9);
    }
};

struct ScanResult {
    std::vector<FileInfo> files;
    ScanStats stats;
    std::vector<Error> errors;
    Path root_path;
    
    [[nodiscard]] u64 total_size() const { return stats.total_size.load(); }
    [[nodiscard]] u64 file_count() const { return stats.files_scanned.load(); }
    
    template<typename Pred>
    [[nodiscard]] std::vector<FileInfo> filter(Pred&& pred) const {
        std::vector<FileInfo> result;
        for (const auto& f : files) {
            if (pred(f)) result.push_back(f);
        }
        return result;
    }
    
    [[nodiscard]] std::vector<FileInfo> by_category(FileCategory cat) const {
        return filter([cat](const FileInfo& f) { return f.category == cat; });
    }
    
    [[nodiscard]] std::vector<FileInfo> by_type(FileType type) const {
        return filter([type](const FileInfo& f) { return f.type == type; });
    }
};

class ArchiveHandler {
public:
    virtual ~ArchiveHandler() = default;
    
    [[nodiscard]] virtual bool can_handle(FileType type) const = 0;
    [[nodiscard]] virtual Result<std::vector<FileInfo>> extract(const Path& archive_path, const Path& dest_dir) = 0;
    [[nodiscard]] virtual Result<std::vector<FileInfo>> list_contents(const Path& archive_path) = 0;
    [[nodiscard]] virtual String name() const = 0;
    [[nodiscard]] virtual std::vector<FileType> supported_types() const = 0;
};

class ArchiveManager {
public:
    static ArchiveManager& instance();
    
    void register_handler(std::unique_ptr<ArchiveHandler> handler);
    [[nodiscard]] Result<std::vector<FileInfo>> extract(const Path& archive_path, const Path& dest_dir);
    [[nodiscard]] Result<std::vector<FileInfo>> list_contents(const Path& archive_path);
    [[nodiscard]] bool is_archive(FileType type) const;
    [[nodiscard]] std::vector<String> supported_extensions() const;

private:
    ArchiveManager() = default;
    std::unordered_map<FileType, std::unique_ptr<ArchiveHandler>> handlers_;
    mutable std::shared_mutex mutex_;
};

class FileScanner {
public:
    explicit FileScanner(const ScanConfig& config);
    
    Result<ScanResult> scan(const Path& root_path);
    Result<ScanResult> scan_async(const Path& root_path, std::function<void(f32 progress)> callback = {});
    
    void cancel();
    [[nodiscard]] bool is_cancelled() const;
    [[nodiscard]] const ScanStats& stats() const { return stats_; }

private:
    ScanConfig config_;
    ScanStats stats_;
    std::atomic<bool> cancelled_{false};
    std::mutex results_mutex_;
    ScanResult results_;
    
    void scan_directory(const Path& dir, u32 depth);
    Result<void> scan_file(const Path& file);
    Result<void> process_archive(const FileInfo& archive);
    void update_stats(const FileInfo& file);
};

} // namespace game_req