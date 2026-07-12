#include <game_req_analyzer/scanner/file_type_detector.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/utils/hash_utils.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/utils/time_utils.hpp>
#include <fstream>
#include <cstring>
#include <sys/stat.h>

namespace game_req {

FileTypeDetector& FileTypeDetector::instance() {
    static FileTypeDetector detector;
    return detector;
}

FileTypeDetector::FileTypeDetector() {
    init_default_signatures();
    init_default_extensions();
    init_default_mimes();
}

void FileTypeDetector::init_default_signatures() {
    // Executable signatures
    signatures_.push_back(FileSignature{{0x7f, 'E', 'L', 'F', 0,0,0,0,0,0,0,0,0,0,0,0}, 4, 0}); // ELF
    signatures_.push_back(FileSignature{{'M', 'Z', 0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 2, 0}); // DOS/PE
    signatures_.push_back(FileSignature{{0xCF, 0xFA, 0xED, 0xFE, 0,0,0,0,0,0,0,0,0,0,0,0}, 4, 0}); // Mach-O big endian
    signatures_.push_back(FileSignature{{0xFE, 0xED, 0xFA, 0xCF, 0,0,0,0,0,0,0,0,0,0,0,0}, 4, 0}); // Mach-O little endian
    signatures_.push_back(FileSignature{{0xCA, 0xFE, 0xBA, 0xBE, 0,0,0,0,0,0,0,0,0,0,0,0}, 4, 0}); // Java class
    
    // Archive signatures
    signatures_.push_back(FileSignature{{'P', 'K', 0x03, 0x04, 0,0,0,0,0,0,0,0,0,0,0,0}, 4, 0}); // ZIP
    signatures_.push_back(FileSignature{{'R', 'a', 'r', '!', 0,0,0,0,0,0,0,0,0,0,0,0}, 4, 0}); // RAR
    signatures_.push_back(FileSignature{{0x1F, 0x8B, 0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 2, 0}); // GZIP
    signatures_.push_back(FileSignature{{'B', 'Z', 'h', 0,0,0,0,0,0,0,0,0,0,0,0}, 3, 0}); // BZIP2
    signatures_.push_back(FileSignature{{0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C, 0,0,0,0,0,0,0,0}, 6, 0}); // 7z
    signatures_.push_back(FileSignature{{0xFD, '7', 'z', 'X', 'Z', 0x00, 0,0,0,0,0,0,0,0}, 6, 0}); // XZ
    
    // Image/texture signatures
    signatures_.push_back(FileSignature{{0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 0,0,0,0,0,0,0,0}, 8, 0}); // PNG
    signatures_.push_back(FileSignature{{0xFF, 0xD8, 0xFF, 0,0,0,0,0,0,0,0,0,0,0,0,0}, 2, 0}); // JPEG
    signatures_.push_back(FileSignature{{'B', 'M', 0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 2, 0}); // BMP
    signatures_.push_back(FileSignature{{'G', 'I', 'F', '8', '7', 'a', 0,0,0,0,0,0,0,0}, 6, 0}); // GIF87a
    signatures_.push_back(FileSignature{{'G', 'I', 'F', '8', '9', 'a', 0,0,0,0,0,0,0,0}, 6, 0}); // GIF89a
    signatures_.push_back(FileSignature{{'T', 'I', 'I', 'F', 0,0,0,0,0,0,0,0,0,0,0,0}, 4, 0}); // TIFF (big endian)
    signatures_.push_back(FileSignature{{'I', 'I', '*', 0x00, 0,0,0,0,0,0,0,0,0,0,0,0}, 4, 0}); // TIFF (little endian)
    signatures_.push_back(FileSignature{{'D', 'D', 'S', ' ', 0,0,0,0,0,0,0,0,0,0,0,0}, 4, 0}); // DDS
    signatures_.push_back(FileSignature{{0x89, 'K', 'T', 'X', ' ', '1', '1', 0x0B, 0,0,0,0,0,0,0,0}, 8, 0}); // KTX
    
    // Audio signatures
    signatures_.push_back(FileSignature{{'R', 'I', 'F', 'F', 0x00, 0x00, 0x00, 0x00, 'W', 'A', 'V', 'E', 0,0,0,0}, 12, 0}); // WAV
    signatures_.push_back(FileSignature{{'O', 'g', 'g', 'S', 0,0,0,0,0,0,0,0,0,0,0}, 4, 0}); // OGG
    signatures_.push_back(FileSignature{{'f', 'L', 'a', 'C', 0,0,0,0,0,0,0,0,0}, 4, 0}); // FLAC
    signatures_.push_back(FileSignature{{'I', 'D', '3', 0,0,0,0,0,0,0,0,0}, 3, 0}); // ID3 tag (MP3)
    signatures_.push_back(FileSignature{{0xFF, 0xFB, 0,0,0,0,0,0,0,0,0}, 2, 0}); // MP3
    signatures_.push_back(FileSignature{{0xFF, 0xF3, 0,0,0,0,0,0,0,0,0}, 2, 0}); // MP3
    signatures_.push_back(FileSignature{{0xFF, 0xF2, 0,0,0,0,0,0,0,0,0}, 2, 0}); // MP3
    
    // 3D model signatures
    signatures_.push_back(FileSignature{{'f', 'b', 'x', 0,0,0,0,0,0,0,0,0,0}, 3, 0}); // FBX (binary)
    // Note: FBX binary header is longer, but we're just checking for the start
    
    // Script signatures (shebangs)
    signatures_.push_back(FileSignature{{'#', '!', '/', 'b', 'i', 'n', '/', 'b', 'a', 's', 'h', 0,0,0,0}, 10, 0});
    signatures_.push_back(FileSignature{{'#', '!', '/', 'u', 's', 'r', '/', 'e', 'n', 'v', ' ', 'p', 'y', 't', 'h', 'o'}, 16, 0}); // python (truncated)
    signatures_.push_back(FileSignature{{'#', '!', '/', 'b', 'i', 'n', '/', 's', 'h', 0,0,0,0}, 9, 0});
    
    // Shader signatures
    signatures_.push_back(FileSignature{{/* DXBC */ 0x44, 0x4B, 0x43, 0x42, 0,0,0,0,0,0,0,0}, 4, 0});
    signatures_.push_back(FileSignature{{/* SPIR-V */ 0x03, 0x02, 0x23, 0x07, 0,0,0,0,0,0,0,0}, 4, 0});
}

void FileTypeDetector::init_default_extensions() {
    // Executables
    register_extension(FileType::PE32, ".exe");
    register_extension(FileType::PE64, ".exe");
    register_extension(FileType::ELF32, "");
    register_extension(FileType::ELF64, "");
    register_extension(FileType::MachO32, "");
    register_extension(FileType::MachO64, "");
    register_extension(FileType::DLL, ".dll");
    register_extension(FileType::SO, ".so");
    register_extension(FileType::DYLIB, ".dylib");
    
    // Textures
    register_extension(FileType::DDS, ".dds");
    register_extension(FileType::PNG, ".png");
    register_extension(FileType::JPG, ".jpg");
    register_extension(FileType::JPG, ".jpeg");
    register_extension(FileType::TGA, ".tga");
    register_extension(FileType::BMP, ".bmp");
    register_extension(FileType::TIFF, ".tiff");
    register_extension(FileType::TIFF, ".tif");
    register_extension(FileType::EXR, ".exr");
    register_extension(FileType::HDR, ".hdr");
    register_extension(FileType::KTX, ".ktx");
    register_extension(FileType::ASTC, ".astc");
    register_extension(FileType::PVR, ".pvr");
    register_extension(FileType::WebP, ".webp");
    
    // 3D Models
    register_extension(FileType::FBX, ".fbx");
    register_extension(FileType::OBJ, ".obj");
    register_extension(FileType::GLTF, ".gltf");
    register_extension(FileType::GLB, ".glb");
    register_extension(FileType::COLLADA, ".dae");
    register_extension(FileType::USD, ".usd");
    register_extension(FileType::USDZ, ".usdz");
    register_extension(FileType::BLEND, ".blend");
    register_extension(FileType::MAX, ".max");
    register_extension(FileType::MA, ".ma");
    register_extension(FileType::MB, ".mb");
    register_extension(FileType::X, ".x");
    register_extension(FileType::MDL, ".mdl");
    register_extension(FileType::MD2, ".md2");
    register_extension(FileType::MD3, ".md3");
    register_extension(FileType::MD5, ".md5");
    register_extension(FileType::NIF, ".nif");
    register_extension(FileType::HKX, ".hkx");
    register_extension(FileType::GR2, ".gr2");
    register_extension(FileType::SMD, ".smd");
    register_extension(FileType::DMX, ".dmx");
    
    // Audio
    register_extension(FileType::WAV, ".wav");
    register_extension(FileType::OGG, ".ogg");
    register_extension(FileType::MP3, ".mp3");
    register_extension(FileType::FLAC, ".flac");
    register_extension(FileType::AAC, ".aac");
    register_extension(FileType::M4A, ".m4a");
    register_extension(FileType::WMA, ".wma");
    register_extension(FileType::AIFF, ".aiff");
    register_extension(FileType::AU, ".au");
    register_extension(FileType::RAW, ".raw");
    register_extension(FileType::XMA, ".xma");
    register_extension(FileType::ATRAC, ".atrac");
    register_extension(FileType::FSB, ".fsb");
    register_extension(FileType::BNK, ".bnk");
    register_extension(FileType::PKG, ".pkg");
    register_extension(FileType::XWB, ".xwb");
    register_extension(FileType::XSB, ".xsb");
    
    // Scripts
    register_extension(FileType::LUA, ".lua");
    register_extension(FileType::PY, ".py");
    register_extension(FileType::JS, ".js");
    register_extension(FileType::TS, ".ts");
    register_extension(FileType::CS, ".cs");
    register_extension(FileType::VB, ".vb");
    register_extension(FileType::PL, ".pl");
    register_extension(FileType::SH, ".sh");
    register_extension(FileType::BAT, ".bat");
    register_extension(FileType::PS1, ".ps1");
    register_extension(FileType::ANGELSCRIPT, ".as");
    register_extension(FileType::SQF, ".sqf");
    register_extension(FileType::PAWN, ".pwn");
    register_extension(FileType::SCPT, ".scpt");
    register_extension(FileType::CSC, ".csc");
    register_extension(FileType::CSCRIPT, ".cs");
    
    // Shaders
    register_extension(FileType::HLSL, ".hlsl");
    register_extension(FileType::GLSL, ".glsl");
    register_extension(FileType::SPIRV, ".spv");
    register_extension(FileType::CG, ".cg");
    register_extension(FileType::CGINC, ".cginc");
    register_extension(FileType::FX, ".fx");
    register_extension(FileType::FXO, ".fxo");
    register_extension(FileType::SHADER, ".shader");
    register_extension(FileType::VSH, ".vsh");
    register_extension(FileType::PSH, ".psh");
    register_extension(FileType::GSH, ".gsh");
    register_extension(FileType::USH, ".ush");
    register_extension(FileType::USF, ".usf");
    register_extension(FileType::METAL, ".metal");
    register_extension(FileType::MSL, ".msl");
    
    // Particles
    register_extension(FileType::PFX, ".pfx");
    register_extension(FileType::PTX, ".ptx");
    register_extension(FileType::PAR, ".par");
    register_extension(FileType::PARTICLE, ".particle");
    register_extension(FileType::EMITTER, ".emitter");
    
    // Physics
    register_extension(FileType::PHYSX, ".physx");
    register_extension(FileType::HAVOK, ".havok");
    register_extension(FileType::BULLET, ".bullet");
    register_extension(FileType::ODE, ".ode");
    register_extension(FileType::NEWTON, ".newton");
    register_extension(FileType::PHYSX3, ".physx3");
    
    // Archives
    register_extension(FileType::ZIP, ".zip");
    register_extension(FileType::RAR, ".rar");
    register_extension(FileType::SEVENZ, ".7z");
    register_extension(FileType::TAR, ".tar");
    register_extension(FileType::GZ, ".gz");
    register_extension(FileType::BZ2, ".bz2");
    register_extension(FileType::XZ, ".xz");
    register_extension(FileType::ZST, ".zst");
    register_extension(FileType::PAK, ".pak");
    register_extension(FileType::PK3, ".pk3");
    register_extension(FileType::PK4, ".pk4");
    register_extension(FileType::VPK, ".vpk");
    register_extension(FileType::BSA, ".bsa");
    register_extension(FileType::BA2, ".ba2");
    register_extension(FileType::DAT, ".dat");
    register_extension(FileType::IDX, ".idx");
    register_extension(FileType::TOC, ".toc");
    register_extension(FileType::WAD, ".wad");
    register_extension(FileType::MPQ, ".mpq");
    register_extension(FileType::CASC, ".casc");
    register_extension(FileType::FORGE, ".forge");
    register_extension(FileType::BIG, ".big");
    
    // Databases
    register_extension(FileType::SQLITE, ".sqlite");
    register_extension(FileType::SQLITE, ".db");
    register_extension(FileType::SQLITE, ".sqlite3");
    register_extension(FileType::LEVELDB, ".ldb");
    register_extension(FileType::ROCKSDB, ".rdb");
    register_extension(FileType::LMDB, ".lmdb");
    register_extension(FileType::BERKELEYDB, ".db");
    
    // Config
    register_extension(FileType::INI, ".ini");
    register_extension(FileType::INI, ".cfg");
    register_extension(FileType::INI, ".conf");
    register_extension(FileType::XML, ".xml");
    register_extension(FileType::JSON, ".json");
    register_extension(FileType::YAML, ".yaml");
    register_extension(FileType::YAML, ".yml");
    register_extension(FileType::TOML, ".toml");
    register_extension(FileType::PROPERTIES, ".properties");
    
    // Fonts
    register_extension(FileType::TTF, ".ttf");
    register_extension(FileType::OTF, ".otf");
    register_extension(FileType::WOFF, ".woff");
    register_extension(FileType::WOFF2, ".woff2");
    register_extension(FileType::FNT, ".fnt");
    register_extension(FileType::BMFONT, ".fnt");
    
    // Video
    register_extension(FileType::MP4, ".mp4");
    register_extension(FileType::AVI, ".avi");
    register_extension(FileType::MKV, ".mkv");
    register_extension(FileType::MOV, ".mov");
    register_extension(FileType::WMV, ".wmv");
    register_extension(FileType::FLV, ".flv");
    register_extension(FileType::WEBM, ".webm");
    register_extension(FileType::MPEG, ".mpeg");
    register_extension(FileType::BINK, ".bik");
    register_extension(FileType::BIK, ".bik");
    
    // Documents
    register_extension(FileType::PDF, ".pdf");
    register_extension(FileType::TXT, ".txt");
    register_extension(FileType::RTF, ".rtf");
    register_extension(FileType::MD, ".md");
    register_extension(FileType::HTML, ".html");
    register_extension(FileType::HTML, ".htm");
    register_extension(FileType::CHM, ".chm");
}

void FileTypeDetector::init_default_mimes() {
    // This would typically be populated from system mime types
    // For now, we'll leave it mostly empty and rely on extensions/signatures
}

Result<FileInfo> FileTypeDetector::detect(const Path& path) const {
    FileInfo info;
    info.path = path;
    
    try {
        if (!fs::exists(path)) {
            return make_unexpected(MAKE_ERROR(ErrorCode::FileNotFound, 
                std::format("File does not exist: {}", path.string())));
        }
        
        if (!fs::is_regular_file(path)) {
            info.is_symlink = fs::is_symlink(path);
            if (info.is_symlink) {
                try {
                    info.symlink_target = fs::read_symlink(path);
                } catch (...) {
                    // If we can't read the symlink target, just continue
                }
            }
            return info; // Return basic info for directories/symlinks
        }
        
        info.size = fs::file_size(path);
        auto ftime = fs::last_write_time(path);
        // Convert file_time_type to system_clock::time_point
        auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + chrono::system_clock::now()
        );
        info.modified_time = sctp;
        info.created_time = sctp; // Approximation
        
        // Get permissions
#ifdef _WIN32
        info.permissions = 0; // Simplified for Windows
#else
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            info.permissions = st.st_mode & 0777;
        }
#endif
        
        // Detect from extension first (fastest)
        auto ext_result = detect_from_extension(path);
        if (ext_result) {
            info.type = *ext_result;
        }
        
        // Detect from content/magic
        auto content_result = detect_from_content(path);
        if (content_result) {
            info.type = *content_result;
        }
        
        // Determine category from type
        info.category = categorize_type(info.type);
        info.extension = path.extension().string();
        
        // Try to get MIME type
        info.mime_type = FileUtils::mime_type(path).value_or("application/octet-stream");
        
        return info;
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Detection failed: {}", e.what())));
    }
}

Result<FileType> FileTypeDetector::detect_from_extension(const Path& path) const {
    std::string ext = path.extension().string();
    if (ext.empty()) return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "No extension"));
    
    ext = StringUtils::to_lower(ext);
    
    auto it = extension_map_.find(ext);
    if (it != extension_map_.end()) {
        return it->second;
    }
    
    return make_unexpected(MAKE_ERROR(ErrorCode::UnsupportedFormat, 
        std::format("Unknown extension: {}", ext)));
}

Result<FileType> FileTypeDetector::detect_from_content(const Path& path) const {
    // Try magic number detection first
    auto magic_result = detect_from_magic(path);
    if (magic_result) return magic_result;
    
    // For text-based files, try content analysis
    if (fs::file_size(path) < 1024 * 1024) { // Only for files < 1MB
        auto content_result = detect_from_text_content(path);
        if (content_result) return content_result;
    }
    
    return make_unexpected(MAKE_ERROR(ErrorCode::UnsupportedFormat, "Content detection failed"));
}

Result<FileType> FileTypeDetector::detect_from_magic(const Path& path) const {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open file: {}", path.string())));
    }
    
    std::array<uint8_t, 64> buffer{};
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    std::streamsize bytes_read = file.gcount();
    
    if (bytes_read < 1) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Cannot read file"));
    }
    
    std::shared_lock<std::shared_mutex> lock(mutex_);
    for (const auto& sig : signatures_) {
        if (bytes_read < static_cast<std::streamsize>(sig.offset + sig.mask_length)) continue;
        
        bool match = true;
        for (uint8_t i = 0; i < sig.mask_length; ++i) {
            if ((buffer[static_cast<std::size_t>(sig.offset) + i] & 0xFF) != (sig.bytes[i] & 0xFF)) {
                match = false;
                break;
            }
        }
        
        if (match) {
            // In a real implementation, we would have a mapping from signatures to types
            // For now, we'll return Unknown for signatures with mask_length > 0
            // and try to infer the type from the signature for those with mask_length == 0
            if (sig.mask_length == 0) {
                // This is a simplified approach - in reality we'd need a proper mapping
                // For demonstration purposes, we'll hardcode a few known signatures
                if (sig.bytes[0] == 0x7F && sig.bytes[1] == 'E' && sig.bytes[2] == 'L' && sig.bytes[3] == 'F') {
                    return FileType::ELF64; // or ELF32 depending on context
                }
                if (sig.bytes[0] == 'M' && sig.bytes[1] == 'Z') {
                    return FileType::PE32; // or PE64 depending on context
                }
                if (sig.bytes[0] == 0x89 && sig.bytes[1] == 'P' && sig.bytes[2] == 'N' && sig.bytes[3] == 'G') {
                    return FileType::PNG;
                }
                if (sig.bytes[0] == 0xFF && sig.bytes[1] == 0xD8 && sig.bytes[2] == 0xFF) {
                    return FileType::JPG;
                }
                if (sig.bytes[0] == 'B' && sig.bytes[1] == 'M') {
                    return FileType::BMP;
                }
                if (sig.bytes[0] == 0x1F && sig.bytes[1] == 0x8B) {
                    return FileType::GZ;
                }
                if (sig.bytes[0] == 'B' && sig.bytes[1] == 'Z' && sig.bytes[2] == 'h') {
                    return FileType::BZ2;
                }
                if (sig.bytes[0] == 'P' && sig.bytes[1] == 'K' && 
                    sig.bytes[2] == 0x03 && sig.bytes[3] == 0x04) {
                    return FileType::ZIP;
                }
                // Add more known signatures as needed
            }
            // For signatures with mask_length > 0, we return Unknown since we don't have a mapping
            return make_unexpected(MAKE_ERROR(ErrorCode::UnsupportedFormat, "Signature matched but type not determined"));
        }
    }
    
    return make_unexpected(MAKE_ERROR(ErrorCode::UnsupportedFormat, "No magic match"));
}

Result<FileType> FileTypeDetector::detect_from_text_content(const Path& path) const {
    std::ifstream file(path);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open file: {}", path.string())));
    }
    
    std::string line;
    std::getline(file, line);
    
    // Check for shebang
    if (line.size() >= 2 && line[0] == '#' && line[1] == '!') {
        if (line.find("/bin/bash") != std::string::npos || 
            line.find("/usr/bin/env bash") != std::string::npos) {
            return FileType::SH;
        }
        if (line.find("/usr/bin/env python") != std::string::npos ||
            line.find("/usr/bin/python") != std::string::npos ||
            line.find("/bin/python") != std::string::npos) {
            return FileType::PY;
        }
        if (line.find("/usr/bin/env node") != std::string::npos ||
            line.find("/usr/bin/node") != std::string::npos) {
            return FileType::JS;
        }
    }
    
    // Check for common text file patterns
    if (line.find("<?xml") != std::string::npos) {
        return FileType::XML;
    }
    if (line.find("{") != std::string::npos && line.find("}") != std::string::npos) {
        // Might be JSON - need more analysis
        return FileType::JSON;
    }
    if (line.find("[") != std::string::npos && line.find("]") != std::string::npos &&
        line.find("=") != std::string::npos) {
        return FileType::INI;
    }
    
    return make_unexpected(MAKE_ERROR(ErrorCode::UnsupportedFormat, "Text pattern not recognized"));
}

void FileTypeDetector::register_signature(FileType type, const FileSignature& sig) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    // In a real implementation, we'd store signature->type mapping
    // For now, we'll just note that we could extend this
}

void FileTypeDetector::register_extension(FileType type, StringView ext) {
    std::string ext_str(ext);
    if (!ext_str.empty() && ext_str[0] != '.') {
        ext_str = "." + ext_str;
    }
    std::unique_lock<std::shared_mutex> lock(mutex_);
    extension_map_[StringUtils::to_lower(ext_str)] = type;
}

void FileTypeDetector::register_mime(FileType type, StringView mime) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    mime_map_[std::string(mime)] = type;
}

const std::unordered_map<std::string, FileType>& FileTypeDetector::extension_map() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return extension_map_;
}

const std::unordered_map<std::string, FileType>& FileTypeDetector::mime_map() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return mime_map_;
}

const std::vector<FileSignature>& FileTypeDetector::signatures() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return signatures_;
}

FileCategory FileTypeDetector::categorize_type(FileType type) const {
    switch (type) {
        case FileType::PE32:
        case FileType::PE64:
        case FileType::ELF32:
        case FileType::ELF64:
        case FileType::MachO32:
        case FileType::MachO64:
        case FileType::DLL:
        case FileType::SO:
        case FileType::DYLIB:
            return FileCategory::Executable;
            
        case FileType::DDS:
        case FileType::PNG:
        case FileType::JPG:
        case FileType::TGA:
        case FileType::BMP:
        case FileType::TIFF:
        case FileType::EXR:
        case FileType::HDR:
        case FileType::KTX:
        case FileType::ASTC:
        case FileType::PVR:
        case FileType::WebP:
            return FileCategory::Texture;
            
        case FileType::FBX:
        case FileType::OBJ:
        case FileType::GLTF:
        case FileType::GLB:
        case FileType::COLLADA:
        case FileType::USD:
        case FileType::USDZ:
        case FileType::BLEND:
        case FileType::MAX:
        case FileType::MA:
        case FileType::MB:
        case FileType::X:
        case FileType::MDL:
        case FileType::MD2:
        case FileType::MD3:
        case FileType::MD5:
        case FileType::NIF:
        case FileType::HKX:
        case FileType::GR2:
        case FileType::SMD:
        case FileType::DMX:
            return FileCategory::Model3D;
            
        case FileType::WAV:
        case FileType::OGG:
        case FileType::MP3:
        case FileType::FLAC:
        case FileType::AAC:
        case FileType::M4A:
        case FileType::WMA:
        case FileType::AIFF:
        case FileType::AU:
        case FileType::RAW:
        case FileType::XMA:
        case FileType::ATRAC:
        case FileType::FSB:
        case FileType::BNK:
        case FileType::PKG:
        case FileType::XWB:
        case FileType::XSB:
            return FileCategory::Audio;
            
        case FileType::LUA:
        case FileType::PY:
        case FileType::JS:
        case FileType::TS:
        case FileType::CS:
        case FileType::VB:
        case FileType::PL:
        case FileType::SH:
        case FileType::BAT:
        case FileType::PS1:
        case FileType::ANGELSCRIPT:
        case FileType::SQF:
        case FileType::PAWN:
        case FileType::SCPT:
        case FileType::CSC:
        case FileType::CSCRIPT:
            return FileCategory::Script;
            
        case FileType::HLSL:
        case FileType::GLSL:
        case FileType::SPIRV:
        case FileType::CG:
        case FileType::CGINC:
        case FileType::FX:
        case FileType::FXO:
        case FileType::SHADER:
        case FileType::VSH:
        case FileType::PSH:
        case FileType::GSH:
        case FileType::USH:
        case FileType::USF:
        case FileType::METAL:
        case FileType::MSL:
            return FileCategory::Shader;
            
        case FileType::PFX:
        case FileType::PTX:
        case FileType::PAR:
        case FileType::PARTICLE:
        case FileType::EMITTER:
            return FileCategory::Particle;
            
        case FileType::PHYSX:
        case FileType::HAVOK:
        case FileType::BULLET:
        case FileType::ODE:
        case FileType::NEWTON:
        case FileType::PHYSX3:
            return FileCategory::Physics;
            
        case FileType::ZIP:
        case FileType::RAR:
        case FileType::SEVENZ:
        case FileType::TAR:
        case FileType::GZ:
        case FileType::BZ2:
        case FileType::XZ:
        case FileType::ZST:
        case FileType::PAK:
        case FileType::PK3:
        case FileType::PK4:
        case FileType::VPK:
        case FileType::BSA:
        case FileType::BA2:
        case FileType::DAT:
        case FileType::IDX:
        case FileType::TOC:
        case FileType::WAD:
        case FileType::MPQ:
        case FileType::CASC:
        case FileType::FORGE:
        case FileType::BIG:
            return FileCategory::Archive;
            
        case FileType::SQLITE:
        case FileType::LEVELDB:
        case FileType::ROCKSDB:
        case FileType::LMDB:
        case FileType::BERKELEYDB:
            return FileCategory::Database;
            
        case FileType::INI:
        case FileType::XML:
        case FileType::JSON:
        case FileType::YAML:
        case FileType::TOML:
        case FileType::PROPERTIES:
            return FileCategory::Config;
            
        case FileType::TTF:
        case FileType::OTF:
        case FileType::WOFF:
        case FileType::WOFF2:
        case FileType::FNT:
        case FileType::BMFONT:
            return FileCategory::Font;
            
        case FileType::MP4:
        case FileType::AVI:
        case FileType::MKV:
        case FileType::MOV:
        case FileType::WMV:
        case FileType::FLV:
        case FileType::WEBM:
        case FileType::MPEG:
        case FileType::BINK:
        case FileType::BIK:
            return FileCategory::Video;
            
        case FileType::PDF:
        case FileType::TXT:
        case FileType::RTF:
        case FileType::MD:
        case FileType::HTML:
        case FileType::HTM:
        case FileType::CHM:
            return FileCategory::Document;
            
        default:
            return FileCategory::Unknown;
    }
}

} // namespace game_req