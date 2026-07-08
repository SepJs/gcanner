#include <game_req_analyzer/analyzers/texture_analyzer.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/hash_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/utils/math_utils.hpp>
#include <algorithm>
#include <fstream>
#include <vector>
#include <cstring>

using namespace game_req;

TextureAnalyzer::TextureAnalyzer(const AnalyzerConfig& config) : config_(config) {
    // Initialize texture format database with common formats
    TextureFormatDatabase::instance().register_format({ "DXT1", 4, 4, 1, 64, true, false, true });
    TextureFormatDatabase::instance().register_format({ "DXT3", 4, 4, 1, 128, true, false, true });
    TextureFormatDatabase::instance().register_format({ "DXT5", 4, 4, 1, 128, true, false, true });
    TextureFormatDatabase::instance().register_format({ "BC4", 4, 4, 1, 64, true, false, false });
    TextureFormatDatabase::instance().register_format({ "BC5", 4, 4, 1, 128, true, false, false });
    TextureFormatDatabase::instance().register_format({ "BC6H", 4, 4, 1, 128, true, false, false });
    TextureFormatDatabase::instance().register_format({ "BC7", 4, 4, 1, 128, true, false, true });
    TextureFormatDatabase::instance().register_format({ "ASTC 4x4", 4, 4, 1, 128, true, false, false });
    TextureFormatDatabase::instance().register_format({ "ASTC 6x6", 6, 6, 1, 128, true, false, false });
    TextureFormatDatabase::instance().register_format({ "ASTC 8x8", 8, 8, 1, 128, true, false, false });
    TextureFormatDatabase::instance().register_format({ "RGBA8", 1, 1, 1, 32, false, false, true });
    TextureFormatDatabase::instance().register_format({ "RGB8", 1, 1, 1, 24, false, false, false });
    TextureFormatDatabase::instance().register_format({ "RGBA16F", 1, 1, 1, 64, false, false, true });
    TextureFormatDatabase::instance().register_format({ "RGBA32F", 1, 1, 1, 128, false, false, true });
}

Result<std::vector<TextureInfo>> TextureAnalyzer::analyze(const std::vector<FileInfo>& textures) {
    std::vector<TextureInfo> results;
    results.reserve(textures.size());
    
    for (const auto& texture : textures) {
        auto result = analyze_single(texture);
        if (result) {
            results.push_back(*result);
        } else {
            LOG_WARN("Failed to analyze texture {}: {}", texture.path.string(), result.error().message);
            // Add a basic info even if analysis failed
            TextureInfo basic_info;
            basic_info.disk_size = texture.size;
            results.push_back(basic_info);
        }
    }
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_textures = results.size();
        stats_.total_disk_size = 0;
        stats_.total_vram = 0;
        stats_.format_counts.clear();
        stats_.compression_counts.clear();
        stats_.resolution_counts.clear();
        stats_.max_width = 0;
        stats_.max_height = 0;
        
        for (const auto& tex : results) {
            stats_.total_disk_size += tex.disk_size;
            stats_.total_vram += tex.estimated_vram;
            
            if (!tex.format.empty()) {
                stats_.format_counts[tex.format]++;
            }
            
            if (tex.is_compressed) {
                stats_.compression_counts[tex.compression_format]++;
            } else {
                stats_.compression_counts["Uncompressed"]++;
            }
            
            if (tex.width > 0 && tex.height > 0) {
                std::string res_key = std::to_string(tex.width) + "x" + std::to_string(tex.height);
                stats_.resolution_counts[res_key]++;
                
                if (tex.width > stats_.max_width) stats_.max_width = tex.width;
                if (tex.height > stats_.max_height) stats_.max_height = tex.height;
            }
        }
        
        if (results.size() > 0) {
            float total_comp_ratio = 0.0f;
            int count = 0;
            for (const auto& tex : results) {
                if (tex.compression_ratio > 0) {
                    total_comp_ratio += tex.compression_ratio;
                    count++;
                }
            }
            if (count > 0) {
                stats_.avg_compression_ratio = total_comp_ratio / count;
            }
        }
    }
    
    return results;
}

Result<TextureInfo> TextureAnalyzer::analyze_single(const FileInfo& texture) {
    TextureInfo info;
    info.disk_size = texture.size;
    
    try {
        // Analyze based on file type
        switch (texture.type) {
            case FileType::DDS:
                return analyze_dds(texture.path);
            case FileType::KTX:
                return analyze_ktx(texture.path);
            case FileType::PVR:
                return analyze_pvr(texture.path);
            case FileType::ASTC:
                return analyze_astc(texture.path);
            case FileType::PNG:
                return analyze_png(texture.path);
            case FileType::JPG:
                return analyze_jpg(texture.path);
            case FileType::TGA:
                return analyze_tga(texture.path);
            case FileType::BMP:
                return analyze_bmp(texture.path);
            case FileType::EXR:
                return analyze_exr(texture.path);
            case FileType::HDR:
                return analyze_hdr(texture.path);
            case FileType::WebP:
                return analyze_webp(texture.path);
            default:
                return analyze_generic(texture.path, texture.type);
        }
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Texture analysis failed: {}", e.what())));
    }
}

Result<TextureInfo> TextureAnalyzer::analyze_dds(const Path& path) {
    TextureInfo info;
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open DDS file: {}", path.string())));
    }
    
    // Read DDS header (128 bytes)
    std::array<uint8_t, 128> header{};
    file.read(reinterpret_cast<char*>(header.data()), header.size());
    
    if (!file || std::memcmp(header.data(), "DDS ", 4) != 0) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, 
            "Invalid DDS file signature"));
    }
    
    // Parse DDS header
    uint32_t height = *reinterpret_cast<uint32_t*>(&header[8]);
    uint32_t width = *reinterpret_cast<uint32_t*>(&header[12]);
    uint32_t pitch_or_linear_size = *reinterpret_cast<uint32_t*>(&header[16]);
    uint32_t depth = *reinterpret_cast<uint32_t*>(&header[24]);
    uint32_t mip_map_count = *reinterpret_cast<uint32_t*>(&header[28]);
    uint32_t flags = *reinterpret_cast<uint32_t*>(&header[76]);
    uint32_t fourcc = *reinterpret_cast<uint32_t*>(&header[84]);
    
    info.width = width;
    info.height = height;
    info.depth = std::max(1u, depth);
    info.mip_levels = std::max(1u, mip_map_count);
    
    // Determine format based on flags and fourCC
    if (flags & 0x1) { // DDSD_CAPS
        // Check for cubemap
        uint32_t caps2 = *reinterpret_cast<uint32_t*>(&header[104]);
        if (caps2 & 0x200) { // DDSCAPS2_CUBEMAP
            info.is_cubemap = true;
        }
        if (caps2 & 0x400) { // DDSCAPS2_VOLUME
            info.is_volume = true;
        }
    }
    
    // Determine pixel format
    uint32_t pf_flags = *reinterpret_cast<uint32_t*>(&header[88]);
    if (pf_flags & 0x4) { // DDPF_FOURCC
        switch (fourcc) {
            case 0x31545844: // 'DXT1' 
                info.format = "DXT1";
                info.block_size = 8;
                info.is_compressed = true;
                info.compression_format = "DXT1";
                break;
            case 0x33545844: // 'DXT3'
                info.format = "DXT3";
                info.block_size = 16;
                info.is_compressed = true;
                info.compression_format = "DXT3";
                break;
            case 0x35545844: // 'DXT5'
                info.format = "DXT5";
                info.block_size = 16;
                info.is_compressed = true;
                info.compression_format = "DXT5";
                break;
            case 0x34545844: // 'DXT4' (rare)
                info.format = "DXT4";
                info.block_size = 16;
                info.is_compressed = true;
                info.compression_format = "DXT4";
                break;
            case 0x35424334: // '4C5B' -> 'BC4'
                info.format = "BC4";
                info.block_size = 8;
                info.is_compressed = true;
                info.compression_format = "BC4";
                break;
            case 0x35424335: // '5C5B' -> 'BC5'
                info.format = "BC5";
                info.block_size = 16;
                info.is_compressed = true;
                info.compression_format = "BC5";
                break;
            case 0x36424336: // '6C5B' -> 'BC6H'
                info.format = "BC6H";
                info.block_size = 16;
                info.is_compressed = true;
                info.compression_format = "BC6H";
                break;
            case 0x37424337: // '7C5B' -> 'BC7'
                info.format = "BC7";
                info.block_size = 16;
                info.is_compressed = true;
                info.compression_format = "BC7";
                break;
            default:
                info.format = std::format("FourCC 0x{:08X}", fourcc);
                info.is_compressed = true;
                break;
        }
    } else if (pf_flags & 0x40) { // DDPF_RGB
        uint32_t rgb_bit_count = *reinterpret_cast<uint32_t*>(&header[96]);
        uint32_t r_bit_mask = *reinterpret_cast<uint32_t*>(&header[100]);
        uint32_t g_bit_mask = *reinterpret_cast<uint32_t*>(&header[104]);
        uint32_t b_bit_mask = *reinterpret_cast<uint32_t*>(&header[108]);
        uint32_t a_bit_mask = *reinterpret_cast<uint32_t*>(&header[112]);
        
        if (rgb_bit_count == 32 && a_bit_mask != 0) {
            info.format = "RGBA8";
        } else if (rgb_bit_count == 24) {
            info.format = "RGB8";
        } else {
            info.format = std::format("RGB{}", rgb_bit_count);
        }
        
        info.block_size = rgb_bit_count / 8;
        info.is_compressed = false;
    }
    
    // Calculate mip map sizes
    info.mip_sizes.clear();
    uint32_t w = width;
    uint32_t h = height;
    uint32_t d = info.depth;
    
    for (uint32_t level = 0; level < info.mip_levels; ++level) {
        uint64_t size = 0;
        if (info.is_compressed && info.block_size > 0) {
            // Compressed format
            uint32_t block_w = std::max(1u, (w + 3) / 4);
            uint32_t block_h = std::max(1u, (h + 3) / 4);
            size = static_cast<uint64_t>(block_w) * block_h * d * info.block_size;
        } else {
            // Uncompressed format
            size = static_cast<uint64_t>(w) * h * d * (info.block_size > 0 ? info.block_size : 4);
        }
        info.mip_sizes.push_back(size);
        
        // Next mip level
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);
        d = std::max(1u, d / 2);
    }
    
    // Calculate total size
    info.disk_size = std::accumulate(info.mip_sizes.begin(), info.mip_sizes.end(), 0ULL);
    info.estimated_vram = info.disk_size; // VRAM usage roughly equals disk size for textures
    
    return info;
}

Result<TextureInfo> TextureAnalyzer::analyze_ktx(const Path& path) {
    // Simplified KTX parser - in reality would be more complex
    TextureInfo info;
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open KTX file: {}", path.string())));
    }
    
    // KTS identifier: 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
    std::array<uint8_t, 12> identifier{};
    file.read(reinterpret_cast<char*>(identifier.data()), identifier.size());
    
    const std::array<uint8_t, 12> expected_id = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
    if (!file || std::memcmp(identifier.data(), expected_id.data(), identifier.size()) != 0) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Invalid KTX file"));
    }
    
    // Skip to pixel format information (simplified)
    // In a real implementation, we'd parse the full header
    
    info.format = "KTX";
    info.is_compressed = true; // Assume compressed for simplicity
    
    // Get file size as approximation
    info.disk_size = fs::file_size(path);
    info.estimated_vram = info.disk_size * 0.8f; // Rough estimate
    
    return info;
}

Result<TextureInfo> TextureAnalyzer::analyze_pvr(const Path& path) {
    TextureInfo info;
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open PVR file: {}", path.string())));
    }
    
    // PVR header is 52 bytes for v2, 64 bytes for v3
    std::array<uint8_t, 64> header{};
    file.read(reinterpret_cast<char*>(header.data()), std::min<size_t>(header.size(), 52));
    
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to read PVR header"));
    }
    
    // Check for PVR identifier
    uint32_t version = *reinterpret_cast<uint32_t*>(&header[0]);
    if (version != 0x50565203 && version != 0x50520321) { // 'PVR!' or '!RVP'
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Invalid PVR file"));
    }
    
    // Parse basic info (simplified)
    uint32_t width = *reinterpret_cast<uint32_t*>(&header[12]);
    uint32_t height = *reinterpret_cast<uint32_t*>(&header[16]);
    uint32_t depth = *reinterpret_cast<uint32_t*>(&header[20]);
    uint32_t num_surfaces = *reinterpret_cast<uint32_t*>(&header[24]);
    uint32_t format_flags = *reinterpret_cast<uint32_t*>(&header[40]);
    
    info.width = width;
    info.height = height;
    info.depth = std::max(1u, depth);
    
    // Determine format based on flags (simplified)
    if (format_flags & 0x100) { // PVRTC 2BPP
        info.format = "PVRTC2";
        info.block_size = 8;
        info.is_compressed = true;
        info.compression_format = "PVRTC2";
    } else if (format_flags & 0x200) { // PVRTC 4BPP
        info.format = "PVRTC4";
        info.block_size = 8;
        info.is_compressed = true;
        info.compression_format = "PVRTC4";
    } else {
        // Assume uncompressed RGBA
        info.format = "RGBA8";
        info.block_size = 4;
        info.is_compressed = false;
    }
    
    // Calculate size
    if (info.is_compressed && info.block_size > 0) {
        // For compressed textures
        uint32_t block_w = std::max(1u, (width + 3) / 4);
        uint32_t block_h = std::max(1u, (height + 3) / 4);
        info.disk_size = static_cast<uint64_t>(block_w) * block_h * std::max(1u, depth) * info.block_size;
    } else {
        // For uncompressed textures
        info.disk_size = static_cast<uint64_t>(width) * height * std::max(1u, depth) * 
                        (info.block_size > 0 ? info.block_size : 4);
    }
    
    info.estimated_vram = info.disk_size;
    
    return info;
}

Result<TextureInfo> TextureAnalyzer::analyze_astc(const Path& path) {
    TextureInfo info;
    
    // ASTC files are rare as standalone - usually embedded in containers
    // For now, treat as generic binary with ASTC extension
    
    info.format = "ASTC";
    info.is_compressed = true;
    info.compression_format = "ASTC";
    
    // Try to extract block size from filename if possible
    std::string filename = path.filename().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
    
    if (filename.find("_4x4") != std::string::npos) {
        info.block_size = 16; // 128 bits / 8
    } else if (filename.find("_6x6") != std::string::npos) {
        info.block_size = 16;
    } else if (filename.find("_8x8") != std::string::npos) {
        info.block_size = 16;
    } else {
        // Default to 4x4 block
        info.block_size = 16;
    }
    
    info.disk_size = fs::file_size(path);
    // Estimate compressed size based on block size
    // This is a rough approximation
    if (info.block_size > 0) {
        // Assume 128-bit blocks
        uint64_t blocks = (info.disk_size * 8) / 128;
        info.estimated_vram = blocks * 16; // Approximate uncompressed size
    } else {
        info.estimated_vram = info.disk_size * 3; // Rough decompression ratio
    }
    
    return info;
}

Result<TextureInfo> TextureAnalyzer::analyze_png(const Path& path) {
    TextureInfo info;
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open PNG file: {}", path.string())));
    }
    
    // PNG signature: 89 50 4E 47 0D 0A 1A 0A
    std::array<uint8_t, 8> signature{};
    file.read(reinterpret_cast<char*>(signature.data()), signature.size());
    
    const std::array<uint8_t, 8> png_sig = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    if (!file || std::memcmp(signature.data(), png_sig.data(), signature.size()) != 0) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Invalid PNG file"));
    }
    
    // Read chunks until we find IHDR
    while (!file.eof()) {
        // Read chunk length
        uint32_t length;
        file.read(reinterpret_cast<char*>(&length), sizeof(length));
        length = __builtin_bswap32(length); // PNG is big-endian
        
        if (file.eof()) break;
        
        // Read chunk type
        std::array<char, 4> type{};
        file.read(type.data(), 4);
        
        if (std::memcmp(type.data(), "IHDR", 4) == 0) {
            // Found IHDR chunk, read width and height
            uint32_t width, height;
            file.read(reinterpret_cast<char*>(&width), sizeof(width));
            file.read(reinterpret_cast<char*>(&height), sizeof(height));
            width = __builtin_bswap32(width);
            height = __builtin_bswap32(height);
            
            info.width = width;
            info.height = height;
            
            // Read bit depth and color type
            uint8_t bit_depth, color_type;
            file.read(reinterpret_cast<char*>(&bit_depth), 1);
            file.read(reinterpret_cast<char*>(&color_type), 1);
            
            // Determine format based on color type
            switch (color_type) {
                case 0: // Grayscale
                    info.format = bit_depth == 16 ? "GRAY16" : "GRAY8";
                    info.block_size = bit_depth / 8;
                    break;
                case 2: // RGB
                    info.format = bit_depth == 16 ? "RGB16" : "RGB8";
                    info.block_size = (bit_depth * 3) / 8;
                    break;
                case 3: // Indexed
                    info.format = "INDEXED";
                    info.block_size = 1; // Palette index
                    break;
                case 4: // Grayscale + alpha
                    info.format = bit_depth == 16 ? "GRAYA16" : "GRAYA8";
                    info.block_size = (bit_depth * 2) / 8;
                    info.has_alpha = true;
                    break;
                case 6: // RGBA
                    info.format = bit_depth == 16 ? "RGBA16" : "RGBA8";
                    info.block_size = (bit_depth * 4) / 8;
                    info.has_alpha = true;
                    break;
                default:
                    info.format = "UNKNOWN";
                    info.block_size = 4;
                    break;
            }
            
            // Check if interlaced
            uint8_t interlace_method;
            file.read(reinterpret_cast<char*>(&interlace_method), 1);
            // Interlace affects decoding but not basic dimensions
            
            // Skip CRC
            file.seekg(4, std::ios::cur);
            
            info.is_compressed = true; // PNG is always compressed
            info.compression_format = "DEFLATE";
            info.compression_ratio = 0.5f; // Typical PNG compression ratio
            
            // Calculate approximate memory size
            // Width * Height * Bytes per pixel
            info.disk_size = fs::file_size(path);
            size_t bpp = info.block_size > 0 ? info.block_size : 4;
            info.estimated_vram = static_cast<uint64_t>(width) * height * bpp;
            
            return info;
        }
        
        // Skip chunk data and CRC
        file.seekg(length + 4, std::ios::cur);
    }
    
    return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "PNG file missing IHDR chunk"));
}

Result<TextureInfo> TextureAnalyzer::analyze_jpg(const Path& path) {
    TextureInfo info;
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open JPG file: {}", path.string())));
    }
    
    // JPEG SOI marker: FF D8
    std::array<uint8_t, 2> soi{};
    file.read(reinterpret_cast<char*>(soi.data()), soi.size());
    
    if (!file || soi[0] != 0xFF || soi[1] != 0xD8) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Invalid JPEG file"));
    }
    
    // Scan for SOF0 (Start of Frame) marker
    while (!file.eof()) {
        // Find next marker
        uint8_t byte;
        do {
            file.read(reinterpret_cast<char*>(&byte), 1);
            if (file.eof()) return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, 
                "JPEG file missing SOF0 marker"));
        } while (byte != 0xFF);
        
        // Skip any padding FF bytes
        while (file.peek() == 0xFF) {
            file.get();
        }
        
        uint8_t marker = file.get();
        
        if (marker == 0xC0) { // SOF0 - Baseline DCT
            // Read segment length
            uint16_t length;
            file.read(reinterpret_cast<char*>(&length), sizeof(length));
            length = __builtin_bswap16(length);
            
            // Skip precision
            file.seekg(1, std::ios::cur);
            
            // Read height and width
            uint16_t height, width;
            file.read(reinterpret_cast<char*>(&height), sizeof(height));
            file.read(reinterpret_cast<char*>(&width), sizeof(width));
            height = __builtin_bswap16(height);
            width = __builtin_bswap16(width);
            
            info.width = width;
            info.height = height;
            
            // Read number of components
            uint8_t num_components;
            file.read(reinterpret_cast<char*>(&num_components), 1);
            
            // Determine format based on components
            if (num_components == 1) {
                info.format = "GRAY8";
                info.block_size = 1;
            } else if (num_components == 3) {
                info.format = "YCBCR"; // JPEG typically uses YCbCr
                info.block_size = 3;
            } else if (num_components == 4) {
                info.format = "YCBCRA"; // With alpha
                info.block_size = 4;
            } else {
                info.format = "UNKNOWN";
                info.block_size = 3;
            }
            
            info.is_compressed = true;
            info.compression_format = "JPEG";
            info.compression_ratio = 0.1f; // Typical JPEG compression ratio
            
            // File size is compressed size
            info.disk_size = fs::file_size(path);
            
            // Estimate uncompressed size
            size_t bpp = info.block_size > 0 ? info.block_size : 3;
            info.estimated_vram = static_cast<uint64_t>(width) * height * bpp;
            
            return info;
        } else if (marker == 0xDA) { // SOS - Start of Scan
            // We've passed the header without finding SOF0
            break;
        } else {
            // Skip this segment
            uint16_t length;
            file.read(reinterpret_cast<char*>(&length), sizeof(length));
            length = __builtin_bswap16(length);
            file.seekg(length - 2, std::ios::cur); // Subtract 2 for length field itself
        }
    }
    
    return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "JPEG file missing SOF0 marker"));
}

// Simplified implementations for other formats - in reality these would be more complex
Result<TextureInfo> TextureAnalyzer::analyze_tga(const Path& path) {
    TextureInfo info;
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open TGA file: {}", path.string())));
    }
    
    // TGA header is 18 bytes
    std::array<uint8_t, 18> header{};
    file.read(reinterpret_cast<char*>(header.data()), header.size());
    
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to read TGA header"));
    }
    
    // Width and height are at offset 12 and 14 (little endian)
    uint16_t width = *reinterpret_cast<uint16_t*>(&header[12]);
    uint16_t height = *reinterpret_cast<uint16_t*>(&header[14]);
    uint8_t pixel_depth = header[16];
    uint8_t image_descriptor = header[17];
    
    info.width = width;
    info.height = height;
    
    // Determine format based on pixel depth
    switch (pixel_depth) {
        case 8:
            info.format = "GRAY8";
            info.block_size = 1;
            break;
        case 16:
            // Check if it's ARGB1555 or RGB565
            if (image_descriptor & 0x80) {
                info.format = "ARGB1555";
            } else {
                info.format = "RGB565";
            }
            info.block_size = 2;
            break;
        case 24:
            info.format = "RGB8";
            info.block_size = 3;
            break;
        case 32:
            info.format = "RGBA8";
            info.block_size = 4;
            if (image_descriptor & 0x80) {
                info.has_alpha = true;
            }
            break;
        default:
            info.format = std::format("{}BIT", pixel_depth);
            info.block_size = pixel_depth / 8;
            break;
    }
    
    info.is_compressed = false; // TGA is usually uncompressed
    info.compression_format = "NONE";
    info.compression_ratio = 1.0f;
    
    // Calculate image size
    size_t bytes_per_pixel = info.block_size > 0 ? info.block_size : 4;
    info.disk_size = static_cast<uint64_t>(width) * height * bytes_per_pixel;
    info.estimated_vram = info.disk_size;
    
    // Check for RLE compression (bit 3 of image descriptor)
    if (image_descriptor & 0x08) {
        info.is_compressed = true;
        info.compression_format = "RLE";
        // Compressed size would be less, but we don't decompress to check
        info.compression_ratio = 0.5f; // Rough estimate
    }
    
    return info;
}

Result<TextureInfo> TextureAnalyzer::analyze_bmp(const Path& path) {
    TextureInfo info;
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open BMP file: {}", path.string())));
    }
    
    // BMP header is 14 bytes
    std::array<uint8_t, 14> header{};
    file.read(reinterpret_cast<char*>(header.data()), header.size());
    
    if (!file || header[0] != 'B' || header[1] != 'M') {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Invalid BMP file"));
    }
    
    // DIB header starts at offset 14
    // We need to read enough to get width and height
    std::array<uint8_t, 40> dib_header{}; // BITMAPINFOHEADER
    file.read(reinterpret_cast<char*>(dib_header.data()), dib_header.size());
    
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to read BMP DIB header"));
    }
    
    // Width and height are at offset 4 and 8 in DIB header (little endian)
    int32_t width = *reinterpret_cast<int32_t*>(&dib_header[4]);
    int32_t height = *reinterpret_cast<int32_t*>(&dib_header[8]);
    uint16_t bits_per_pixel = *reinterpret_cast<uint16_t*>(&dib_header[14]);
    uint32_t compression = *reinterpret_cast<uint32_t*>(&dib_header[16]);
    
    info.width = std::abs(width);
    info.height = std::abs(height);
    
    // Determine format based on bit depth
    switch (bits_per_pixel) {
        case 1:
            info.format = "MONO";
            info.block_size = 1; // Actually bit-packed
            break;
        case 4:
            info.format = "COLOR4";
            info.block_size = 1; // Palette index
            break;
        case 8:
            info.format = "COLOR8";
            info.block_size = 1; // Palette index
            break;
        case 16:
            info.format = "RGB565";
            info.block_size = 2;
            break;
        case 24:
            info.format = "BGR8"; // BMP stores as BGR
            info.block_size = 3;
            break;
        case 32:
            info.format = "BGRA8"; // BGRA with alpha
            info.block_size = 4;
            info.has_alpha = true;
            break;
        default:
            info.format = std::format("{}BIT", bits_per_pixel);
            info.block_size = bits_per_pixel / 8;
            break;
    }
    
    info.is_compressed = (compression != 0); // BI_RGB = 0 means no compression
    if (info.is_compressed) {
        switch (compression) {
            case 1: info.compression_format = "RLE8"; break;
            case 2: info.compression_format = "RLE4"; break;
            case 3: info.compression_format = "BITFIELDS"; break;
            default: info.compression_format = "UNKNOWN"; break;
        }
    } else {
        info.compression_format = "NONE";
    }
    
    info.compression_ratio = info.is_compressed ? 0.5f : 1.0f;
    
    // Calculate image size (including padding)
    int32_t stride = ((info.width * bits_per_pixel + 31) / 32) * 4; // Row stride in bytes
    info.disk_size = static_cast<uint64_t>(stride) * std::abs(height);
    info.estimated_vram = info.disk_size; // Approximate
    
    return info;
}

Result<TextureInfo> TextureAnalyzer::analyze_exr(const Path& path) {
    // OpenEXR is complex - simplified implementation
    TextureInfo info;
    
    info.format = "EXR";
    info.is_compressed = true;
    info.compression_format = "ZIP"; // Default assumption
    info.compression_ratio = 0.3f; // EXR typically compresses well
    
    info.disk_size = fs::file_size(path);
    // EXR is usually float16 or float32 per channel
    info.estimated_vram = info.disk_size * 3.0f; // Rough estimate for RGB
    
    return info;
}

Result<TextureInfo> TextureAnalyzer::analyze_hdr(const Path& path) {
    // RGBE HDR format
    TextureInfo info;
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open HDR file: {}", path.string())));
    }
    
    // Check for "#?RADIANCE" or "?#RADIANCE" header
    std::string line;
    std::getline(file, line);
    
    if (line.find("RADIANCE") == std::string::npos) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Invalid HDR file"));
    }
    
    // Parse header to find resolution (simplified)
    int width = 0, height = 0;
    while (std::getline(file, line) && !file.eof()) {
        if (line.empty() || line[0] == '\n') break; // End of header
        
        // Look for resolution in format like "-Y 480 +X 640"
        size_t y_pos = line.find("-Y ");
        if (y_pos != std::string::npos) {
            try {
                height = std::stoi(line.substr(y_pos + 3));
            } catch (...) {}
        }
        
        size_t x_pos = line.find("+X ");
        if (x_pos != std::string::npos) {
            try {
                width = std::stoi(line.substr(x_pos + 3));
            } catch (...) {}
        }
    }
    
    if (width > 0 && height > 0) {
        info.width = width;
        info.height = height;
        info.format = "RGBE"; // Shared exponent RGB
        info.block_size = 4; // 4 bytes per pixel (RGBE)
        info.is_compressed = false; // HDR is usually not compressed
        info.compression_format = "NONE";
        info.compression_ratio = 1.0f;
        
        info.disk_size = fs::file_size(path);
        info.estimated_vram = static_cast<uint64_t>(width) * height * 4 * 3; // 3 float16 components
    } else {
        // Fallback
        info.disk_size = fs::file_size(path);
        info.estimated_vram = info.disk_size * 2.0f; // Rough estimate
    }
    
    return info;
}

Result<TextureInfo> TextureAnalyzer::analyze_webp(const Path& path) {
    // WebP format - simplified
    TextureInfo info;
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open WebP file: {}", path.string())));
    }
    
    // WebP header: "RIFF" + size + "WEBP" + chunk
    std::array<uint8_t, 4> riff{};
    file.read(reinterpret_cast<char*>(riff.data()), riff.size());
    
    if (!file || std::memcmp(riff.data(), "RIFF", 4) != 0) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Not a RIFF file"));
    }
    
    // Skip file size
    file.seekg(4, std::ios::cur);
    
    // Check for WEBP
    std::array<uint8_t, 4> webp{};
    file.read(reinterpret_cast<char*>(webp.data()), webp.size());
    
    if (!file || std::memcmp(webp.data(), "WEBP", 4) != 0) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Not a WebP file"));
    }
    
    // Now look for VP8/X chunk
    while (!file.eof()) {
        std::array<uint8_t, 4> chunk_header{};
        file.read(reinterpret_cast<char*>(chunk_header.data()), 4);
        
        if (!file) break;
        
        uint32_t chunk_size;
        file.read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size));
        chunk_size = __builtin_bswap32(chunk_size);
        
        std::string chunk_type(chunk_header.begin(), chunk_header.end());
        
        if (chunk_type == "VP8 " || chunk_type == "VP8L" || chunk_type == "VP8X") {
            // Found VP8 chunk - now we need to parse it to get dimensions
            // This is simplified - real implementation would parse the VP8 data
            
            // For now, just set some defaults
            info.format = "WEBP";
            info.is_compressed = true;
            info.compression_format = "VP8";
            info.compression_ratio = 0.25f; // WebP is quite efficient
            
            info.disk_size = fs::file_size(path);
            
            // Very rough estimate - assume 24bpp for RGB
            // In reality we'd need to parse the actual dimensions
            uint64_t estimated_pixels = info.disk_size * 4; // Assume 4x compression
            uint32_t approx_dim = static_cast<uint32_t>(std::sqrt(static_cast<double>(estimated_pixels / 3)));
            info.width = std::max(32u, std::min(4096u, approx_dim));
            info.height = std::max(32u, std::min(4096u, approx_dim));
            
            info.estimated_vram = static_cast<uint64_t>(info.width) * info.height * 3;
            
            return info;
        }
        
        // Skip chunk data
        file.seekg(chunk_size, std::ios::cur);
    }
    
    return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "WebP file missing VP8 chunk"));
}

Result<TextureInfo> TextureAnalyzer::analyze_generic(const Path& path, FileType type) {
    TextureInfo info;
    
    // Generic fallback - just use file size and make reasonable assumptions
    info.disk_size = fs::file_size(path);
    
    // Try to guess if it's likely a texture based on extension and size
    std::string ext = StringUtils::to_lower(path.extension().string());
    
    // Common texture extensions that we might not have explicit handlers for
    static const std::unordered_set<std::string> texture_exts = {
        ".tex", ".ktx2", ".pvr", ".astc", ".exr", ".hdr", ".pic", ".tif", ".iff"
    };
    
    bool is_likely_texture = texture_exts.find(ext) != texture_exts.end();
    
    if (is_likely_texture || info.disk_size > 1024) { // Assume files >1KB might be textures
        // Make reasonable assumptions
        info.format = "UNKNOWN";
        info.is_compressed = false;
        info.compression_format = "NONE";
        info.compression_ratio = 1.0f;
        
        // Very rough estimate: assume it's a square texture
        // This is just a placeholder - real analysis would be much more sophisticated
        uint64_t area = info.disk_size / 4; // Assume 4 bytes per pixel (RGBA8)
        uint32_t side = static_cast<uint32_t>(std::sqrt(static_cast<double>(area)));
        
        if (side >= 32) { // Reasonable minimum texture size
            info.width = side;
            info.height = side;
            info.depth = 1;
            info.estimated_vram = info.disk_size; // Assume VRAM ~= disk size for textures
        } else {
            // Too small to be a reasonable texture - treat as generic data
            info.width = 0;
            info.height = 0;
            info.depth = 0;
            info.estimated_vram = 0;
        }
    } else {
        // Definitely not a texture
        info.width = 0;
        info.height = 0;
        info.depth = 0;
        info.estimated_vram = 0;
    }
    
    return info;
}

TextureStats TextureAnalyzer::stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

String TextureAnalyzer::generate_report() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    std::stringstream ss;
    ss << "Texture Analysis Report\n";
    ss << "=====================\n";
    ss << "Total Textures: " << stats_.total_textures << "\n";
    ss << "Total Disk Size: " << StringUtils::format_bytes(stats_.total_disk_size) << "\n";
    ss << "Estimated VRAM Usage: " << StringUtils::format_bytes(stats_.total_vram) << "\n";
    ss << "Average Compression Ratio: " << std::fixed << std::setprecision(2) << stats_.avg_compression_ratio << "\n";
    ss << "Max Resolution: " << stats_.max_width << "x" << stats_.max_height << "\n";
    
    if (!stats_.format_counts.empty()) {
        ss << "\nFormat Distribution:\n";
        for (const auto& [format, count] : stats_.format_counts) {
            ss << "  " << format << ": " << count << "\n";
        }
    }
    
    if (!stats_.compression_counts.empty()) {
        ss << "\nCompression Distribution:\n";
        for (const auto& [comp, count] : stats_.compression_counts) {
            ss << "  " << comp << ": " << count << "\n";
        }
    }
    
    if (!stats_.resolution_counts.empty() && stats_.total_textures < 20) {
        ss << "\nResolution Distribution:\n";
        for (const auto& [res, count] : stats_.resolution_counts) {
            ss << "  " << res << ": " << count << "\n";
        }
    }
    
    return ss.str();
}

} // namespace game_req