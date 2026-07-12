#pragma once
#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>
#include <game_req_analyzer/core/error.hpp>

namespace game_req {

class FileUtils {
public:
    static Result<std::vector<u8>> read_file(const Path& path);
    static Result<void> write_file(const Path& path, std::span<const u8> data);
    static Result<void> write_file(const Path& path, StringView data);
    static Result<void> append_file(const Path& path, StringView data);
    
    static Result<String> read_text_file(const Path& path);
    static Result<void> write_text_file(const Path& path, StringView data);
    
    static Result<std::vector<Path>> list_directory(const Path& path, bool recursive = false);
    static Result<std::vector<Path>> find_files(const Path& root, StringView pattern, bool recursive = true);
    
    static bool exists(const Path& path);
    static bool is_file(const Path& path);
    static bool is_directory(const Path& path);
    static bool is_symlink(const Path& path);
    static Result<Path> read_symlink(const Path& path);
    
    static Result<u64> get_size(const Path& path);
    static Result<TimePoint> get_modified_time(const Path& path);
    static Result<TimePoint> get_created_time(const Path& path);
    static Result<u32> get_permissions(const Path& path);
    
    static Result<void> copy(const Path& src, const Path& dst, bool overwrite = false);
    static Result<void> move(const Path& src, const Path& dst);
    static Result<void> remove(const Path& path);
    static Result<void> remove_all(const Path& path);
    
    static Result<void> create_directories(const Path& path);
    static Result<void> create_symlink(const Path& target, const Path& link);
    
    static String extension(const Path& path);
    static String stem(const Path& path);
    static String filename(const Path& path);
    static Path parent_path(const Path& path);
    
    static Result<String> mime_type(const Path& path);
    static Result<FileType> detect_type(const Path& path);
    
    static bool is_hidden(const Path& path);
    static bool is_executable(const Path& path);
    static bool is_archive(const Path& path);
    
    static Result<std::vector<Path>> extract_archive(const Path& archive, const Path& dest);
    static Result<std::vector<Path>> list_archive(const Path& archive);
    
    static u64 calculate_crc32(const Path& path);
    static std::array<u8, 32> calculate_sha256(const Path& path);
    static std::array<u8, 16> calculate_md5(const Path& path);
    static std::array<u8, 20> calculate_sha1(const Path& path);
    static std::array<u8, 64> calculate_blake3(const Path& path);
    
    static Result<void> verify_checksum(const Path& path, StringView expected, StringView algo);
    
    struct MMapFile {
        void* data = nullptr;
        u64 size = 0;
        bool valid = false;
        
        MMapFile() = default;
        ~MMapFile() { close(); }
        MMapFile(MMapFile&& other) noexcept;
        MMapFile& operator=(MMapFile&& other) noexcept;
        MMapFile(const MMapFile&) = delete;
        MMapFile& operator=(const MMapFile&) = delete;
        
        void close();
        [[nodiscard]] std::span<const u8> span() const;
        [[nodiscard]] std::span<u8> span();
    };
    
    static Result<MMapFile> mmap_file(const Path& path);
    static Result<MMapFile> mmap_file_readonly(const Path& path);
};

class TempFile {
public:
    TempFile();
    explicit TempFile(StringView prefix);
    ~TempFile();
    
    TempFile(TempFile&&) noexcept;
    TempFile& operator=(TempFile&&) noexcept;
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
    
    [[nodiscard]] const Path& path() const { return path_; }
    [[nodiscard]] std::ofstream& stream() { return stream_; }
    
    void close();
    void keep();
    
private:
    Path path_;
    std::ofstream stream_;
    bool keep_ = false;
};

class TempDir {
public:
    TempDir();
    explicit TempDir(StringView prefix);
    ~TempDir();
    
    TempDir(TempDir&&) noexcept;
    TempDir& operator=(TempDir&&) noexcept;
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    
    [[nodiscard]] const Path& path() const { return path_; }
    void keep();
    
private:
    Path path_;
    bool keep_ = false;
};

} // namespace game_req