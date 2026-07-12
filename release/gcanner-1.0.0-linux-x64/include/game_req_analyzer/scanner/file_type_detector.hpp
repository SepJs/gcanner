#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/core/config.hpp>

namespace game_req {

enum class FileCategory : u8 {
    Unknown = 0,
    Executable,
    Library,
    Texture,
    Model3D,
    Audio,
    Script,
    Shader,
    Particle,
    Physics,
    AI,
    Network,
    Config,
    Archive,
    Database,
    Font,
    Video,
    Document,
    Other
};

enum class FileType : u16 {
    Unknown = 0,
    
    // Executables
    PE32, PE64, ELF32, ELF64, MachO32, MachO64,
    
    // Libraries
    DLL, SO, DYLIB,
    
    // Textures
    DDS, PNG, JPG, TGA, BMP, TIFF, EXR, HDR, KTX, ASTC, PVR, WebP,
    
    // 3D Models
    FBX, OBJ, GLTF, GLB, COLLADA, USD, USDZ, BLEND, MAX, MA, MB,
    X, MDL, MD2, MD3, MD5, NIF, HKX, GR2, SMD, DMX,
    
    // Audio
    WAV, OGG, MP3, FLAC, AAC, M4A, WMA, AIFF, AU, RAW, XMA, ATRAC,
    FSB, BNK, PKG, XWB, XSB,
    
    // Scripts
    LUA, PY, JS, TS, CS, VB, PL, SH, BAT, PS1, ANGELSCRIPT, SQF,
    PAWN, SCPT, CSC, CSCRIPT,
    
    // Shaders
    HLSL, GLSL, SPIRV, CG, CGINC, FX, FXO, SHADER, VSH, PSH, GSH,
    USH, USF, METAL, MSL,
    
    // Particles
    PFX, PTX, PAR, PARTICLE, EMITTER,
    
    // Physics
    PHYSX, HAVOK, BULLET, ODE, NEWTON, PHYSX3, PXD, PXB,
    
    // Archives
    ZIP, RAR, SEVENZ, TAR, GZ, BZ2, XZ, ZST, PAK, PK3, PK4, VPK,
    BSA, BA2, DAT, IDX, TOC, WAD, MPQ, CASC, FORGE, BIG,
    
    // Databases
    SQLITE, LEVELDB, ROCKSDB, LMDB, BERKELEYDB,
    
    // Config
    INI, XML, JSON, YAML, TOML, CFG, CONF, PROPERTIES,
    
    // Fonts
    TTF, OTF, WOFF, WOFF2, FNT, BMFONT,
    
    // Video
    MP4, AVI, MKV, MOV, WMV, FLV, WEBM, MPEG, BINK, BIK,
    
    // Documents
    PDF, TXT, RTF, MD, HTML, HTM, CHM,
};

struct FileSignature {
    std::array<u8, 16> bytes{};
    u8 mask_length = 0;
    u8 offset = 0;
};

struct FileInfo {
    Path path;
    u64 size = 0;
    FileCategory category = FileCategory::Unknown;
    FileType type = FileType::Unknown;
    String mime_type;
    String extension;
    String filename;
    bool is_archive = false;
    bool is_compressed = false;
    f32 compression_ratio = 1.0f;
    u64 uncompressed_size = 0;
    u64 compressed_size = 0;
    std::array<u8, 32> hash{};
    bool hash_valid = false;
    TimePoint modified_time;
    TimePoint created_time;
    u32 permissions = 0;
    bool is_symlink = false;
    Path symlink_target;
    String error_message;
    
    [[nodiscard]] String category_name() const;
    [[nodiscard]] String type_name() const;
    [[nodiscard]] bool is_valid() const { return category != FileCategory::Unknown; }
};

class FileTypeDetector {
public:
    static FileTypeDetector& instance();
    
    Result<FileInfo> detect(const Path& path) const;
    Result<FileType> detect_from_extension(const Path& path) const;
    Result<FileType> detect_from_content(const Path& path) const;
    Result<FileType> detect_from_magic(const Path& path) const;
    Result<FileType> detect_from_text_content(const Path& path) const; // Added missing declaration
    
    void register_signature(FileType type, const FileSignature& sig);
    void register_extension(FileType type, StringView ext);
    void register_mime(FileType type, StringView mime);
    
    [[nodiscard]] FileCategory categorize_type(FileType type) const;
    
    [[nodiscard]] const std::unordered_map<String, FileType>& extension_map() const;
    [[nodiscard]] const std::unordered_map<String, FileType>& mime_map() const;
    [[nodiscard]] const std::vector<FileSignature>& signatures() const;

private:
    FileTypeDetector();
    void init_default_signatures();
    void init_default_extensions();
    void init_default_mimes();
    
    std::unordered_map<String, FileType> extension_map_;
    std::unordered_map<String, FileType> mime_map_;
    std::vector<FileSignature> signatures_;
    mutable std::shared_mutex mutex_;
};

} // namespace game_req