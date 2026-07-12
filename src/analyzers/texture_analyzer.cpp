#include <game_req_analyzer/analyzers/texture_analyzer.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <cmath>
#include <iomanip>
#include <sstream>

#if defined(HAVE_LIBPNG) || defined(PNG_FOUND)
#include <png.h>
#endif

namespace game_req {

TextureAnalyzer::TextureAnalyzer(const AnalyzerConfig& config) : config_(config), stats_() {}

Result<std::vector<TextureInfo>> TextureAnalyzer::analyze(const std::vector<FileInfo>& textures) {
    std::vector<TextureInfo> results;
    results.reserve(textures.size());
    
    for (const auto& file : textures) {
        if (file.category == FileCategory::Texture) {
            auto result = analyze_single(file);
            if (result) {
                results.push_back(*result);
                
                // Update statistics
                {
                    std::lock_guard<std::mutex> lock(stats_mutex_);
                    stats_.total_textures++;
                    stats_.total_disk_size += result->disk_size;
                    stats_.total_vram += result->estimated_vram;
                    
                    // Update format counts
                    stats_.format_counts[result->format]++;
                    
                    // Update resolution counts
                    u64 resolution_key = static_cast<u64>(result->width) * result->height;
                    stats_.resolution_counts[resolution_key]++;
                    
                    // Update max resolution
                    if (result->width > stats_.max_width) stats_.max_width = result->width;
                    if (result->height > stats_.max_height) stats_.max_height = result->height;
                }
            } else {
                LOG_WARN("Failed to analyze texture {}: {}", file.path.string(), result.error().to_string());
            }
        }
    }
    
    // Calculate averages
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        if (stats_.total_textures > 0) {
            stats_.avg_compression_ratio = 1.0f; // Placeholder
        }
    }
    
    return results;
}

Result<TextureInfo> TextureAnalyzer::analyze_single(const FileInfo& texture) {
    return analyze_texture(texture.path);
}

Result<TextureInfo> TextureAnalyzer::analyze_texture(const Path& file_path) {
    try {
        // Detect file type
        auto detect_result = FileTypeDetector::instance().detect(file_path);
        if (!detect_result) {
            return make_unexpected(detect_result.error());
        }
        FileType type = detect_result->type;
        
        // Dispatch to appropriate analyzer based on file type
        switch (type) {
            case FileType::DDS:
                return analyze_dds(file_path);
            case FileType::PNG:
                return analyze_png(file_path);
            case FileType::JPG:
                return analyze_jpg(file_path);
            case FileType::BMP:
                return analyze_bmp(file_path);
            case FileType::TGA:
                return analyze_tga(file_path);
            case FileType::TIFF:
                return analyze_tiff(file_path);
            case FileType::EXR:
                return analyze_exr(file_path);
            case FileType::HDR:
                return analyze_hdr(file_path);
            case FileType::KTX:
                return analyze_ktx(file_path);
            case FileType::PVR:
                return analyze_pvr(file_path);
            case FileType::WebP:
                return analyze_webp(file_path);
            default:
                return analyze_generic(file_path, type);
        }
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Texture analysis failed: {}", e.what())));
    }
}

Result<TextureInfo> TextureAnalyzer::analyze_dds(const Path& file_path) {
    TextureInfo info;
    info.format = "DDS";
    
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open file: {}", file_path.string())));
    }
    
    // Read DDS header
    char header[124];
    file.read(header, sizeof(header));
    
    if (!file || strncmp(header, "DDS ", 4) != 0) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidFileFormat, 
            std::format("Invalid DDS file: {}", file_path.string())));
    }
    
    // Extract header information
    u32 height = *reinterpret_cast<u32*>(&header[8]);
    u32 width = *reinterpret_cast<u32*>(&header[12]);
    u32 pitch_or_linear_size = *reinterpret_cast<u32*>(&header[16]);
    u32 depth = *reinterpret_cast<u32*>(&header[20]);
    u32 mip_map_count = *reinterpret_cast<u32*>(&header[24]);
    u32 flags = *reinterpret_cast<u32*>(&header[36]);
    u32 four_cc = *reinterpret_cast<u32*>(&header[80]);
    
    // Determine format based on FourCC
    switch (four_cc) {
        case MAKEFOURCC('D', 'X', 'T', '1'):
            info.format = "DXT1";
            info.is_compressed = true;
            info.block_size = 8;
            break;
        case MAKEFOURCC('D', 'X', 'T', '3'):
            info.format = "DXT3";
            info.is_compressed = true;
            info.block_size = 16;
            break;
        case MAKEFOURCC('D', 'X', 'T', '5'):
            info.format = "DXT5";
            info.is_compressed = true;
            info.block_size = 16;
            break;
        default:
            // For other formats, calculate from pitch
            if (pitch_or_linear_size > 0 && height > 0) {
                info.block_size = pitch_or_linear_size / width;
            } else {
                info.block_size = 4; // Default RGBA8
            }
            break;
    }
    
    info.width = width;
    info.height = height;
    info.depth = depth > 1 ? depth : 1;
    info.mip_levels = std::max<u32>(1, mip_map_count);
    
    // Calculate sizes
    info.disk_size = file_size(file_path);
    
    // Calculate mip map sizes
    u32 w = width;
    u32 h = height;
    for (u32 i = 0; i < info.mip_levels; i++) {
        u32 size = std::max<u32>(1, w) * std::max<u32>(1, h) * info.block_size;
        info.mip_sizes.push_back(size);
        w = std::max<u32>(1, w / 2);
        h = std::max<u32>(1, h / 2);
    }
    
    // Estimate VRAM usage
    info.estimated_vram = calculate_vram(info);
    
    return info;
}

Result<TextureInfo> TextureAnalyzer::analyze_png(const Path& file_path) {
#if defined(HAVE_LIBPNG) || defined(PNG_FOUND)
    TextureInfo info;
    info.format = "PNG";
    
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open file: {}", file_path.string())));
    }
    
    // Read PNG header
    char header[8];
    file.read(header, sizeof(header));
    
    if (!file || png_sig_cmp(reinterpret_cast<png_const_bytep>(header), 0, 8) != 0) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidFileFormat, 
            std::format("Invalid PNG file: {}", file_path.string())));
    }
    
    // In a real implementation, we would use libpng to get detailed info
    // For now, we'll just get basic file info
    info.width = 0; // Placeholder
    info.height = 0; // Placeholder
    info.block_size = 4; // Assume RGBA8
    
    info.disk_size = file_size(file_path);
    info.estimated_vram = info.disk_size * 2; // Rough estimate
    
    return info;
#else
    (void)file_path;
    return make_unexpected(MAKE_ERROR(ErrorCode::FeatureUnavailable, "PNG analysis requires libpng"));
#endif
}

Result<TextureInfo> TextureAnalyzer::analyze_jpg(const Path& file_path) {
    TextureInfo info;
    info.format = "JPEG";
    
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open file: {}", file_path.string())));
    }
    
    // Read JPEG markers
    char marker[2];
    file.read(marker, sizeof(marker));
    
    if (!file || marker[0] != 0xFF || marker[1] != 0xD8) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidFileFormat, 
            std::format("Invalid JPEG file: {}", file_path.string())));
    }
    
    // In a real implementation, we would parse JPEG segments to get dimensions
    // For now, we'll just get basic file info
    info.width = 0; // Placeholder
    info.height = 0; // Placeholder
    info.block_size = 3; // Assume RGB8
    
    info.disk_size = file_size(file_path);
    info.estimated_vram = info.disk_size * 2; // Rough estimate
    
    return info;
}

Result<TextureInfo> TextureAnalyzer::analyze_bmp(const Path& file_path) {
    TextureInfo info;
    info.format = "BMP";
    
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open file: {}", file_path.string())));
    }
    
    // Read BMP header
    char header[54];
    file.read(header, sizeof(header));
    
    if (!file || *reinterpret_cast<u16*>(header) != 0x4D42) { // 'BM'
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidFileFormat, 
            std::format("Invalid BMP file: {}", file_path.string())));
    }
    
    // Extract header information
    u32 width = *reinterpret_cast<u32*>(&header[18]);
    u32 height = *reinterpret_cast<u32*>(&header[22]);
    u16 bit_count = *reinterpret_cast<u16*>(&header[28]);
    u32 compression = *reinterpret_cast<u32*>(&header[30]);
    
    info.width = width;
    info.height = height;
    info.is_compressed = (compression != 0);
    
    // Determine format based on bit depth
    if (bit_count == 1) {
        info.format = "MONO";
        info.block_size = 1;
    } else if (bit_count == 4) {
        info.format = "COLOR4";
        info.block_size = 1;
    } else if (bit_count == 8) {
        info.format = "COLOR8";
        info.block_size = 1;
    } else if (bit_count == 16) {
        info.format = "RGB565";
        info.block_size = 2;
    } else if (bit_count == 24) {
        info.format = "RGB8";
        info.block_size = 3;
    } else if (bit_count == 32) {
        info.format = "RGBA8";
        info.block_size = 4;
    }
    
    info.disk_size = file_size(file_path);
    info.estimated_vram = calculate_vram(info);
    
    return info;
}

// Placeholder implementations for other texture formats
Result<TextureInfo> TextureAnalyzer::analyze_tga(const Path& file_path) {
    return analyze_generic(file_path, FileType::TGA);
}

Result<TextureInfo> TextureAnalyzer::analyze_tiff(const Path& file_path) {
    return analyze_generic(file_path, FileType::TIFF);
}

Result<TextureInfo> TextureAnalyzer::analyze_exr(const Path& file_path) {
    return analyze_generic(file_path, FileType::EXR);
}

Result<TextureInfo> TextureAnalyzer::analyze_hdr(const Path& file_path) {
    return analyze_generic(file_path, FileType::HDR);
}

Result<TextureInfo> TextureAnalyzer::analyze_ktx(const Path& file_path) {
    return analyze_generic(file_path, FileType::KTX);
}

Result<TextureInfo> TextureAnalyzer::analyze_pvr(const Path& file_path) {
    return analyze_generic(file_path, FileType::PVR);
}

Result<TextureInfo> TextureAnalyzer::analyze_webp(const Path& file_path) {
    return analyze_generic(file_path, FileType::WebP);
}

Result<TextureInfo> TextureAnalyzer::analyze_generic(const Path& file_path, FileType type) {
    TextureInfo info;
    
    // Map FileType to format string
    switch (type) {
        case FileType::PNG: info.format = "PNG"; break;
        case FileType::JPG: info.format = "JPEG"; break;
        case FileType::BMP: info.format = "BMP"; break;
        case FileType::TGA: info.format = "TGA"; break;
        case FileType::TIFF: info.format = "TIFF"; break;
        case FileType::EXR: info.format = "EXR"; break;
        case FileType::HDR: info.format = "HDR"; break;
        case FileType::KTX: info.format = "KTX"; break;
        case FileType::PVR: info.format = "PVR"; break;
        case FileType::WebP: info.format = "WEBP"; break;
        case FileType::DDS: info.format = "DDS"; break;
        default: info.format = "UNKNOWN"; break;
    }
    
    // Get basic file info
    info.disk_size = file_size(file_path);
    info.width = 0; // Placeholder
    info.height = 0; // Placeholder
    info.block_size = 4; // Assume RGBA8 as default
    info.estimated_vram = calculate_vram(info);
    
    return info;
}

u64 TextureAnalyzer::calculate_vram(const TextureInfo& tex) const {
    if (tex.width == 0 || tex.height == 0) return 0;
    
    u64 base_size = static_cast<u64>(tex.width) * tex.height * tex.block_size;
    
    // Account for mip levels
    u64 total = 0;
    u32 w = tex.width;
    u32 h = tex.height;
    for (u32 i = 0; i < tex.mip_levels; i++) {
        total += static_cast<u64>(std::max<u32>(1, w)) * std::max<u32>(1, h) * tex.block_size;
        w = std::max<u32>(1, w / 2);
        h = std::max<u32>(1, h / 2);
    }
    
    // Account for depth, array size, cubemap faces
    total *= tex.depth * tex.array_size * (tex.is_cubemap ? 6 : 1);
    
    return total;
}

u32 TextureAnalyzer::calculate_mip_size(u32 w, u32 h, u32 block_size) const {
    return std::max<u32>(1, w) * std::max<u32>(1, h) * block_size;
}

String TextureAnalyzer::detect_compression_format(const std::vector<u8>& header, FileType type) const {
    (void)header;
    (void)type;
    return "Unknown";
}

u64 TextureAnalyzer::file_size(const Path& path) const {
    try {
        return fs::file_size(path);
    } catch (...) {
        return 0;
    }
}

} // namespace game_req