#include <game_req_analyzer/scanner/file_type_detector.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/utils/hash_utils.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <magic.h>
#include <fstream>
#include <cstring>

using namespace game_req;

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
    signatures_.push_back(FileType{{0x7f, 'E', 'L', 'F'}, 4, 0}); // ELF
    signatures_.push_back(FileType{{'M', 'Z'}, 2, 0}); // DOS/PE
    signatures_.push_back(FileType{{0xCF, 0xFA, 0xED, 0xFE}, 4, 0}); // Mach-O big endian
    signatures_.push_back(FileType{{0xFE, 0xED, 0xFA, 0xCF}, 4, 0}); // Mach-O little endian
    signatures_.push_back(FileType{{0xCA, 0xFE, 0xBA, 0xBE}, 4, 0}); // Java class
    
    // Archive signatures
    signatures_.push_back(FileType{{'P', 'K', 0x03, 0x04}, 4, 0}); // ZIP
    signatures_.push_back(FileType{{'R', 'a', 'r', '!'}, 4, 0}); // RAR
    signatures_.push_back(FileType{{0x1F, 0x8B}, 2, 0}); // GZIP
    signatures_.push_back(FileType{{'B', 'Z', 'h'}, 3, 0}); // BZIP2
    signatures_.push_back(FileType{{0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C}, 6, 0}); // 7z
    signatures_.push_back(FileType{{0xFD, '7', 'z', 'X', 'Z', 0x00}, 6, 0}); // XZ
    
    // Image/texture signatures
    signatures_.push_back(FileType{{0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A}, 8, 0}); // PNG
    signatures_.push_back(FileType{{0xFF, 0xD8, 0xFF}, 3, 0}); // JPEG
    signatures_.push_back(FileType{{'B', 'M'}, 2, 0}); // BMP
    signatures_.push_back(FileType{{'G', 'I', 'F', '8', '7', 'a'}, 6, 0}); // GIF87a
    signatures_.push_back(FileType{{'G', 'I', 'F', '8', '9', 'a'}, 6, 0}); // GIF89a
    signatures_.push_back(FileType{{'T', 'I', 'I', 'F'}, 4, 0}); // TIFF (big endian)
    signatures_.push_back(FileType{{'I', 'I', '*', '\x00'}, 4, 0}); // TIFF (little endian)
    signatures_.push_back(FileType{{'D', 'D', 'S', ' '}, 4, 0}); // DDS
    signatures_.push_back(FileType{{0x89, 'K', 'T', 'X', ' ', '1', '1', 0x0B}, 8, 0}); // KTX
    
    // Audio signatures
    signatures_.push_back(FileType{{'R', 'I', 'F', 'F', 0x00, 0x00, 0x00, 0x00, 'W', 'A', 'V', 'E'}, 12, 0}); // WAV
    signatures_.push_back(FileType{{'O', 'g', 'g', 'S'}, 4, 0}); // OGG
    signatures_.push_back(FileType{{'f', 'L', 'a', 'C'}, 4, 0}); // FLAC
    signatures_.push_back(FileType{{'I', 'D', '3'}, 3, 0}); // ID3 tag (MP3)
    signatures_.push_back(FileType{{0xFF, 0xFB}, 2, 0}); // MP3
    signatures_.push_back(FileType{{0xFF, 0xF3}, 2, 0}); // MP3
    signatures_.push_back(FileType{{0xFF, 0xF2}, 2, 0}); // MP3
    
    // 3D model signatures
    signatures_.push_back(FileType{{'f', 'b', 'x'}, 3, 0}); // FBX (binary)
    signatures_.push_back(FileType{{"Kaydara FBX Binary  "}, 20, 0}); // FBX binary header
    
    // Script signatures (shebangs)
    signatures_.push_back(FileType{{'#', '!', '/', 'b', 'i', 'n', '/', 'b', 'a', 's', 'h'}, 10, 0});
    signatures_.push_back(FileType{{'#', '!', '/', 'u', 's', 'r', '/', 'e', 'n', 'v', ' ', 'p', 'y', 't', 'h', 'o', 'n'}, 16, 0});
    signatures_.push_back(FileType{{'#', '!', '/', 'b', 'i', 'n', '/', 's', 'h'}, 9, 0});
    
    // Shader signatures
    signatures_.push_back(FileType{{/* DXBC */ 0x44, 0x4B, 0x43, 0x42}, 4, 0});
    signatures_.push_back(FileType{{/* SPIR-V */ 0x03, 0x02, 0x23, 0x07}, 4, 0});
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
    register_extension(FileType::_7Z, ".7z");
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
    register_extension(FileType::MPG, ".mpg");
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
                info.symlink_target = fs::read_symlink(path);
            }
            return info; // Return basic info for directories/symlinks
        }
        
        info.size = fs::file_size(path);
        info.modified_time = fs::last_write_time(path);
        info.created_time = fs::last_write_time(path); // Approximation
        
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
        info.extension = fs::extension(path).string();
        
        // Try to get MIME type
        info.mime_type = get_mime_type(path).value_or("application/octet-stream");
        
        return info;
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Detection failed: {}", e.what())));
    }
}

Result<FileType> FileTypeDetector::detect_from_extension(const Path& path) const {
    String ext = StringUtils::to_lower(fs::extension(path).string());
    if (ext.empty()) return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "No extension"));
    
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
    
    std::array<u8, 64> buffer{};
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    std::streamsize bytes_read = file.gcount();
    
    if (bytes_read < 1) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Cannot read file"));
    }
    
    std::shared_lock<std::shared_mutex> lock(mutex_);
    for (const auto& sig : signatures_) {
        if (bytes_read < sig.offset + sig.mask_length) continue;
        
        bool match = true;
        for (u8 i = 0; i < sig.mask_length; ++i) {
            if ((buffer[sig.offset + i] & 0xFF) != (sig.bytes[i] & 0xFF)) {
                match = false;
                break;
            }
        }
        
        if (match) {
            return sig.mask_length > 0 ? std::make_optional(FileType::Unknown) : std::make_optional(sig.type);
            // Note: In a real implementation, we'd need to map signatures to types
            // This is simplified - we'd need a signature->type mapping
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
    mime_map_[String(mime)] = type;
}

const std::unordered_map<String, FileType>& FileTypeDetector::extension_map() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return extension_map_;
}

const std::unordered_map<String, FileType>& FileTypeDetector::mime_map() const {
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
            return FileCategory::Executable;
            
        case FileType::DLL:
        case FileType::SO:
        case FileType::DYLIB:
            return FileCategory::Library;
            
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
        case FileType::_7Z:
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
        case FileType::MPG:
        case FileType::BINK:
        case FileType::BIK:
            return FileCategory::Video;
            
        case FileType::PDF:
        case FileType::TXT:
        case FileType::RTF:
        case FileType::MD:
        case FileType::HTML:
        case FileType::CHM:
            return FileCategory::Document;
            
        default:
            return FileCategory::Unknown;
    }
}

String FileInfo::category_name() const {
    switch (category) {
        case FileCategory::Executable: return "Executable";
        case FileCategory::Library: return "Library";
        case FileCategory::Texture: return "Texture";
        case FileCategory::Model3D: return "3D Model";
        case FileCategory::Audio: return "Audio";
        case FileCategory::Script: return "Script";
        case FileCategory::Shader: return "Shader";
        case FileCategory::Particle: return "Particle";
        case FileCategory::Physics: return "Physics";
        case FileCategory::AI: return "AI";
        case FileCategory::Network: return "Network";
        case FileCategory::Config: return "Config";
        case FileCategory::Archive: return "Archive";
        case FileCategory::Database: return "Database";
        case FileCategory::Font: return "Font";
        case FileCategory::Video: return "Video";
        case FileCategory::Document: return "Document";
        case FileCategory::Unknown: return "Unknown";
        default: return "Other";
    }
}

String FileInfo::type_name() const {
    // Simplified - in reality we'd have a mapping from FileType to string
    switch (type) {
        case FileType::PE32: return "PE32 Executable";
        case FileType::PE64: return "PE64 Executable";
        case FileType::ELF32: return "ELF32 Executable";
        case FileType::ELF64: return "ELF64 Executable";
        case FileType::MachO32: return "Mach-O 32 Executable";
        case FileType::MachO64: return "Mach-O 64 Executable";
        case FileType::DLL: return "Dynamic Link Library";
        case FileType::SO: return "Shared Object";
        case FileType::DYLIB: return "Dynamic Library";
        case FileType::DDS: return "DirectDraw Surface";
        case FileType::PNG: return "Portable Network Graphics";
        case FileType::JPG: return "JPEG Image";
        case FileType::TGA: return "Truevision TGA";
        case FileType::BMP: return "Bitmap Image";
        case FileType::TIFF: return "Tagged Image File Format";
        case FileType::EXR: return "OpenEXR Image";
        case FileType::HDR: return "High Dynamic Range Image";
        case FileType::KTX: return "Khronos Texture";
        case FileType::ASTC: return "Adaptive Scalable Texture Compression";
        case FileType::PVR: return "PowerVR Texture";
        case FileType::WebP: return "WebP Image";
        case FileType::FBX: return "Autodesk FBX";
        case FileType::OBJ: return "Wavefront OBJ";
        case FileType::GLTF: return "GL Transmission Format";
        case FileType::GLB: return "GLB Binary";
        case FileType::COLLADA: return "COLLADA";
        case FileType::USD: return "Universal Scene Description";
        case FileType::USDZ: return "USDZ Archive";
        case FileType::BLEND: return "Blender File";
        case FileType::MAX: return "3ds Max File";
        case FileType::MA: return "Maya ASCII";
        case FileType::MB: return "Maya Binary";
        case FileType::X: return "DirectX X File";
        case FileType::MDL: return "Model File";
        case FileType::NIF: return "NetImmerse/Gamebryo File";
        case FileType::HKX: return "Havok File";
        case FileType::GR2: return "Granny 2 File";
        case FileType::SMD: return "Studio Model";
        case FileType::DMX: return "Source Engine Model";
        case FileType::WAV: return "Waveform Audio";
        case FileType::OGG: return "Ogg Vorbis";
        case FileType::MP3: return "MPEG Audio Layer III";
        case FileType::FLAC: return "Free Lossless Audio Codec";
        case FileType::AAC: return "Advanced Audio Coding";
        case FileType::M4A: return "MPEG-4 Audio";
        case FileType::WMA: return "Windows Media Audio";
        case FileType::AIFF: return "Audio Interchange File Format";
        case FileType::AU: return "Sun/NeXT Audio";
        case FileType::RAW: return "Raw Audio";
        case FileType::XMA: return "XMA Audio";
        case FileType::ATRAC: return "ATRAC Audio";
        case FileType::FSB: return "FMOD Sample Bank";
        case FileType::BNK: return "Audio Bank";
        case FileType::PKG: return "Audio Package";
        case FileType::XWB: return "Xbox Wave Bank";
        case FileType::XSB: return "Xbox Sound Bank";
        case FileType::LUA: return "Lua Script";
        case FileType::PY: return "Python Script";
        case FileType::JS: return "JavaScript";
        case FileType::TS: return "TypeScript";
        case FileType::CS: return "C# Script";
        case FileType::VB: return "Visual Basic";
        case FileType::PL: return "Perl Script";
        case FileType::SH: return "Shell Script";
        case FileType::BAT: return "Batch File";
        case FileType::PS1: return "PowerShell Script";
        case FileType::ANGELSCRIPT: return "AngelScript";
        case FileType::SQF: return "SQF Script";
        case FileType::PAWN: return "Pawn Script";
        case FileType::SCPT: return "Script";
        case FileType::CSC: return "Compiled Script";
        case FileType::CSCRIPT: return "Compiled Script";
        case FileType::HLSL: return "High Level Shader Language";
        case FileType::GLSL: return "OpenGL Shading Language";
        case FileType::SPIRV: return "SPIR-V";
        case FileType::CG: return "C for Graphics";
        case FileType::CGINC: return "Cg Include";
        case FileType::FX: return "Effect File";
        case FileType::FXO: return "Compiled Effect";
        case FileType::SHADER: return "Shader File";
        case FileType::VSH: return "Vertex Shader";
        case FileType::PSH: return "Pixel Shader";
        case FileType::GSH: return "Geometry Shader";
        case FileType::USH: return "Union Shader";
        case FileType::USF: return "Union Shader Fragment";
        case FileType::METAL: return "Metal Shading Language";
        case FileType::MSL: return "Metal Shading Language";
        case FileType::PFX: return "Particle Effect";
        case FileType::PTX: return "Particle Texture";
        case FileType::PAR: return "Particle Archive";
        case FileType::PARTICLE: return "Particle File";
        case FileType::EMITTER: return "Particle Emitter";
        case FileType::PHYSX: return "PhysX File";
        case FileType::HAVOK: return "Havok Physics";
        case FileType::BULLET: return "Bullet Physics";
        case FileType::ODE: return "Open Dynamics Engine";
        case FileType::NEWTON: return "Newton Physics";
        case FileType::PHYSX3: return "PhysX 3";
        case FileType::ZIP: return "ZIP Archive";
        case FileType::RAR: return "RAR Archive";
        case FileType::_7Z: return "7-Zip Archive";
        case FileType::TAR: return "TAR Archive";
        case FileType::GZ: return "GZIP Archive";
        case FileType::BZ2: return "BZIP2 Archive";
        case FileType::XZ: return "XZ Archive";
        case FileType::ZST: return "Zstandard Archive";
        case FileType::PAK: return "PAK Archive";
        case FileType::PK3: return "Quake 3 Package";
        case FileType::PK4: return "Quake 4 Package";
        case FileType::VPK: return "Valve Package";
        case FileType::BSA: return "Bethesda Softworks Archive";
        case FileType::BA2: return "Bethesda Archive 2";
        case FileType::DAT: return "Data File";
        case FileType::IDX: return "Index File";
        case FileType::TOC: return "Table of Contents";
        case FileType::WAD: return "Where's All the Data?";
        case FileType::MPQ: return "Mo'PaQ Archive";
        case FileType::CASC: return "Blizzard CASC";
        case FileType::FORGE: return "Minecraft Forge";
        case FileType::BIG: return "BIG Archive";
        case FileType::SQLITE: return "SQLite Database";
        case FileType::LEVELDB: return "LevelDB Database";
        case FileType::ROCKSDB: return "RocksDB Database";
        case FileType::LMDB: return "Lightning Memory-Mapped Database";
        case FileType::BERKELEYDB: return "Berkeley DB";
        case FileType::INI: return "INI Configuration";
        case FileType::XML: return "XML File";
        case FileType::JSON: return "JSON File";
        case FileType::YAML: return "YAML File";
        case FileType::TOML: return "TOML File";
        case FileType::PROPERTIES: return "Properties File";
        case FileType::TTF: return "TrueType Font";
        case FileType::OTF: return "OpenType Font";
        case FileType::WOFF: return "Web Open Font Format";
        case FileType::WOFF2: return "Web Open Font Format 2";
        case FileType::FNT: return "Font File";
        case FileType::BMFONT: return "Bitmap Font";
        case FileType::MP4: return "MPEG-4 Video";
        case FileType::AVI: return "Audio Video Interleave";
        case FileType::MKV: return "Matroska Video";
        case FileType::MOV: return "QuickTime Movie";
        case FileType::WMV: return "Windows Media Video";
        case FileType::FLV: return "Flash Video";
        case FileType::WEBM: return "WebM Video";
        case FileType::MPEG: return "MPEG Video";
        case FileType::MPG: return "MPEG Video";
        case FileType::BINK: return "Bink Video";
        case FileType::BIK: return "Bink Video";
        case FileType::PDF: return "PDF Document";
        case FileType::TXT: return "Text File";
        case FileType::RTF: return "Rich Text Format";
        case FileType::MD: return "Markdown Document";
        case FileType::HTML: return "HTML Document";
        case FileType::CHM: return "Compiled HTML Help";
        default: return "Unknown Type";
    }
}

} // namespace game_req