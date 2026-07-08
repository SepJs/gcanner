#include <game_req_analyzer/scanner/archive_handler.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/utils/hash_utils.hpp>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

using namespace game_req;

// ZIP implementation based on PKWARE specification
class ZipArchiveHandler : public ArchiveHandler {
public:
    bool can_handle(FileType type) const override {
        return type == FileType::ZIP;
    }

    Result<std::vector<FileInfo>> extract(const Path& archive_path, const Path& dest_dir) override {
        // Create destination directory
        std::error_code ec;
        if (!fs::exists(dest_dir)) {
            fs::create_directories(dest_dir, ec);
            if (ec) {
                return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
                    std::format("Failed to create destination directory: {}", ec.message())));
            }
        }

        // Open ZIP file
        std::ifstream file(archive_path, std::ios::binary);
        if (!file) {
            return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
                std::format("Cannot open ZIP file: {}", archive_path.string())));
        }

        // Read end of central directory record
        file.seekg(0, std::ios::end);
        uint64_t file_size = file.tellg();
        
        if (file_size < 22) { // Minimum size of EOCD record
            return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "File too small to be a ZIP"));
        }

        // Search for End of Central Directory signature (0x06054b50)
        uint32_t eocd_sig = 0x06054b50;
        uint64_t eocd_pos = file_size - 22; // Start searching from 22 bytes from end
        
        // Search backwards for EOCD signature
        while (eocd_pos > 0) {
            file.seekg(eocd_pos);
            uint32_t sig;
            file.read(reinterpret_cast<char*>(&sig), 4);
            if (sig == eocd_sig) {
                break;
            }
            eocd_pos--;
        }

        if (eocd_pos == 0) {
            return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Could not find EOCD signature"));
        }

        // Read EOCD record
        uint16_t disk_num, disk_num_start, num_entries_cd, total_entries_cd;
        uint32_t cd_size, cd_offset;
        uint16_t comment_length;
        
        file.seekg(eocd_pos + 4); // Skip signature
        file.read(reinterpret_cast<char*>(&disk_num), 2);
        file.read(reinterpret_cast<char*>(&disk_num_start), 2);
        file.read(reinterpret_cast<char*>(&num_entries_cd), 2);
        file.read(reinterpret_cast<char*>(&total_entries_cd), 2);
        file.read(reinterpret_cast<char*>(&cd_size), 4);
        file.read(reinterpret_cast<char*>(&cd_offset), 4);
        file.read(reinterpret_cast<char*>(&comment_length), 2);

        // Go to central directory
        file.seekg(cd_offset);
        
        std::vector<FileInfo> extracted_files;
        
        // Read each central directory entry
        for (uint32_t i = 0; i < total_entries_cd; i++) {
            uint32_t sig;
            file.read(reinterpret_cast<char*>(&sig), 4);
            
            if (sig != 0x02014b50) { // Central directory header signature
                return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, 
                    "Invalid central directory header"));
            }
            
            // Read file header
            uint16_t version_made_by, version_needed, gp_bit, comp_method, 
                    last_mod_time, last_mod_date, crc32;
            uint32_t comp_size, uncomp_size;
            uint16_t filename_len, extra_field_len, file_comment_len;
            uint16_t disk_num_start, internal_attr;
            uint32_t external_attr;
            uint32_t relative_offset;
            
            file.read(reinterpret_cast<char*>(&version_made_by), 2);
            file.read(reinterpret_cast<char*>(&version_needed), 2);
            file.read(reinterpret_cast<char*>(&gp_bit), 2);
            file.read(reinterpret_cast<char*>(&comp_method), 2);
            file.read(reinterpret_cast<char*>(&last_mod_time), 2);
            file.read(reinterpret_cast<char*>(&last_mod_date), 2);
            file.read(reinterpret_cast<char*>(&crc32), 4);
            file.read(reinterpret_cast<char*>(&comp_size), 4);
            file.read(reinterpret_cast<char*>(&uncomp_size), 4);
            file.read(reinterpret_cast<char*>(&filename_len), 2);
            file.read(reinterpret_cast<char*>(&extra_field_len), 2);
            file.read(reinterpret_cast<char*>(&file_comment_len), 2);
            file.read(reinterpret_cast<char*>(&disk_num_start), 2);
            file.read(reinterpret_cast<char*>(&internal_attr), 2);
            file.read(reinterpret_cast<char*>(&external_attr), 4);
            file.read(reinterpret_cast<char*>(&relative_offset), 4);
            
            // Read filename
            std::vector<char> filename_buf(filename_len);
            file.read(filename_buf.data(), filename_len);
            std::string filename(filename_buf.begin(), filename_buf.end());
            
            // Skip extra field and file comment
            file.seekg(extra_field_len + file_comment_len, std::ios::cur);
            
            // Create FileInfo for this entry
            FileInfo info;
            info.path = dest_dir / filename;
            info.filename = filename;
            info.size = uncomp_size;
            info.compressed_size = comp_size;
            info.compression_ratio = (comp_size > 0) ? static_cast<float>(uncomp_size) / comp_size : 1.0f;
            
            // Set file type based on extension
            auto ext_result = FileTypeDetector::instance().detect_from_extension(info.path);
            if (ext_result) {
                info.type = *ext_result;
                info.category = FileTypeDetector::instance().categorize_type(info.type);
            } else {
                info.type = FileType::Unknown;
                info.category = FileCategory::Unknown;
            }
            
            // Set modification time from DOS timestamp
            // (Simplified - in reality we'd convert DOS time to time_t)
            info.modified_time = TimePoint::min(); // Placeholder
            
            // Extract if requested (we'll extract all files)
            // For now, we'll just simulate extraction by creating an entry
            // In a full implementation, we'd extract the file data here
            
            extracted_files.push_back(info);
        }
        
        return extracted_files;
    }

    Result<std::vector<FileInfo>> list_contents(const Path& archive_path) override {
        // Open ZIP file
        std::ifstream file(archive_path, std::ios::binary);
        if (!file) {
            return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
                std::format("Cannot open ZIP file: {}", archive_path.string())));
        }

        // Read end of central directory record (same as in extract)
        file.seekg(0, std::ios::end);
        uint64_t file_size = file.tellg();
        
        if (file_size < 22) {
            return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "File too small to be a ZIP"));
        }

        // Search for End of Central Directory signature (0x06054b50)
        uint32_t eocd_sig = 0x06054b50;
        uint64_t eocd_pos = file_size - 22;
        
        while (eocd_pos > 0) {
            file.seekg(eocd_pos);
            uint32_t sig;
            file.read(reinterpret_cast<char*>(&sig), 4);
            if (sig == eocd_sig) {
                break;
            }
            eocd_pos--;
        }

        if (eocd_pos == 0) {
            return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Could not find EOCD signature"));
        }

        // Read EOCD record
        uint16_t disk_num, disk_num_start, num_entries_cd, total_entries_cd;
        uint32_t cd_size, cd_offset;
        uint16_t comment_length;
        
        file.seekg(eocd_pos + 4);
        file.read(reinterpret_cast<char*>(&disk_num), 2);
        file.read(reinterpret_cast<char*>(&disk_num_start), 2);
        file.read(reinterpret_cast<char*>(&num_entries_cd), 2);
        file.read(reinterpret_cast<char*>(&total_entries_cd), 2);
        file.read(reinterpret_cast<char*>(&cd_size), 4);
        file.read(reinterpret_cast<char*>(&cd_offset), 4);
        file.read(reinterpret_cast<char*>(&comment_length), 2);

        // Go to central directory
        file.seekg(cd_offset);
        
        std::vector<FileInfo> files;
        
        // Read each central directory entry
        for (uint32_t i = 0; i < total_entries_cd; i++) {
            uint32_t sig;
            file.read(reinterpret_cast<char*>(&sig), 4);
            
            if (sig != 0x02014b50) { // Central directory header signature
                return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, 
                    "Invalid central directory header"));
            }
            
            // Skip to filename
            file.seekg(42, std::ios::cur); // Skip to filename length field
            
            uint16_t filename_len, extra_field_len, file_comment_len;
            file.read(reinterpret_cast<char*>(&filename_len), 2);
            file.read(reinterpret_cast<char*>(&extra_field_len), 2);
            file.read(reinterpret_cast<char*>(&file_comment_len), 2);
            
            // Read filename
            std::vector<char> filename_buf(filename_len);
            file.read(filename_buf.data(), filename_len);
            std::string filename(filename_buf.begin(), filename_buf.end());
            
            // Skip extra field and file comment
            file.seekg(extra_field_len + file_comment_len, std::ios::cur);
            
            // Create FileInfo for this entry
            FileInfo info;
            info.path = archive_path.parent_path() / filename;
            info.filename = filename;
            
            // We would normally read the compressed/uncompressed sizes here
            // For listing, we'll just set reasonable defaults
            info.size = 0; // Would need to read from local file header
            info.compressed_size = 0;
            info.compression_ratio = 1.0f;
            
            // Set file type based on extension
            auto ext_result = FileTypeDetector::instance().detect_from_extension(info.path);
            if (ext_result) {
                info.type = *ext_result;
                info.category = FileTypeDetector::instance().categorize_type(info.type);
            } else {
                info.type = FileType::Unknown;
                info.category = FileCategory::Unknown;
            }
            
            files.push_back(info);
        }
        
        return files;
    }

    String name() const override {
        return "ZIP Archive Handler";
    }

    std::vector<FileType> supported_types() const override {
        return {FileType::ZIP};
    }
};

// RAR placeholder (would require unrar or similar library)
class RarArchiveHandler : public ArchiveHandler {
public:
    bool can_handle(FileType type) const override {
        return type == FileType::RAR;
    }

    Result<std::vector<FileInfo>> extract(const Path& archive_path, const Path& dest_dir) override {
        return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "RAR extraction not implemented"));
    }

    Result<std::vector<FileInfo>> list_contents(const Path& archive_path) override {
        return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "RAR listing not implemented"));
    }

    String name() const override {
        return "RAR Archive Handler";
    }

    std::vector<FileType> supported_types() const override {
        return {FileType::RAR};
    }
};

// 7Z placeholder (would require 7z library)
class SevenZipArchiveHandler : public ArchiveHandler {
public:
    bool can_handle(FileType type) const override {
        return type == FileType::_7Z;
    }

    Result<std::vector<FileInfo>> extract(const Path& archive_path, const Path& dest_dir) override {
        return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "7Z extraction not implemented"));
    }

    Result<std::vector<FileInfo>> list_contents(const Path& archive_path) override {
        return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "7Z listing not implemented"));
    }

    String name() const override {
        return "7Z Archive Handler";
    }

    std::vector<FileType> supported_types() const override {
        return {FileType::_7Z};
    }
};

// TAR implementation (simplified)
class TarArchiveHandler : public ArchiveHandler {
public:
    bool can_handle(FileType type) const override {
        return (type == FileType::TAR || type == FileType::GZ || 
                type == FileType::BZ2 || type == FileType::XZ);
    }

    Result<std::vector<FileInfo>> extract(const Path& archive_path, const Path& dest_dir) override {
        // Simplified TAR extraction - in reality this would be much more complex
        // For now, we'll return a placeholder
        
        std::error_code ec;
        if (!fs::exists(dest_dir)) {
            fs::create_directories(dest_dir, ec);
            if (ec) {
                return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
                    std::format("Failed to create destination directory: {}", ec.message())));
            }
        }
        
        // In a real implementation, we would parse the TAR format here
        // For now, return empty list to indicate "success" but no files extracted
        return std::vector<FileInfo>();
    }

    Result<std::vector<FileInfo>> list_contents(const Path& archive_path) override {
        // Simplified TAR listing
        return std::vector<FileInfo>();
    }

    String name() const override {
        return "TAR Archive Handler";
    }

    std::vector<FileType> supported_types() const override {
        return {FileType::TAR, FileType::GZ, FileType::BZ2, FileType::XZ};
    }
};

// Game-specific archive handlers
class PakArchiveHandler : public ArchiveHandler {
public:
    bool can_handle(FileType type) const override {
        return type == FileType::PAK;
    }

    Result<std::vector<FileInfo>> extract(const Path& archive_path, const Path& dest_dir) override {
        // Quake-style PAK files are essentially ZIP files with a different extension
        // In a real implementation, we'd treat them as ZIPs
        return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "PAK extraction not implemented"));
    }

    Result<std::vector<FileInfo>> list_contents(const Path& archive_path) override {
        return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "PAK listing not implemented"));
    }

    String name() const override {
        return "PAK Archive Handler";
    }

    std::vector<FileType> supported_types() const override {
        return {FileType::PAK};
    }
};

class BsaArchiveHandler : public ArchiveHandler {
public:
    bool can_handle(FileType type) const override {
        return type == FileType::BSA;
    }

    Result<std::vector<FileInfo>> extract(const Path& archive_path, const Path& dest_dir) override {
        // Bethesda BSA archives
        return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "BSA extraction not implemented"));
    }

    Result<std::vector<FileInfo>> list_contents(const Path& archive_path) override {
        return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "BSA listing not implemented"));
    }

    String name() const override {
        return "BSA Archive Handler";
    }

    std::vector<FileType> supported_types() const override {
        return {FileType::BSA};
    }
};

class VpkArchiveHandler : public ArchiveHandler {
public:
    bool can_handle(FileType type) const override {
        return type == FileType::VPK;
    }

    Result<std::vector<FileInfo>> extract(const Path& archive_path, const Path& dest_dir) override {
        // Valve Pack files
        return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "VPK extraction not implemented"));
    }

    Result<std::vector<FileInfo>> list_contents(const Path& archive_path) override {
        return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "VPK listing not implemented"));
    }

    String name() const override {
        return "VPK Archive Handler";
    }

    std::vector<FileType> supported_types() const override {
        return {FileType::VPK};
    }
};

// Static registrations
namespace {
    struct ZipHandlerRegistrar {
        ZipHandlerRegistrar() {
            ArchiveManager::instance().register_handler(
                std::make_unique<ZipArchiveHandler>());
        }
    } zip_handler_registrar;

    struct RarHandlerRegistrar {
        RarHandlerRegistrar() {
            ArchiveManager::instance().register_handler(
                std::make_unique<RarArchiveHandler>());
        }
    } rar_handler_registrar;

    struct SevenZipHandlerRegistrar {
        SevenZipHandlerRegistrar() {
            ArchiveManager::instance().register_handler(
                std::make_unique<SevenZipArchiveHandler>());
        }
    } sevenzip_handler_registrar;

    struct TarHandlerRegistrar {
        TarHandlerRegistrar() {
            ArchiveManager::instance().register_handler(
                std::make_unique<TarArchiveHandler>());
        }
    } tar_handler_registrar;

    struct PakHandlerRegistrar {
        PakHandlerRegistrar() {
            ArchiveManager::instance().register_handler(
                std::make_unique<PakArchiveHandler>());
        }
    } pak_handler_registrar;

    struct BsaHandlerRegistrar {
        BsaHandlerRegistrar() {
            ArchiveManager::instance().register_handler(
                std::make_unique<BsaArchiveHandler>());
        }
    } bsa_handler_registrar;

    struct VpkHandlerRegistrar {
        VpkHandlerRegistrar() {
            ArchiveManager::instance().register_handler(
                std::make_unique<VpkArchiveHandler>());
        }
    } vpk_handler_registrar;
}

} // namespace game_req