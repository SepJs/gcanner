#include <game_req_analyzer/scanner/file_scanner.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/utils/hash_utils.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/utils/platform_utils.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <future>

using namespace game_req;

namespace {
    // Simple archive handlers for common formats
    class ZipHandler : public ArchiveHandler {
    public:
        bool can_handle(FileType type) const override {
            return type == FileType::ZIP;
        }
        
        Result<std::vector<FileInfo>> extract(const Path& archive_path, const Path& dest_dir) override {
            LOG_WARN("ZIP extraction not fully implemented");
            return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "ZIP extraction not implemented"));
        }
        
        Result<std::vector<FileInfo>> list_contents(const Path& archive_path) override {
            LOG_WARN("ZIP listing not fully implemented");
            return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "ZIP listing not implemented"));
        }
        
        String name() const override { return "ZIP Handler"; }
        std::vector<FileType> supported_types() const override { return {FileType::ZIP}; }
    };
    
    class TarHandler : public ArchiveHandler {
    public:
        bool can_handle(FileType type) const override {
            return type == FileType::TAR || type == FileType::GZ || 
                   type == FileType::BZ2 || type == FileType::XZ;
        }
        
        Result<std::vector<FileInfo>> extract(const Path& archive_path, const Path& dest_dir) override {
            LOG_WARN("TAR extraction not fully implemented");
            return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "TAR extraction not implemented"));
        }
        
        Result<std::vector<FileInfo>> list_contents(const Path& archive_path) override {
            LOG_WARN("TAR listing not fully implemented");
            return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "TAR listing not implemented"));
        }
        
        String name() const override { return "TAR Handler"; }
        std::vector<FileType> supported_types() const override {
            return {FileType::TAR, FileType::GZ, FileType::BZ2, FileType::XZ};
        }
    };
}

FileScanner::FileScanner(const ScanConfig& config) : config_(config) {
    // Register default archive handlers
    ArchiveManager::instance().register_handler(std::make_unique<ZipHandler>());
    ArchiveManager::instance().register_handler(std::make_unique<TarHandler>());
}

Result<ScanResult> FileScanner::scan(const Path& root_path) {
    if (!fs::exists(root_path)) {
        return make_unexpected(MAKE_ERROR(ErrorCode::FileNotFound, 
            std::format("Root path does not exist: {}", root_path.string())));
    }
    
    if (!fs::is_directory(root_path)) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, 
            std::format("Path is not a directory: {}", root_path.string())));
    }
    
    ScanResult result;
    result.root_path = root_path;
    result.stats.start_time = TimeUtils::now();
    
    try {
        scan_directory(root_path, 0);
        result.stats.end_time = TimeUtils::now();
        return result;
    } catch (const std::exception& e) {
        result.stats.end_time = TimeUtils::now();
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Scan failed: {}", e.what())));
    }
}

Result<ScanResult> FileScanner::scan_async(const Path& root_path, std::function<void(f32 progress)> callback) {
    if (!fs::exists(root_path)) {
        return make_unexpected(MAKE_ERROR(ErrorCode::FileNotFound, 
            std::format("Root path does not exist: {}", root_path.string())));
    }
    
    if (!fs::is_directory(root_path)) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, 
            std::format("Path is not a directory: {}", root_path.string())));
    }
    
    ScanResult result;
    result.root_path = root_path;
    result.stats.start_time = TimeUtils::now();
    
    try {
        // For simplicity, we'll do synchronous scanning but call the callback
        // In a full implementation, we'd use a thread pool
        scan_directory(root_path, 0);
        
        if (callback) {
            callback(1.0f); // 100% complete
        }
        
        result.stats.end_time = TimeUtils::now();
        return result;
    } catch (const std::exception& e) {
        result.stats.end_time = TimeUtils::now();
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Async scan failed: {}", e.what())));
    }
}

void FileScanner::cancel() {
    cancelled_.store(true);
}

bool FileScanner::is_cancelled() const {
    return cancelled_.load();
}

void FileScanner::scan_directory(const Path& dir, u32 depth) {
    if (cancelled_.load()) return;
    
    if (depth > config_.max_depth) {
        LOG_DEBUG("Max depth reached: {}", depth);
        return;
    }
    
    try {
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (cancelled_.load()) return;
            
            const auto& path = entry.path();
            
            // Skip if it's a symlink and we're not following symlinks
            if (fs::is_symlink(path) && !config_.follow_symlinks) {
                continue;
            }
            
            // Check if directory should be excluded
            if (fs::is_directory(path)) {
                String dir_name = path.filename().string();
                if (config_.excluded_dirs.find(dir_name) != config_.excluded_dirs.end()) {
                    LOG_DEBUG("Skipping excluded directory: {}", dir_name);
                    continue;
                }
                
                scan_directory(path, depth + 1);
            } else {
                // Process file
                auto file_result = scan_file(path);
                if (file_result) {
                    update_stats(*file_result);
                    {
                        std::lock_guard<std::mutex> lock(results_mutex_);
                        result_.files.push_back(*file_result);
                    }
                    
                    // Handle archives if enabled
                    if (file_result->is_archive && !config_.follow_symlinks) {
                        // In a full implementation, we'd extract and scan archives
                        // For now, just count them
                        result_.stats.archives_found++;
                    }
                } else {
                    std::lock_guard<std::mutex> lock(results_mutex_);
                    result_.errors.push_back(file_result.error());
                    result_.stats.errors++;
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        LOG_WARN("Filesystem error scanning {}: {}", dir.string(), e.what());
        std::lock_guard<std::mutex> lock(results_mutex_);
        result_.errors.push_back(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Filesystem error: {}", e.what())));
        result_.stats.errors++;
    } catch (const std::exception& e) {
        LOG_WARN("Error scanning {}: {}", dir.string(), e.what());
        std::lock_guard<std::mutex> lock(results_mutex_);
        result_.errors.push_back(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Error: {}", e.what())));
        result_.stats.errors++;
    }
}

Result<void> FileScanner::scan_file(const Path& file_path) {
    FileInfo info;
    
    try {
        // Detect file type
        auto detect_result = FileTypeDetector::instance().detect(file_path);
        if (!detect_result) {
            return make_unexpected(detect_result.error());
        }
        info = *detect_result;
        
        // Check file size limits
        if (info.size > config_.max_file_size) {
            return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, 
                std::format("File too large: {} > {}", info.size, config_.max_file_size)));
        }
        
        // Check if extension is excluded
        String ext = StringUtils::to_lower(fs::extension(file_path).string());
        if (!ext.empty() && config_.excluded_exts.find(ext) != config_.excluded_exts.end()) {
            return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, 
                std::format("Excluded extension: {}", ext)));
        }
        
        // Calculate hash if requested
        if (config_.verify_checksums && info.size < 100 * MiB) { // Only hash files < 100MB for performance
            auto hash_result = HashUtils::sha256(file_path);
            if (hash_result) {
                std::copy(hash_result->begin(), hash_result->end(), info.hash.begin());
                info.hash_valid = true;
            }
        }
        
        // Mark as archive if applicable
        info.is_archive = ArchiveManager::instance().is_archive(info.type);
        
        return info;
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("File scan failed: {}", e.what())));
    }
}

void FileScanner::update_stats(const FileInfo& file) {
    result_.stats.files_scanned++;
    result_.stats.total_size += file.size;
    
    // Update depth tracking
    u32 depth = 0;
    for (auto p = file.path.begin(); p != file.path.end(); ++p) {
        if (*p == fs::path::preferred_separator) {
            depth++;
        }
    }
    if (depth > result_.stats.current_depth.load()) {
        result_.stats.current_depth.store(depth);
    }
}

} // namespace game_req