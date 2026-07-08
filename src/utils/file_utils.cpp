#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <fstream>
#include <filesystem>
#include <cstring>

namespace game_req {

Result<std::vector<u8>> FileUtils::read_file(const Path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return make_unexpected(MAKE_ERROR(ErrorCode::FileNotFound, path.string()));
    
    u64 size = file.tellg();
    file.seekg(0);
    
    std::vector<u8> data(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to read file"));
    }
    return data;
}

Result<void> FileUtils::write_file(const Path& path, std::span<const u8> data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to open file for writing"));
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    if (!file) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to write file"));
    return success();
}

Result<void> FileUtils::write_file(const Path& path, StringView data) {
    std::ofstream file(path);
    if (!file) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to open file for writing"));
    
    file.write(data.data(), data.size());
    if (!file) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to write file"));
    return success();
}

Result<void> FileUtils::append_file(const Path& path, StringView data) {
    std::ofstream file(path, std::ios::app);
    if (!file) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to open file for appending"));
    
    file.write(data.data(), data.size());
    if (!file) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to append to file"));
    return success();
}

Result<String> FileUtils::read_text_file(const Path& path) {
    auto data = read_file(path);
    if (!data) return make_unexpected(data.error());
    return String(reinterpret_cast<char*>(data->data()), data->size());
}

Result<void> FileUtils::write_text_file(const Path& path, StringView data) {
    return write_file(path, data);
}

Result<std::vector<Path>> FileUtils::list_directory(const Path& path, bool recursive) {
    std::vector<Path> entries;
    std::error_code ec;
    
    if (recursive) {
        for (const auto& entry : fs::recursive_directory_iterator(path, ec)) {
            if (!ec) entries.push_back(entry.path());
        }
    } else {
        for (const auto& entry : fs::directory_iterator(path, ec)) {
            if (!ec) entries.push_back(entry.path());
        }
    }
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return entries;
}

Result<std::vector<Path>> FileUtils::find_files(const Path& root, StringView pattern, bool recursive) {
    std::vector<Path> results;
    std::error_code ec;
    
    auto iterator = recursive 
        ? fs::recursive_directory_iterator(root, ec)
        : fs::directory_iterator(root, ec);
    
    for (const auto& entry : iterator) {
        if (ec) break;
        if (entry.is_regular_file()) {
            String filename = entry.path().filename().string();
            if (filename.find(pattern) != String::npos) {
                results.push_back(entry.path());
            }
        }
    }
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return results;
}

bool FileUtils::exists(const Path& path) {
    std::error_code ec;
    return fs::exists(path, ec);
}

bool FileUtils::is_file(const Path& path) {
    std::error_code ec;
    return fs::is_regular_file(path, ec);
}

bool FileUtils::is_directory(const Path& path) {
    std::error_code ec;
    return fs::is_directory(path, ec);
}

bool FileUtils::is_symlink(const Path& path) {
    std::error_code ec;
    return fs::is_symlink(path, ec);
}

Result<Path> FileUtils::read_symlink(const Path& path) {
    std::error_code ec;
    Path target = fs::read_symlink(path, ec);
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return target;
}

Result<u64> FileUtils::get_size(const Path& path) {
    std::error_code ec;
    u64 size = fs::file_size(path, ec);
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return size;
}

Result<TimePoint> FileUtils::get_modified_time(const Path& path) {
    std::error_code ec;
    auto ftime = fs::last_write_time(path, ec);
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return ftime;
}

Result<TimePoint> FileUtils::get_created_time(const Path& path) {
    std::error_code ec;
    auto ftime = fs::last_write_time(path, ec); // Fallback
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return ftime;
}

Result<u32> FileUtils::get_permissions(const Path& path) {
    std::error_code ec;
    auto perms = fs::status(path, ec).permissions();
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return static_cast<u32>(perms & fs::perms::mask);
}

Result<void> FileUtils::copy(const Path& src, const Path& dst, bool overwrite) {
    std::error_code ec;
    fs::copy(src, dst, overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none, ec);
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return success();
}

Result<void> FileUtils::move(const Path& src, const Path& dst) {
    std::error_code ec;
    fs::rename(src, dst, ec);
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return success();
}

Result<void> FileUtils::remove(const Path& path) {
    std::error_code ec;
    fs::remove(path, ec);
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return success();
}

Result<void> FileUtils::remove_all(const Path& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return success();
}

Result<void> FileUtils::create_directories(const Path& path) {
    std::error_code ec;
    fs::create_directories(path, ec);
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return success();
}

Result<void> FileUtils::create_symlink(const Path& target, const Path& link) {
    std::error_code ec;
    fs::create_symlink(target, link, ec);
    if (ec) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, ec.message()));
    return success();
}

String FileUtils::extension(const Path& path) {
    return path.extension().string();
}

String FileUtils::stem(const Path& path) {
    return path.stem().string();
}

String FileUtils::filename(const Path& path) {
    return path.filename().string();
}

Path FileUtils::parent_path(const Path& path) {
    return path.parent_path();
}

Result<String> FileUtils::mime_type(const Path& path) {
    // Simplified - would use libmagic in real implementation
    String ext = extension(path);
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".txt") return "text/plain";
    return "application/octet-stream";
}

Result<FileType> FileUtils::detect_type(const Path& path) {
    return FileTypeDetector::instance().detect(path).transform([](const FileInfo& info) { return info.type; });
}

bool FileUtils::is_hidden(const Path& path) {
    String name = path.filename().string();
    return !name.empty() && name[0] == '.';
}

bool FileUtils::is_executable(const Path& path) {
    std::error_code ec;
    auto perms = fs::status(path, ec).permissions();
    if (ec) return false;
    return (perms & fs::perms::owner_exec) != fs::perms::none;
}

bool FileUtils::is_archive(const Path& path) {
    String ext = extension(path);
    static const std::vector<String> archive_exts = {
        ".zip", ".rar", ".7z", ".tar", ".gz", ".bz2", ".xz", ".zst",
        ".pak", ".pk3", ".pk4", ".vpk", ".bsa", ".ba2", ".dat", ".idx",
        ".toc", ".wad", ".mpq", ".casc", ".forge", ".big"
    };
    return std::find(archive_exts.begin(), archive_exts.end(), ext) != archive_exts.end();
}

Result<std::vector<Path>> FileUtils::extract_archive(const Path& archive, const Path& dest) {
    return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "Archive extraction not implemented"));
}

Result<std::vector<Path>> FileUtils::list_archive(const Path& archive) {
    return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "Archive listing not implemented"));
}

u64 FileUtils::calculate_crc32(const Path& path) {
    auto data = read_file(path);
    if (!data) return 0;
    return HashUtils::crc32(*data);
}

std::array<u8, 32> FileUtils::calculate_sha256(const Path& path) {
    auto data = read_file(path);
    if (!data) return {};
    return HashUtils::sha256(*data);
}

std::array<u8, 16> FileUtils::calculate_md5(const Path& path) {
    auto data = read_file(path);
    if (!data) return {};
    return HashUtils::md5(*data);
}

std::array<u8, 20> FileUtils::calculate_sha1(const Path& path) {
    auto data = read_file(path);
    if (!data) return {};
    return HashUtils::sha1(*data);
}

std::array<u8, 64> FileUtils::calculate_blake3(const Path& path) {
    auto data = read_file(path);
    if (!data) return {};
    return HashUtils::blake3(*data);
}

Result<void> FileUtils::verify_checksum(const Path& path, StringView expected, StringView algo) {
    return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "Checksum verification not implemented"));
}

FileUtils::MMapFile::MMapFile() = default;
FileUtils::MMapFile::~MMapFile() { close(); }
FileUtils::MMapFile::MMapFile(MMapFile&& other) noexcept 
    : data(other.data), size(other.size), valid(other.valid) {
    other.data = nullptr;
    other.size = 0;
    other.valid = false;
}
FileUtils::MMapFile& FileUtils::MMapFile::operator=(MMapFile&& other) noexcept {
    if (this != &other) {
        close();
        data = other.data;
        size = other.size;
        valid = other.valid;
        other.data = nullptr;
        other.size = 0;
        other.valid = false;
    }
    return *this;
}
void FileUtils::MMapFile::close() {
    if (data && valid) {
#ifdef _WIN32
        UnmapViewOfFile(data);
#else
        munmap(data, size);
#endif
        data = nullptr;
        size = 0;
        valid = false;
    }
}
std::span<const u8> FileUtils::MMapFile::span() const {
    if (!valid || !data) return {};
    return std::span<const u8>(static_cast<const u8*>(data), size);
}
std::span<u8> FileUtils::MMapFile::span() {
    if (!valid || !data) return {};
    return std::span<u8>(static_cast<u8*>(data), size);
}

Result<FileUtils::MMapFile> FileUtils::mmap_file(const Path& path) {
    return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "Memory mapping not implemented"));
}

Result<FileUtils::MMapFile> FileUtils::mmap_file_readonly(const Path& path) {
    return make_unexpected(MAKE_ERROR(ErrorCode::NotImplemented, "Memory mapping not implemented"));
}

TempFile::TempFile() {
    path = fs::temp_directory_path() / fs::unique_path("game-req-%%%%-%%%%-%%%%-%%%%");
    stream.open(path, std::ios::binary | std::ios::trunc);
}

TempFile::TempFile(StringView prefix) {
    path = fs::temp_directory_path() / (String(prefix) + fs::unique_path("-%%-%%-%%-%%").string());
    stream.open(path, std::ios::binary | std::ios::trunc);
}

TempFile::~TempFile() {
    close();
    if (!keep_) {
        std::error_code ec;
        fs::remove(path, ec);
    }
}

TempFile::TempFile(TempFile&& other) noexcept 
    : path(std::move(other.path)), stream(std::move(other.stream)), keep_(other.keep_) {
    other.keep_ = true;
}

TempFile& TempFile::operator=(TempFile&& other) noexcept {
    if (this != &other) {
        close();
        if (!keep_) {
            std::error_code ec;
            fs::remove(path, ec);
        }
        path = std::move(other.path);
        stream = std::move(other.stream);
        keep_ = other.keep_;
        other.keep_ = true;
    }
    return *this;
}

void TempFile::close() {
    if (stream.is_open()) {
        stream.flush();
        stream.close();
    }
}

void keep() {
    keep_ = true;
}

TempDir::TempDir() {
    path = fs::temp_directory_path() / fs::unique_path("game-req-%%%%-%%%%-%%%%-%%%%");
    fs::create_directories(path);
}

TempDir::TempDir(StringView prefix) {
    path = fs::temp_directory_path() / (String(prefix) + fs::unique_path("-%%-%%-%%-%%").string());
    fs::create_directories(path);
}

TempDir::~TempDir() {
    if (!keep_) {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
}

TempDir::TempDir(TempDir&& other) noexcept 
    : path(std::move(other.path)), keep_(other.keep_) {
    other.keep_ = true;
}

TempDir& TempDir::operator=(TempDir&& other) noexcept {
    if (this != &other) {
        if (!keep_) {
            std::error_code ec;
            fs::remove_all(path, ec);
        }
        path = std::move(other.path);
        keep_ = other.keep_;
        other.keep_ = true;
    }
    return *this;
}

void TempDir::keep() {
    keep_ = true;
}

} // namespace game_req
