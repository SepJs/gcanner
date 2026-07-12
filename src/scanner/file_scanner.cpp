#include <game_req_analyzer/scanner/file_scanner.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/utils/hash_utils.hpp>
#include <game_req_analyzer/utils/time_utils.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

using namespace game_req;

// Note: The actual archive handlers are now defined in archive_handler.cpp
// and registered through the static registrators in that file

FileScanner::FileScanner(const ScanConfig& config) : config_(config) {
    // Archive handlers are now registered via static initializers in archive_handler.cpp
    // No need to register them here anymore
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
                        results_.files.push_back(*file_result);
                    }
                    
                    // Handle archives if enabled
                    if (file_result->is_archive && !config_.follow_symlinks) {
                        // In a full implementation, we'd extract and scan archives
                        // For now, just count them
                        results_.stats.archives_found++;
                    }
                } else {
                    std::lock_guard<std::mutex> lock(results_mutex_);
                    results_.errors.push_back(file_result.error());
                    results_.stats.errors++;
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        LOG_WARN("Filesystem error scanning {}: {}", dir.string(), e.what());
        std::lock_guard<std::mutex> lock(results_mutex_);
        results_.errors.push_back(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Filesystem error: {}", e.what())));
        results_.stats.errors++;
    } catch (const std::exception& e) {
        LOG_WARN("Error scanning {}: {}", dir.string(), e.what());
        std::lock_guard<std::mutex> lock(results_mutex_);
        results_.errors.push_back(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Error: {}", e.what())));
        results_.stats.errors++;
    }
}

Result<FileInfo> FileScanner::scan_file(const Path& file_path) {
    try {
        // Detect file type
        auto detect_result = FileTypeDetector::instance().detect(file_path);
        if (!detect_result) {
            return make_unexpected(detect_result.error());
        }
        FileInfo info = *detect_result;
        
        // Check file size limits
        if (info.size > config_.max_file_size) {
            return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, 
                std::format("File too large: {} > {}", info.size, config_.max_file_size)));
        }
        
        // Check if extension is excluded
        String ext = StringUtils::to_lower(file_path.extension().string());
        if (!ext.empty() && config_.excluded_exts.find(ext) != config_.excluded_exts.end()) {
            return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, 
                std::format("Excluded extension: {}", ext)));
        }
        
// Calculate hash if requested
        if (config_.verify_checksums && info.size < 100 * MiB) { // Only hash files < 100MB for performance
            // Read file content into a buffer for hashing
            std::ifstream file(file_path, std::ios::binary);
            if (file) {
                std::vector<u8> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

                // Compute hash
                std::array<u8, 32> hash_result = HashUtils::sha256(buffer);
                std::copy(hash_result.begin(), hash_result.end(), info.hash.begin());
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
    results_.stats.files_scanned++;
    results_.stats.total_size += file.size;
    
    // Update depth counting by counting the number of path separators
    u32 depth = 0;
    auto path_str = file.path.string();
    for (char c : path_str) {
        if (c == fs::path::preferred_separator) {
            depth++;
        }
    }
    if (depth > results_.stats.current_depth) {
        results_.stats.current_depth = depth;
    }
}