#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>

namespace game_req {

struct TextureInfo {
    u32 width = 0;
    u32 height = 0;
    u32 depth = 1;
    u32 mip_levels = 1;
    u32 array_size = 1;
    String format;
    u32 block_size = 0;
    bool is_compressed = false;
    bool is_cubemap = false;
    bool is_volume = false;
    bool has_alpha = false;
    bool is_srgb = false;
    u64 estimated_vram = 0;
    u64 disk_size = 0;
    f32 compression_ratio = 1.0f;
    String compression_format;
    std::vector<u32> mip_sizes;
};

struct TextureStats {
    u64 total_textures = 0;
    u64 total_vram = 0;
    u64 total_disk_size = 0;
    std::unordered_map<String, u32> format_counts;
    std::unordered_map<String, u32> compression_counts;
    std::unordered_map<u32, u32> resolution_counts;
    u32 max_width = 0;
    u32 max_height = 0;
    f32 avg_compression_ratio = 1.0f;
    
    [[nodiscard]] String summary() const;
};

class TextureAnalyzer {
public:
    explicit TextureAnalyzer(const AnalyzerConfig& config);
    
    Result<std::vector<TextureInfo>> analyze(const std::vector<FileInfo>& textures);
    Result<TextureInfo> analyze_single(const FileInfo& texture);
    Result<TextureInfo> analyze_texture(const Path& file_path);
    [[nodiscard]] TextureStats stats() const;
    [[nodiscard]] String generate_report() const;

private:
    AnalyzerConfig config_;
    TextureStats stats_;
    mutable std::mutex stats_mutex_;
    
    Result<TextureInfo> analyze_dds(const Path& path);
    Result<TextureInfo> analyze_ktx(const Path& path);
    Result<TextureInfo> analyze_pvr(const Path& path);
    Result<TextureInfo> analyze_astc(const Path& path);
    Result<TextureInfo> analyze_png(const Path& path);
    Result<TextureInfo> analyze_jpg(const Path& path);
    Result<TextureInfo> analyze_tga(const Path& path);
    Result<TextureInfo> analyze_bmp(const Path& path);
    Result<TextureInfo> analyze_exr(const Path& path);
    Result<TextureInfo> analyze_hdr(const Path& path);
    Result<TextureInfo> analyze_webp(const Path& path);
    Result<TextureInfo> analyze_tiff(const Path& path);
    Result<TextureInfo> analyze_generic(const Path& path, FileType type);

    u64 calculate_vram(const TextureInfo& tex) const;
    u32 calculate_mip_size(u32 w, u32 h, u32 block_size) const;
    String detect_compression_format(const std::vector<u8>& header, FileType type) const;
    u64 file_size(const Path& path) const;
};

struct TextureFormatInfo {
    String name;
    u32 block_width = 4;
    u32 block_height = 4;
    u32 block_depth = 1;
    u32 bits_per_block = 0;
    bool is_compressed = false;
    bool is_srgb = false;
    bool has_alpha = false;
};

class TextureFormatDatabase {
public:
    static TextureFormatDatabase& instance();
    
    [[nodiscard]] const TextureFormatInfo* find(StringView format) const;
    [[nodiscard]] const TextureFormatInfo* find_by_fourcc(u32 fourcc) const;
    [[nodiscard]] const TextureFormatInfo* find_by_dxgi_format(u32 dxgi_format) const;
    [[nodiscard]] const TextureFormatInfo* find_by_vk_format(u32 vk_format) const;
    
    void register_format(const TextureFormatInfo& info);

private:
    TextureFormatDatabase();
    std::unordered_map<String, TextureFormatInfo> formats_;
    std::unordered_map<u32, const TextureFormatInfo*> fourcc_map_;
    std::unordered_map<u32, const TextureFormatInfo*> dxgi_map_;
    std::unordered_map<u32, const TextureFormatInfo*> vk_map_;
    mutable std::shared_mutex mutex_;
};

} // namespace game_req