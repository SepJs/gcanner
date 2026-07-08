#include <game_req_analyzer/analyzers/model_analyzer.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/hash_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <algorithm>
#include <fstream>
#include <vector>
#include <cstring>
#include <limits>

using namespace game_req;

ModelAnalyzer::ModelAnalyzer(const AnalyzerConfig& config) : config_(config) {
    // Initialize any necessary data structures
}

std::vector<ModelInfo> ModelAnalyzer::analyze(const std::vector<FileInfo>& models) {
    std::vector<ModelInfo> results;
    results.reserve(models.size());
    
    for (const auto& model : models) {
        auto result = analyze_single(model);
        if (result) {
            results.push_back(*result);
        } else {
            LOG_WARN("Failed to analyze model {}: {}", model.path.string(), result.error().message);
            // Add a basic info even if analysis failed
            ModelInfo basic_info;
            basic_info.disk_size = model.size;
            results.push_back(basic_info);
        }
    }
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_models = results.size();
        stats_.total_vertices = 0;
        stats_.total_triangles = 0;
        stats_.total_meshes = 0;
        stats_.total_materials = 0;
        stats_.total_bones = 0;
        stats_.total_vram = 0;
        stats_.total_disk_size = 0;
        stats_.format_counts.clear();
        stats_.engine_counts.clear();
        stats_.max_vertices_per_model = 0;
        stats_.max_triangles_per_model = 0;
        
        for (const auto& model : results) {
            stats_.total_disk_size += model.disk_size;
            stats_.total_vertices += model.vertex_count;
            stats_.total_triangles += model.triangle_count;
            stats_.total_meshes += model.mesh_count;
            stats_.total_materials += model.material_count;
            stats_.total_bones += model.bone_count;
            stats_.total_vram += model.estimated_vram;
            
            if (!model.format.empty()) {
                stats_.format_counts[model.format]++;
            }
            
            if (!model.engine_hint.empty()) {
                stats_.engine_counts[model.engine_hint]++;
            }
            
            if (model.vertex_count > stats_.max_vertices_per_model) {
                stats_.max_vertices_per_model = model.vertex_count;
            }
            
            if (model.triangle_count > stats_.max_triangles_per_model) {
                stats_.max_triangles_per_model = model.triangle_count;
            }
        }
    }
    
    return results;
}

ModelInfo ModelAnalyzer::analyze_single(const FileInfo& model) {
    ModelInfo info;
    info.disk_size = model.size;
    
    try {
        // Analyze based on file type
        switch (model.type) {
            case FileType::FBX:
                return analyze_fbx(model.path);
            case FileType::OBJ:
                return analyze_obj(model.path);
            case FileType::GLTF:
                return analyze_gltf(model.path);
            case FileType::GLB:
                return analyze_glb(model.path);
            case FileType::COLLADA:
                return analyze_collada(model.path);
            case FileType::USD:
                return analyze_usd(model.path);
            case FileType::USDZ:
                return analyze_usdz(model.path);
            case FileType::BLEND:
                return analyze_blend(model.path);
            case FileType::MAX:
                return analyze_max(model.path);
            case FileType::MA:
                return analyze_ma(model.path);
            case FileType::MB:
                return analyze_mb(model.path);
            case FileType::X:
                return analyze_x(model.path);
            case FileType::MDL:
                return analyze_mdl(model.path);
            case FileType::MD2:
                return analyze_md2(model.path);
            case FileType::MD3:
                return analyze_md3(model.path);
            case FileType::MD5:
                return analyze_md5(model.path);
            case FileType::NIF:
                return analyze_nif(model.path);
            case FileType::HKX:
                return analyze_hkx(model.path);
            case FileType::GR2:
                return analyze_gr2(model.path);
            case FileType::SMD:
                return analyze_smd(model.path);
            case FileType::DMX:
                return analyze_dmx(model.path);
            default:
                return analyze_generic(model.path, model.type);
        }
    } catch (const std::exception& e) {
        // Return basic info on exception
        info.error_message = e.what();
        return info;
    }
}

// Implementation of specific format analyzers - simplified for demonstration
ModelInfo ModelAnalyzer::analyze_fbx(const Path& path) {
    ModelInfo info;
    
    // FBX is complex - this is a simplified implementation
    // In reality, we'd use the FBX SDK or parse the binary format properly
    
    info.disk_size = fs::file_size(path);
    
    // Check if it's binary or ASCII FBX by looking for magic bytes
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        info.error_message = "Cannot open file";
        return info;
    }
    
    // Check for binary FBX header
    std::array<char, 21> header{};
    file.read(header.data(), 21);
    
    bool is_binary = false;
    if (file && std::memcmp(header.data(), "Kaydara FBX Binary  ", 20) == 0) {
        is_binary = true;
    }
    
    // Reset and check for ASCII FBX
    file.clear();
    file.seekg(0, std::ios::beg);
    std::string first_line;
    std::getline(file, first_line);
    
    bool is_ascii = false;
    if (!first_line.empty() && 
        (first_line.find("FBXHeaderExtension") != std::string::npos ||
         first_line.find("FBXHeader") != std::string::npos)) {
        is_ascii = true;
    }
    
    if (is_binary) {
        info.format = "FBX Binary";
    } else if (is_ascii) {
        info.format = "FBX ASCII";
    } else {
        info.format = "FBX (Unknown)";
    }
    
    // Try to extract some basic info (very simplified)
    // In reality, we'd parse the FBX structure properly
    
    // Estimate vertex count based on file size (very rough approximation)
    // Assume average of 50 bytes per vertex for animated characters
    if (info.disk_size > 1024) { // Only estimate for reasonably sized files
        uint64_t estimated_vertices = info.disk_size / 50;
        info.vertex_count = std::min(estimated_vertices, static_cast<uint64_t>(1000000)); // Cap at 1M
        info.triangle_count = info.vertex_count / 3; // Rough estimate
    }
    
    // Estimate VRAM usage (vertex buffers + index buffers + textures)
    // Very rough approximation
    size_t vertex_size = info.vertex_count * (3*4 + 3*4 + 2*4); // position + normal + texcoord
    size_t index_size = info.triangle_count * 3 * 4; // 32-bit indices
    info.vertex_buffer_size = vertex_size;
    info.index_buffer_size = index_size;
    info.estimated_vram = vertex_size + index_size + (info.disk_size / 4); // Add texture estimate
    
    // Try to detect engine from file content
    std::string content;
    if (is_ascii) {
        // Read first 64KB for ASCII FBX
        file.seekg(0, std::ios::beg);
        std::array<char, 65536> buffer{};
        file.read(buffer.data(), buffer.size());
        content.assign(buffer.data(), file.gcount());
        
        // Look for engine signatures
        if (content.find("Unity") != std::string::npos) {
            info.engine_hint = "Unity";
        } else if (content.find("Unreal") != std::string::npos || 
                   content.find("UE4") != std::string::npos ||
                   content.find("UE5") != std::string::npos) {
            info.engine_hint = "Unreal Engine";
        } else if (content.find("CryEngine") != std::string::npos) {
            info.engine_hint = "CryEngine";
        } else if (content.find("Frostbite") != std::string::npos) {
            info.engine_hint = "Frostbite";
        }
    }
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_obj(const Path& path) {
    ModelInfo info;
    
    std::ifstream file(path);
    if (!file) {
        info.error_message = "Cannot open file";
        return info;
    }
    
    info.format = "OBJ";
    
    size_t vertex_count = 0;
    size_t normal_count = 0;
    size_t texcoord_count = 0;
    size_t face_count = 0;
    size_t material_count = 0;
    size_t mesh_count = 1; // OBJ typically has one mesh per file
    
    Bounds bounds;
    bool has_bounds = false;
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") { // Vertex
            float x, y, z;
            if (iss >> x >> y >> z) {
                vertex_count++;
                
                // Update bounding box
                if (!has_bounds) {
                    bounds.min_x = bounds.max_x = x;
                    bounds.min_y = bounds.max_y = y;
                    bounds.min_z = bounds.max_z = z;
                    has_bounds = true;
                } else {
                    bounds.min_x = std::min(bounds.min_x, x);
                    bounds.max_x = std::max(bounds.max_x, x);
                    bounds.min_y = std::min(bounds.min_y, y);
                    bounds.max_y = std::max(bounds.max_y, y);
                    bounds.min_z = std::min(bounds.min_z, z);
                    bounds.max_z = std::max(bounds.max_z, z);
                }
            }
        } else if (prefix == "vt") { // Texture coord
            float u, v;
            if (iss >> u >> v) texcoord_count++;
        } else if (prefix == "vn") { // Normal
            float x, y, z;
            if (iss >> x >> y >> z) normal_count++;
        } else if (prefix == "f") { // Face
            int vertex_count_in_face = 0;
            std::string token;
            while (iss >> token) vertex_count_in_face;
            
            if (vertex_count_in_face >= 3) {
                face_count++;
                // Triangulate: a face with n vertices creates (n-2) triangles
                // But we'll just count it as one face for simplicity
            }
        } else if (prefix == "mtllib") { // Material library
            material_count++; // We'll count each mtllib as a material reference
        } else if (prefix == "usemtl") { // Use material
            material_count++; // Count each material usage
        }
    }
    
    info.vertex_count = vertex_count;
    info.normal_count = normal_count;
    info.texcoord_count = texcoord_count;
    info.face_count = face_count;
    info.mesh_count = mesh_count;
    info.material_count = material_count;
    
    if (has_bounds) {
        info.bounds = bounds;
    }
    
    // Estimate triangle count (rough approximation: each face is at least a triangle)
    info.triangle_count = face_count;
    
    // Estimate bone count (OBJ doesn't have bones, so 0)
    info.bone_count = 0;
    
    // Estimate LOD count (OBJ doesn't have LOD info)
    info.lod_count = 1;
    
    // Estimate VRAM usage
    info.estimated_vram = estimate_vram = estimate_vram(info);
    
    info.disk_size = fs::file_size(path);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_gltf(const Path& path) {
    ModelInfo info;
    info.format = "GLTF";
    
    try {
        // GLTF is JSON-based - we'll do a simple parsing approach
        std::string content = StringUtils::read_text_file(path).value_or("");
        
        if (content.empty()) {
            info.error_message = "Empty GLTF file";
            return info;
        }
        
        // Basic attribute counting (simplified)
        size_t pos = 0;
        while ((pos = content.find("\"pos\"", pos)) != std::string::npos) {
            // Look for accessor with POSITION semantic
            size_t bufferview_start = content.find("\"bufferView\"", pos);
            if (bufferview_start != std::string::npos) {
                // Found a position attribute - estimate vertices
                info.vertex_count += 100; // Rough estimate per mesh
            }
            pos += 5;
        }
        
        // Count indices
        pos = 0;
        while ((pos = content.find("\"indices\"", pos)) != std::string::npos) {
            // Found indices - estimate triangles
            info.triangle_count += 50; // Rough estimate
            pos += 8;
        }
        
        // Estimate other properties
        info.mesh_count = std::count(content.begin(), content.end(), 'm') / 10; // Rough
        info.material_count = std::count(content.begin(), content.end(), 'm') / 8; // Rough
        
        // Look for extensions that might indicate engine
        if (content.find("KHR_materials_pbrSpecularGlossiness") != std::string::npos) {
            info.engine_hint = "PBR Material Extension";
        }
        
    } catch (const std::exception& e) {
        info.error_message = std::string("GLTF parsing failed: ") + e.what();
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_glb(const Path& path) {
    ModelInfo info;
    info.format = "GLB";
    
    // GLB is binary GLTF - we'll do a basic parse
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        info.error_message = "Cannot open GLB file";
        return info;
    }
    
    // Read header
    uint32_t magic, version, length;
    file.read(reinterpret_cast<char*>(&magic), 4);
    file.read(reinterpret_cast<char*>(&version), 4);
    file.read(reinterpret_cast<char*>(&length), 4);
    
    if (magic != 0x46546C67) { // 'glTF' in little endian
        info.error_message = "Invalid GLB magic number";
        return info;
    }
    
    // Skip to JSON chunk
    while (file.tellg() < static_cast<std::streampos>(length)) {
        uint32_t chunk_length, chunk_type;
        file.read(reinterpret_cast<char*>(&chunk_length), 4);
        file.read(reinterpret_cast<char*>(&chunk_type), 4);
        
        if (chunk_type == 0x4E4F534A) { // 'JSON' in little endian
            // Read JSON chunk
            std::string json_chunk(chunk_length, '\0');
            file.read(&json_chunk[0], chunk_length);
            
            // Process similar to GLTF
            size_t pos = 0;
            while ((pos = json_chunk.find("\"pos\"", pos)) != std::string::npos) {
                info.vertex_count += 100;
                pos += 5;
            }
            
            pos = 0;
            while ((pos = json_chunk.find("\"indices\"", pos)) != std::string::npos) {
                info.triangle_count += 50;
                pos += 8;
            }
            
            info.mesh_count = std::count(json_chunk.begin(), json_chunk.end(), 'm') / 10;
            info.material_count = std::count(json_chunk.begin(), json_chunk.end(), 'm') / 8;
            
            break;
        } else {
            // Skip this chunk
            file.seekg(chunk_length, std::ios::cur);
        }
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_collada(const Path& path) {
    ModelInfo info;
    info.format = "COLLADA";
    
    try {
        std::string content = StringUtils::read_text_file(path).value_or("");
        
        if (content.empty()) {
            info.error_message = "Empty COLLADA file";
            return info;
        }
        
        // Count vertices by looking for <float_array> or <Source> with position data
        size_t pos = 0;
        while ((pos = content.find("<float_array", pos)) != std::string::npos) {
            size_t id_pos = content.find("id=\"position\"", pos);
            if (id_pos != std::string::npos && id_pos < pos + 200) {
                // Found position array
                size_t count_pos = content.find("count=\"", pos);
                if (count_pos != std::string::npos) {
                    count_pos += 7; // Skip "count=\""
                    size_t end_pos = content.find("\"", count_pos);
                    if (end_pos != std::string::npos) {
                        std::string count_str = content.substr(count_pos, end_pos - count_pos);
                        try {
                            uint32_t count = std::stoi(count_str);
                            // Each vertex has 3 floats (x,y,z)
                            info.vertex_count += count / 3;
                        } catch (...) {}
                    }
                }
            }
            pos += 12; // Move past <float_array
        }
        
        // Count triangles by looking for <p> or <triangles> elements
        pos = 0;
        while ((pos = content.find("<p>", pos)) != std::string::npos || 
               (pos = content.find("<triangles", pos)) != std::string::npos) {
            // Rough estimate: each <p> contains indices for triangles
            size_t end_pos = content.find("</p>", pos);
            if (end_pos != std::string::npos) {
                std::string indices_text = content.substr(pos + 3, end_pos - pos - 3);
                // Count numbers in the indices text
                size_t num_count = std::count_if(indices_text.begin(), indices_text.end(), 
                                                [](char c){ return std::isdigit(c) || c == ' '; });
                // Very rough approximation
                info.triangle_count += num_count / 10;
            }
            pos += 3;
        }
        
        // Count geometries/meshes
        info.mesh_count = std::count(content.begin(), content.end(), '<') / 20; // Rough
        
        // Look for engine hints in asset tags
        pos = 0;
        while ((pos = content.find("<asset>", pos)) != std::string::npos) {
            size_t end_pos = content.find("</asset>", pos);
            if (end_pos != std::string::npos) {
                std::string asset_content = content.substr(pos, end_pos - pos);
                if (asset_content.find("Unity") != std::string::npos) {
                    info.engine_hint = "Unity";
                } else if (asset_content.find("Unreal") != std::string::npos) {
                    info.engine_hint = "Unreal Engine";
                } else if (asset_content.find("Blender") != std::string::npos) {
                    info.engine_hint = "Blender";
                }
                break;
            }
            pos += 7;
        }
        
    } catch (const std::exception& e) {
        info.error_message = std::string("COLLADA parsing failed: ") + e.what();
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_usd(const Path& path) {
    ModelInfo info;
    info.format = "USD";
    
    // USD is complex - we'll do a basic text-based analysis for .usd (ASCII) or binary for .usda/.usdc
    if (path.extension() == ".usda" || path.extension() == ".usdc") {
        // For now, treat as generic binary
        return analyze_generic(path, FileType::USD);
    }
    
    // ASCII USD (.usd)
    try {
        std::string content = StringUtils::read_text_file(path).value_or("");
        
        if (content.empty()) {
            info.error_message = "Empty USD file";
            return info;
        }
        
        // Count prims (basic objects in USD)
        size_t pos = 0;
        while ((pos = content.find("def ", pos)) != std::string::npos) {
            // Look for geometry types
            size_t type_pos = content.find("type=\"", pos);
            if (type_pos != std::string::npos && type_pos < pos + 100) {
                type_pos += 6; // Skip "type=\""
                size_t end_pos = content.find("\"", type_pos);
                if (end_pos != std::string::npos) {
                    std::string type_str = content.substr(type_pos, end_pos - type_pos);
                    if (type_str == "mesh" || type_str == "curves" || type_str == "points") {
                        info.mesh_count++;
                    }
                }
            }
            pos += 4;
        }
        
        // Estimate vertices from point positions
        pos = 0;
        while ((pos = content.find("point[] =", pos)) != std::string::npos) {
            size_t bracket_start = content.find("[", pos);
            size_t bracket_end = content.find("]", pos);
            if (bracket_start != std::string::npos && bracket_end != std::string::npos && 
                bracket_start < bracket_end) {
                std::string points_content = content.substr(bracket_start + 1, bracket_end - bracket_start - 1);
                // Count points (each point has 3 coordinates)
                size_t comma_count = std::count(points_content.begin(), points_content.end(), ',');
                // Rough estimate: points are formatted as "(x, y, z), (x, y, z), ..."
                if (comma_count > 0) {
                    size_t point_count = comma_count / 2 + 1; // Rough approximation
                    info.vertex_count += point_count;
                }
            }
            pos += 9;
        }
        
        // Estimate triangles (very rough)
        info.triangle_count = info.vertex_count / 3;
        
        // Look for engine hints in metadata
        if (content.find("customData") != std::string::npos) {
            size_t metadata_pos = content.find("customData");
            if (metadata_pos != std::string::npos) {
                size_t end_bracket = content.find(")", metadata_pos);
                if (end_bracket != std::string::npos) {
                    std::string metadata = content.substr(metadata_pos, end_bracket - metadata_pos + 1);
                    if (metadata.find("Unity") != std::string::npos) {
                        info.engine_hint = "Unity";
                    } else if (metadata.find("Unreal") != std::string::npos) {
                        info.engine_hint = "Unreal Engine";
                    }
                }
            }
        }
        
    } catch (const std::exception& e) {
        info.error_message = std::string("USD parsing failed: ") + e.what();
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_usdz(const Path& path) {
    // USDZ is a ZIP archive containing USD files
    // For now, we'll treat it as a generic file and extract basic info
    return analyze_generic(path, FileType::USDZ);
}

ModelInfo ModelAnalyzer::analyze_blend(const Path& path) {
    ModelInfo info;
    info.format = "BLEND";
    
    // Blender file is binary - we'll read basic header info
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        info.error_message = "Cannot open Blender file";
        return info;
    }
    
    // Read Blender header (first 12 bytes)
    char header[12];
    file.read(header, 12);
    
    if (std::memcmp(header, "BLENDER", 7) != 0) {
        info.error_message = "Invalid Blender file header";
        return info;
    }
    
    // Version is in bytes 7-10
    int version = (static_cast<unsigned char>(header[7]) << 0) |
                  (static_cast<unsigned char>(header[8]) << 8) |
                  (static_cast<unsigned char>(header[9]) << 16) |
                  (static_cast<unsigned char>(header[10]) << 24);
    
    // Endianness is in byte 11
    bool is_little_endian = (header[11] == '-');
    
    // For now, we'll estimate based on file size and known Blender file structure
    // Blender files have a specific structure with DNA1/RNA1 sections
    
    // Search for mesh data blocks
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string file_data(buffer.begin(), buffer.end());
    
    // Look for "Mesh" identifiers
    size_t mesh_count = std::count(file_data.begin(), file_data.end(), 'M') / 10; // Rough
    if (mesh_count > 0) info.mesh_count = mesh_count;
    
    // Look for vertex data (very approximate)
    size_t vert_pos = file_data.find("float[3]");
    if (vert_pos != std::string::npos) {
        // Rough estimate based on remaining file size
        size_t remaining_size = file_data.size() - vert_pos;
        info.vertex_count = std::min(remaining_size / (3 * 4), 1000000UL); // 3 floats per vertex, 4 bytes each
    }
    
    // Estimate triangles
    if (info.vertex_count > 0) {
        info.triangle_count = std::min(info.vertex_count / 3, 500000UL);
    }
    
    // Blender-specific info
    info.engine_hint = "Blender";
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_max(const Path& path) {
    ModelInfo info;
    info.format = "3DS MAX";
    
    // 3DS Max files can be .max (binary) or .3ds
    // We'll do basic analysis based on file structure
    
    if (path.extension() == ".3ds") {
        // Old 3DS format
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            info.error_message = "Cannot open 3DS file";
            return info;
        }
        
        // Read main chunk
        uint16_t chunk_id;
        uint32_t chunk_length;
        
        while (file.peek() != EOF) {
            file.read(reinterpret_cast<char*>(&chunk_id), 2);
            file.read(reinterpret_cast<char*>(&chunk_length), 4);
            
            if (chunk_id == 0x4D4D) { // MAIN3DS chunk
                // Continue reading sub-chunks
            } else if (chunk_id == 0x3D3D) { // EDIT3DS chunk
                // Continue reading sub-chunks
            } else if (chunk_id == 0x4000) { // EDIT_OBJECT
                // Object block
                info.mesh_count++;
            } else if (chunk_id == 0x4100) { // OBJ_TRIMESH
                // Triangular mesh
                // Skip to vertex list
            } else if (chunk_id == 0x4110) { // VERTICES - vertex list
                // Read number of vertices
                uint16_t num_vertices;
                file.read(reinterpret_cast<char*>(&num_vertices), 2);
                info.vertex_count += num_vertices;
                
                // Skip vertex coordinates (3 floats each)
                file.seekg(num_vertices * 3 * 4, std::ios::cur);
            } else if (chunk_id == 0x4120) { // VERTICES - face list
                // Read number of faces
                uint16_t num_faces;
                file.read(reinterpret_cast<char*>(&num_faces), 2);
                info.triangle_count += num_faces;
                
                // Skip face data (each face has 5 values: A,B,C,flags,material)
                file.seekg(num_faces * 5 * 2, std::ios::cur);
            }
            
            // Move to next chunk
            file.seekg(chunk_length - 6, std::ios::cur); // 6 bytes for id+length already read
        }
    } else {
        // .max file - treat as complex binary
        // For now, estimate based on file size
        info.vertex_count = std::min(fs::file_size(path) / 50, 1000000UL);
        if (info.vertex_count > 0) {
            info.triangle_count = std::min(vertex_count / 3, 500000UL);
        }
        info.mesh_count = std::max(1UL, fs::file_size(path) / (1024 * 1024)); // Rough: 1 mesh per MB
    }
    
    info.engine_hint = "3DS Max";
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_ma(const Path& path) {
    ModelInfo info;
    info.format = "MAYA ASCII";
    
    // Maya ASCII (.ma) files are text-based
    try {
        std::string content = StringUtils::read_text_file(path).value_or("");
        
        if (content.empty()) {
            info.error_message = "Empty Maya file";
            return info;
        }
        
        // Count geometry shapes
        size_t pos = 0;
        while ((pos = content.find("createNode mesh", pos)) != std::string::npos) {
            info.mesh_count++;
            pos += 15; // Move past "createNode mesh"
        }
        
        // Count vertices by looking for point array declarations
        pos = 0;
        while ((pos = content.find(".p[", pos)) != std::string::npos) {
            // Found a point array, now find the size
            size_t eq_pos = content.find(" = ", pos);
            if (eq_pos != std::string::npos) {
                size_t bracket_open = content.find("[", eq_pos);
                size_t bracket_close = content.find("]", eq_pos);
                if (bracket_open != std::string::npos && bracket_close != std::string::npos && bracket_open < bracket_close) {
                    std::string size_str = content.substr(bracket_open + 1, bracket_close - bracket_open - 1);
                    try {
                        uint32_t num_points = std::stoi(size_str);
                        // Each point is a vertex
                        info.vertex_count += num_points;
                    } catch (...) {}
                }
            }
            pos += 3; // Move past ".p["
        }
        
        // Count faces/vertices by looking for face vertex arrays
        pos = 0;
        while ((pos = content.find(".fv[", pos)) != std::string::npos) {
            // Found a face vertex array, now find the size
            size_t eq_pos = content.find(" = ", pos);
            if (eq_pos != std::string::npos) {
                size_t bracket_open = content.find("[", eq_pos);
                size_t bracket_close = content.find("]", eq_pos);
                if (bracket_open != std::string::npos && bracket_close != std::string::npos && bracket_open < bracket_close) {
                    std::string size_str = content.substr(bracket_open + 1, bracket_close - bracket_open - 1);
                    try {
                        uint32_t num_indices = std::stoi(size_str);
                        // Each face has at least 3 vertices (triangle)
                        // But we'll count triangles based on indices
                        // In Maya, face vertex array contains vertex indices for each face
                        // For simplicity, we'll estimate triangles as total_indices / 3
                        info.triangle_count += num_indices / 3;
                    } catch (...) {}
                }
            }
            pos += 4; // Move past ".fv["
        }
        
        // Count materials
        pos = 0;
        while ((pos = content.find("createNode lambert", pos)) != std::string::npos ||
               (pos = content.find("createNode phong", pos)) != std::string::npos ||
               (pos = content.find("createNode blinn", pos)) != std::string::npos) {
            info.material_count++;
            if (pos != std::string::npos) {
                pos += 16; // Move past "createNode lambert" (longest)
            }
        }
        
        // Estimate triangle count if not already set from face vertices
        if (info.triangle_count == 0 && info.vertex_count > 0) {
            // Rough estimate: assume triangular mesh
            info.triangle_count = std::max(1UL, info.vertex_count / 3);
        }
        
    } catch (const std::exception& e) {
        info.error_message = std::string("Maya ASCII parsing failed: ") + e.what();
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_mb(const Path& path) {
    ModelInfo info;
    info.format = "MAYA BINARY";
    
    // Maya Binary (.mb) files are binary - we'll do basic analysis
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            info.error_message = "Cannot open Maya binary file";
            return info;
        }
        
        // Read header
        char magic[4];
        file.read(magic, 4);
        
        if (std::memcmp(mask, "FFEE", 4) != 0 && std::memcmp(mask, "EEFF", 4) != 0) {
            info.error_message = "Invalid Maya binary file header";
            return info;
        }
        
        // For now, estimate based on file size
        // Maya binary files are complex, so we'll use heuristics
        uint64_t file_size = fs::file_size(path);
        
        // Rough estimates based on typical Maya file structure
        info.vertex_count = std::min(file_size / 100, 1000000UL);
        if (info.vertex_count > 0) {
            info.triangle_count = std::min(info.vertex_count / 3, 500000UL);
        }
        info.mesh_count = std::max(1UL, file_size / (512 * 1024)); // Rough: 1 mesh per 512KB
        info.material_count = std::max(1UL, file_size / (10 * 1024 * 1024)); // Rough: 1 material per 10MB
        
    } catch (const std::exception& e) {
        info.error_message = std::string("Maya binary parsing failed: ") + e.what();
    }
    
    info.engine_hint = "Maya";
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_x(const Path& path) {
    ModelInfo info;
    info.format = "DIRECTX X";
    
    // DirectX X files are text-based
    try {
        std::string content = StringUtils::read_text_file(path).value_or("");
        
        if (content.empty()) {
            info.error_message = "Empty DirectX X file";
            return info;
        }
        
        // Check for xof header
        if (content.find("xof ") != 0 && !content.starts_with("xof ")) {
            info.error_message = "Invalid DirectX X file header";
            return info;
        }
        
        // Count meshes by looking for Mesh templates
        size_t pos = 0;
        while ((pos = content.find("Mesh", pos)) != std::string::npos) {
            // Look for the mesh data block
            size_t brace_open = content.find("{", pos);
            if (brace_open != std::string::npos) {
                // Found a mesh, increment count
                info.mesh_count++;
                pos = brace_open + 1;
            } else {
                pos += 4;
            }
        }
        
        // Count vertices by looking for Mesh vertex arrays
        pos = 0;
        while ((pos = content.find(";", pos)) != std::string::npos) {
            // Look for patterns like "nVerts;" followed by coordinates
            size_t semicolon_pos = pos;
            // Look backwards for a number followed by semicolon
            size_t num_start = semicolon_pos;
            while (num_start > 0 && std::isdigit(content[num_start - 1])) {
                num_start--;
            }
            if (num_start < semicolon_pos) {
                std::string num_str = content.substr(num_start, semicolon_pos - num_start);
                try {
                    uint32_t count = std::stoul(num_str);
                    // Check if this looks like a vertex count (followed by 3 floats per vertex)
                    size_t after_semicolon = semicolon_pos + 1;
                    if (after_semicolon < content.size() && 
                        (content.substr(after_semicolon, 3) == ";;;" || 
                         content.substr(after_semicolon, 3) == ";;,")) {
                        // This looks like a vertex list declaration
                        info.vertex_count += count;
                    }
                } catch (...) {}
            }
            pos++;
        }
        
        // Count faces by looking for mesh face arrays
        pos = 0;
        while ((pos = content.find(";", pos)) != std::string::npos) {
            // Look for patterns like "nFaces;" followed by face indices
            size_t semicolon_pos = pos;
            size_t num_start = semicolon_pos;
            while (num_start > 0 && std::isdigit(content[num_start - 1])) {
                num_start--;
            }
            if (num_start < semicolon_pos) {
                std::string num_str = content.substr(num_start, semicolon_pos - num_start);
                try {
                    uint32_t count = std::stoul(num_str);
                    // Check if this looks like a face count (followed by triangle data)
                    size_t after_semicolon = semicolon_pos + 1;
                    if (after_semicolon < content.size()) {
                        std::string after = content.substr(after_semicolon, 10);
                        if (after.find("3;") != std::string::npos || 
                            after.find("3,", 0) != std::string::npos) {
                            // This looks like a face list with triangles
                            info.triangle_count += count;
                        }
                    }
                } catch (...) {}
            }
            pos++;
        }
        
        // Count materials by looking for Material templates
        pos = 0;
        while ((pos = content.find("Material", pos)) != std::string::npos) {
            // Look for material definition
            size_t brace_open = content.find("{", pos);
            if (brace_open != std::string::npos) {
                info.material_count++;
                pos = brace_open + 1;
            } else {
                pos += 8;
            }
        }
        
    } catch (const std::exception& e) {
        info.error_message = std::string("DirectX X parsing failed: ") + e.what();
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_mdl(const Path& path) {
    ModelInfo info;
    info.format = "STUDIO MDL";
    
    // Valve Studio MDL files are binary
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            info.error_message = "Cannot open MDL file";
            return info;
        }
        
        // Read IDST header
        char idst[4];
        file.read(idst, 4);
        
        if (std::memcmp(idst, "IDST", 4) != 0) {
            // Try little-endian version
            char idst2[4] = {'I', 'D', 'S', 'T'};
            if (std::memcmp(idst, idst2, 4) != 0) {
                info.error_message = "Invalid MDL file header";
                return info;
            }
        }
        
        // Read version
        int version;
        file.read(reinterpret_cast<char*>(&version), 4);
        
        // Skip to stats section
        // For simplicity, we'll estimate based on file size
        // MDL files have a known structure but it's complex to parse fully
        
        uint64_t file_size = fs::file_size(path);
        
        // Rough estimates based on typical MDL structure
        // Header is about 100-200 bytes, then studioshdr_t, then various lumps
        if (file_size > 2048) { // Only estimate for reasonably sized files
            // Assume vertices take about 12 bytes each (position + normal + texcoord)
            // But MDL can have multiple LODs
            size_t usable_bytes = std::min(file_size - 512, file_size * 3 / 4); // Reserve space for headers/etc
            info.vertex_count = std::min(usable_bytes / 12, 500000UL);
            
            if (info.vertex_count > 0) {
                // Triangles: typically 3 indices per triangle, 2 or 4 bytes per index
                // Assume 2 bytes per index for simplicity
                info.triangle_count = std::min(usable_bytes / 6, 500000UL);
            }
            
            // Estimate mesh count based on body parts
            info.mesh_count = std::max(1UL, file_size / (2 * 1024 * 1024)); // Rough: 1 mesh per 2MB
            
            // Materials are usually fewer
            info.material_count = std::max(1UL, file_size / (5 * 1024 * 1024)); // Rough: 1 material per 5MB
        }
        
    } catch (const std::exception& e) {
        info.error_message = std::string("MDL parsing failed: ") + e.what();
    }
    
    // Try to detect engine from file content
    std::ifstream file(path, std::ios::binary);
    if (file) {
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (content.find("Valve") != std::string::npos || 
            content.find("Half-Life") != std::string::npos ||
            content.find("Counter-Strike") != std::string::npos ||
            content.find("Team Fortress") != std::string::npos) {
            info.engine_hint = "Source Engine";
        } else if (content.find("GoldSrc") != std::string::npos) {
            info.engine_hint = "GoldSrc Engine";
        }
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_nif(const Path& path) {
    ModelInfo info;
    info.format = "NETIMMERSE/NIF";
    
    // NetImmerse/Gamebryo NIF files are binary
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            info.error_message = "Cannot open NIF file";
            return info;
        }
        
        // Check header
        char header[4];
        file.read(header, 4);
        
        if (std::memcmp(header, "NIFF", 4) != 0) {
            // Try little-endian versions
            char nif1[4] = {'N', 'I', 'F', 'F'};
            char nif2[4] = {'F', 'F', 'I', 'N'};
            if (std::memcmp(header, nif1, 4) != 0 && 
                std::memcmp(header, nif2, 4) != 0) {
                info.error_message = "Invalid NIF file header";
                return info;
            }
        }
        
        // Read version
        uint16_t version;
        file.read(reinterpret_cast<char*>(&version), 2);
        
        // Skip to the rest of the header (version dependent)
        // For simplicity, we'll do a basic scan for NiTriShape and NiTriStrips
        
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        // Count NiTriShape and NiTriStrips (these contain geometry)
        size_t trishape_count = std::count(content.begin(), content.end(), 'N') / 10; // Rough
        size_t tristrip_count = std::count(content.begin(), content.end(), 'N') / 15; // Rough
        
        if (trishape_count > 0) {
            // Each NiTriShape typically has one mesh
            info.mesh_count = trishape_count;
            
            // Estimate vertices: look for m_usVertexCount or similar patterns
            // This is very approximate
            size_t vert_pos = 0;
            while ((vert_pos = content.find("m_v", vert_pos)) != std::string::npos) {
                // Look for patterns like "m_usVertexCount" or "m_uiVertexCount"
                size_t test_pos = vert_pos;
                if (test_pos + 20 < content.size()) {
                    std::string test_str = content.substr(test_pos, 20);
                    if (test_str.find("VertexCount") != std::string::npos) {
                        // Found vertex count field, try to extract the value
                        // This is getting too complex for a simple scanner - estimate instead
                        info.vertex_count += 50; // Rough estimate per shape
                    }
                }
                vert_pos += 2;
            }
            
            if (info.vertex_count == 0 && info.mesh_count > 0) {
                // Fallback estimate
                info.vertex_count = info.mesh_count * 100;
            }
            
            // Estimate triangles
            if (info.vertex_count > 0) {
                // Assume mostly triangular meshes
                info.triangle_count = std::min(info.vertex_count / 3 * 2, 500000UL); // Slightly overestimate
            }
            
        } else if (tristrip_count > 0) {
            // NiTriStrips - each strip with n vertices creates (n-2) triangles
            // Very rough estimate
            info.vertex_count = tristrip_count * 50; // Rough estimate
            info.triangle_count = tristrip_count * 40; // Rough estimate
            info.mesh_count = tristrip_count;
        }
        
        // Estimate bones (NIF files can have bones for skeletal animation)
        size_t bone_count = std::count(content.begin(), content.end(), 'b') / 20; // Rough
        info.bone_count = bone_count;
        
    } catch (const std::exception& e) {
        info.error_message = std::string("NIF parsing failed: ") + e.what();
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_hkx(const Path& path) {
    ModelInfo info;
    info.format = "HAVOK HKX";
    
    // Havok HKX files are binary
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            info.error_message = "Cannot open HKX file";
            return info;
        }
        
        // Check for Havok signature
        char signature[4];
        file.read(signature, 4);
        
        if (std::memcmp(signature, "\x1E\x0D\x0D\x0A", 4) != 0) { // HKX signature
            // Try alternative signatures
            char alt_sig[4] = {'H', 'a', 'v', 'o'};
            if (std::memcmp(signature, alt_sig, 4) != 0) {
                info.error_message = "Invalid HKX file header";
                return info;
            }
        }
        
        // Read version info (simplified)
        uint16_t hx_version;
        file.read(reinterpret_cast<char*>(&hx_version), 2);
        
        // For now, estimate based on file size
        // HKX files contain animation data, collision data, etc.
        uint64_t file_size = fs::file_size(path);
        
        // Rough estimates
        // HKX files can be large due to animation data
        if (info.disk_size > 1024) {
            // Assume some portion is mesh data
            size_t mesh_data_size = std::min(info.disk_size / 4, 10 * 1024 * 1024); // Max 10MB for mesh data
            info.vertex_count = std::min(mesh_data_size / 50, 500000UL); // Rough estimate
            if (info.vertex_count > 0) {
                info.triangle_count = std::min(vertex_count / 3, 500000UL);
            }
            info.mesh_count = std::max(1UL, file_size / (5 * 1024 * 1024)); // Rough: 1 mesh per 5MB
        }
        
    } catch (const std::exception& e) {
        info.error_message = std::string("HKX parsing failed: ") + e.what();
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_gr2(const Path& path) {
    ModelInfo info;
    info.format = "GRANNY 2";
    
    // Granny 2 files are binary
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            info.error_message = "Cannot open GR2 file";
            return info;
        }
        
        // Check for Granny signature
        char signature[8];
        file.read(signature, 8);
        
        if (std::memcmp(signature, "Granny 2", 8) != 0) {
            info.error_message = "Invalid GR2 file header";
            return info;
        }
        
        // Read version
        uint32_t version;
        file.read(reinterpret_cast<char*>(&volume), 4);
        
        // For now, estimate based on file size
        // Granny files contain meshes, skeletons, animations, textures, etc.
        uint64_t file_size = fs::file_size(path);
        
        if (file_size > 1024) {
            // Estimate mesh data portion
            size_t mesh_data_size = std::min(file_size / 3, 20 * 1024 * 1024); // Max 20MB for mesh data
            info.vertex_count = std::min(mesh_data_size / 30, 1000000UL); // Rough estimate
            if (info.vertex_count > 0) {
                info.triangle_count = std::min(vertex_count / 3, 500000UL);
            }
            info.mesh_count = std::max(1UL, file_size / (2 * 1024 * 1024)); // Rough: 1 mesh per 2MB
            info.material_count = std::max(1UL, file_size / (5 * 1024 * 1024)); // Rough: 1 material per 5MB
        }
        
    } catch (const std::exception& e) {
        info.error_message = std::string("GR2 parsing failed: ") + e.what();
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_smd(const Path& path) {
    ModelInfo info;
    info.format = "STUDIO MODEL";
    
    // SMD files are text-based
    try {
        std::string content = StringUtils::read_text_file(path).value_or("");
        
        if (content.empty()) {
            info.error_message = "Empty SMD file";
            return info;
        }
        
        // Count vertices by looking for vertex data
        size_t pos = 0;
        while ((pos = content.find("vertex ", pos)) != std::string::npos) {
            // Found a vertex declaration
            // Format: "vertex 0 0.000 0.000 0.000 0.000 0.000"
            size_t end_line = content.find('\n', pos);
            if (end_line != std::string::npos) {
                std::string line = content.substr(pos, end_line - pos);
                // Count this as one vertex
                info.vertex_count++;
            }
            pos += 7; // Move past "vertex "
        }
        
        // Count triangles by looking for triangle data
        pos = 0;
        while ((pos = content.find("triangle ", pos)) != std::string::nop) {
            // Found a triangle definition
            // Format: "triangle 0 0 1 2"
            size_t end_line = content.find('\n', pos);
            if (end_line != std::string::npos) {
                // Count this as one triangle
                info.triangle_count++;
            }
            pos += 9; // Move past "triangle "
        }
        
        // Estimate mesh count (SMD files usually have one model per file)
        info.mesh_count = 1;
        
        // Estimate materials (usually referenced in accompanying .vt files)
        // For now, default to 1
        info.material_count = 1;
        
        // SMD files are often used for skeletal animation
        // Look for skeleton section
        if (content.find("skeleton") != std::string::npos) {
            // Count bones by looking for bone lines
            size_t bone_pos = 0;
            while ((bone_pos = content.find("bone ", bone_pos)) != std::string::npos) {
                info.bone_count++;
                bone_pos += 5; // Move past "bone "
            }
        }
        
    } catch (const std::exception& e) {
        info.error_message = std::string("SMD parsing failed: ") + e.what();
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_dmx(const Path& path) {
    ModelInfo info;
    info.format = "SOURCE DMX";
    
    // DMX files are text-based (XML-like)
    try {
        std::string content = StringUtils::read_text_file(path).value_or("");
        
        if (content.empty()) {
            info.error_message = "Empty DMX file";
            return info;
        }
        
        // Count vertex elements by looking for vertexdata tags
        size_t pos = 0;
        while ((pos = content.find("<vertexdata", pos)) != std::string::npos) {
            // Found vertex data element
            // Look for the count attribute or count the vertices inside
            size_t end_tag = content.find("</vertexdata>", pos);
            if (end_tag != std::string::npos) {
                std::string vertex_data = content.substr(pos, end_tag - pos);
                // Count vertices by looking for <v> or <vertex> tags
                size_t v_count = std::count(vertex_data.begin(), vertex_data.end(), '<') / 3; // Rough
                if (v_count > 0) {
                    info.vertex_count += v_count;
                }
            }
            pos += 11; // Move past "<vertexdata"
        }
        
        // Count index elements (triangles)
        pos = 0;
        while ((pos = content.find("<indices", pos)) != std::string::npos) {
            size_t end_tag = content.find("</indices>", pos);
            if (end_tag != std::string::npos) {
                std::string indices_data = content.substr(pos, end_tag - pos);
                // Count indices by looking for numbers
                // Each triangle has 3 indices
                size_t num_count = std::count_if(indices_data.begin(), indices_data.end(),
                                                [](char c){ return std::isdigit(c); });
                if (num_count > 0) {
                    // Rough estimate: each index is a number, triangles = indices/3
                    info.triangle_count += num_count / 10; // Very rough
                }
            }
            pos += 8; // Move past "<indices"
        }
        
        // Count meshes by looking for mesh elements
        pos = 0;
        while ((pos = content.find("<mesh", pos)) != std::string::npos) {
            info.mesh_count++;
            pos += 5; // Move past "<mesh"
        }
        
        // Count materials
        pos = 0;
        while ((pos = content.find("<material", pos)) != std::string::npos) {
            info.material_count++;
            pos += 9; // Move past "<material"
        }
        
        // DMX files are often used with Source engine
        if (content.find("source") != std::string::npos || 
            content.find("valve") != std::string::npos) {
            info.engine_hint = "Source Engine";
        }
        
    } catch (const std::exception& e) {
        info.error_message = std::string("DMX parsing failed: ") + e.what();
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_generic(const Path& path, FileType type) {
    ModelInfo info;
    info.disk_size = fs::file_size(path);
    
    // Try to determine if it's likely a model file based on extension
    std::string ext = StringUtils::to_lower(path.extension().string());
    if (!ext.empty() && ext[0] == '.') {
        ext = ext.substr(1);
    }
    
    // Known model extensions that we might not have specific handlers for
    static const std::unordered_set<std::string> model_exts = {
        "obj", "fbx", "gltf", "glb", "dae", "3ds", "max", "blend", 
        "ma", "mb", "x", "mdl", "md2", "md3", "md5", "nif", "hkx",
        "gr2", "smd", "dmx", "usd", "usda", "usdc", "usdz", "ply", "stl"
    };
    
    bool is_likely_model = model_exts.find(ext) != model_exts.end();
    
    if (is_likely_model || info.disk_size > 1024) { // Assume files >1KB might be models
        // Make reasonable assumptions
        info.format = "UNKNOWN";
        info.is_compiled = false;
        
        // Very rough estimate: assume it's a mesh with vertex data
        // This is just a placeholder - real analysis would be much more sophisticated
        if (info.disk_size > 0) {
            // Assume 12 bytes per vertex (position + normal + texcoord as floats)
            uint64_t estimated_vertices = info.disk_size / 12;
            info.vertex_count = std::min(estimated_vertices, 1000000UL); // Cap at 1M
            
            if (info.vertex_count > 0) {
                // Assume triangle mesh
                info.triangle_count = std::min(vertex_count / 3, 500000UL);
            }
            
            // Assume one mesh per file
            info.mesh_count = 1;
            
            // Assume some materials
            info.material_count = std::max(1UL, info.disk_size / (1024 * 1024)); // 1 per MB
        }
    } else {
        // Definitely not a model
        info.vertex_count = 0;
        info.triangle_count = 0;
        info.mesh_count = 0;
        info.material_count = 0;
        info.bone_count = 0;
    }
    
    // Estimate VRAM usage
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

u64 ModelAnalyzer::estimate_vram(const ModelInfo& model) const {
    // Estimate VRAM usage based on vertex data, index data, and textures
    uint64_t vram = 0;
    
    // Vertex buffers: position (3 floats) + normal (3 floats) + texcoord (2 floats) = 8 floats per vertex
    // Assuming 4 bytes per float = 32 bytes per vertex
    vram += model.vertex_count * 32;
    
    // Index buffers: assuming 32-bit indices
    vram += model.triangle_count * 3 * 4; // 3 indices per triangle, 4 bytes each
    
    // Texture estimates: rough estimate based on material count
    // Assume average texture size of 1MB per material
    vram += model.material_count * 1024 * 1024;
    
    // Bone matrices: if we have bones, assume 4x4 matrix per bone (16 floats)
    if (model.bone_count > 0) {
        vram += model.bone_count * 16 * 4; // 16 floats * 4 bytes each
    }
    
    // Add some overhead
    vram = vram * 1.5;
    
    return vram;
}

ModelStats ModelAnalyzer::stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

String ModelAnalyzer::generate_report() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    std::stringstream ss;
    ss << "Model Analysis Report\n";
    ss << "=====================\n";
    ss << "Total Models: " << stats_.total_models << "\n";
    ss << "Total Vertices: " << StringUtils::format_number(stats_.total_vertices) << "\n";
    ss << "Total Triangles: " << StringUtils::format_number(stats_.total_triangles) << "\n";
    ss << "Total Meshes: " << stats_.total_meshes << "\n";
    ss << "Total Materials: " << stats_.total_materials << "\n";
    ss << "Total Bones: " << stats_.total_bones << "\n";
    ss << "Total VRAM Estimate: " << StringUtils::format_bytes(stats_.total_vram) << "\n";
    ss << "Total Disk Size: " << StringUtils::format_bytes(stats_.total_disk_size) << "\n\n";
    
    if (!stats_.format_counts.empty()) {
        ss << "Format Distribution:\n";
        for (const auto& [format, count] : stats_.format_counts) {
            ss << "  " << format << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.engine_counts.empty()) {
        ss << "Engine Distribution:\n";
        for (const auto& [engine, count] : stats_.engine_counts) {
            ss << "  " << engine << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    return ss.str();
}

} // namespace game_req
        
        // Check header
        char header[4];
        file.read(header, 4);
        
        if (std::memcmp(header, "NIFF", 4) != 0) {
            // Try little-endian versions
            char nif1[4] = {'N', 'I', 'F', 'F'};
            char nif2[4] = {'F', 'F', 'I', 'N'};
            if (std::memcmp(header, nif1, 4) != 0 && 
                std::memcmp(header, nif2, 4) != 0) {
                info.error_message = "Invalid NIF file header";
                return info;
            }
        }
        
        // Read version
        uint16_t version;
        file.read(reinterpret_cast<char*>(&version), 2);
        
        // Skip to the rest of the header (version dependent)
        // For simplicity, we'll do a basic scan for NiTriShape and NiTriStrips
        
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        // Count NiTriShape and NiTriStrips (these contain geometry)
        size_t trishape_count = std::count(content.begin(), content.end(), 'N') / 10; // Rough
        size_t tristrip_count = std::count(content.begin(), content.end(), 'N') / 15; // Rough
        
        if (trishape_count > 0) {
            // Each NiTriShape typically has one mesh
            info.mesh_count = trishape_count;
            
            // Estimate vertices: look for m_usVertexCount or similar patterns
            // This is very approximate
            size_t vert_pos = 0;
            while ((vert_pos = content.find("m_v", vert_pos)) != std::string::npos) {
                // Look for patterns like "m_usVertexCount" or "m_uiVertexCount"
                size_t test_pos = vert_pos;
                if (test_pos + 20 < content.size()) {
                    std::string test_str = content.substr(test_pos, 20);
                    if (test_str.find("VertexCount") != std::string::npos) {
                        // Found vertex count field, try to extract the value
                        // This is getting too complex for a simple scanner - estimate instead
                        info.vertex_count += 50; // Rough estimate per shape
                    }
                }
                vert_pos += 2;
            }
            
            if (info.vertex_count == 0 && info.mesh_count > 0) {
                // Fallback estimate
                info.vertex_count = info.mesh_count * 100;
            }
            
            // Estimate triangles
            if (info.vertex_count > 0) {
                // Assume mostly triangular meshes
                info.triangle_count = std::min(info.vertex_count / 3 * 2, 500000UL); // Slightly overestimate
            }
            
        } else if (tristrip_count > 0) {
            // NiTriStrips - each strip with n vertices creates (n-2) triangles
            // Very rough estimate
            info.vertex_count = tristrip_count * 50; // Rough estimate
            info.triangle_count = tristrip_count * 40; // Rough estimate
            info.mesh_count = tristrip_count;
        }
        
        // Estimate bones (NIF files can have bones for skeletal animation)
        size_t bone_count = std::count(content.begin(), content.end(), 'b') / 20; // Rough
        info.bone_count = bone_count;
        
    } catch (const std::exception& e) {
        info.error_message = std::string("NIF parsing failed: ") + e.what();
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_hkx(const Path& path) {
    ModelInfo info;
    info.format = "HAVOK HKX";
    
    // Havok HKX files are binary
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            info.error_message = "Cannot open HKX file";
            return info;
        }
        
        // Check for Havok signature
        char signature[4];
        file.read(signature, 4);
        
        if (std::memcmp(signature, "\x1E\x0D\x0D\x0A", 4) != 0) { // HKX signature
            // Try alternative signatures
            char alt_sig[4] = {'H', 'a', 'v', 'o'};
            if (std::memcmp(signature, alt_sig, 4) != 0) {
                info.error_message = "Invalid HKX file header";
                return info;
            }
        }
        
        // Read version info (simplified)
        uint16_t hx_version;
        file.read(reinterpret_cast<char*>(&hx_version), 2);
        
        // For now, estimate based on file size
        // HKX files contain animation data, collision data, etc.
        uint64_t file_size = fs::file_size = fs::file_size(path);
        
        // Rough estimates
        // HKX files can be large due to animation data
        if (info.disk_size > 1024) {
            // Assume some portion is mesh data
            size_t mesh_data_size = std::min(info.disk_size / 4, 10 * 1024 * 1024); // Max 10MB for mesh data
            info.vertex_count = std::min(mesh_data_size / 50, 500000UL); // Rough estimate
            if (info.vertex_count > 0) {
                info.triangle_count = std::min(vertex_count / 3, 500000UL);
            }
            info.mesh_count = std::max(1UL, file_size / (5 * 1024 * 1024)); // Rough: 1 mesh per 5MB
        }
        
    } catch (const std::exception& e) {
        info.error_message = std::string("HKX parsing failed: ") + e.what();
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_gr2(const Path& path) {
    ModelInfo info;
    info.format = "GRANNY 2";
    
    // Granny 2 files are binary
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            info.error_message = "Cannot open GR2 file";
            return info;
        }
        
        // Check for Granny signature
        char signature[8];
        file.read(signature, 8);
        
        if (std::memcmp(signature, "Granny 2", 8) != 0) {
            info.error_message = "Invalid GR2 file header";
            return info;
        }
        
        // Read version
        uint32_t version;
        file.read(reinterpret_cast<char*>(&version), 4);
        
        // For now, estimate based on file size
        // Granny files contain meshes, skeletons, animations, textures, etc.
        uint64_t file_size = fs::file_size(path);
        
        if (file_size > 1024) {
            // Estimate mesh data portion
            size_t mesh_data_size = std::min(file_size / 3, 20 * 1024 * 1024); // Max 20MB for mesh data
            info.vertex_count = std::min(mesh_data_size / 30, 1000000UL); // Rough estimate
            if (info.vertex_count > 0) {
                info.triangle_count = std::min(vertex_count / 3, 500000UL);
            }
            info.mesh_count = std::max(1UL, file_size / (2 * 1024 * 1024)); // Rough: 1 mesh per 2MB
            info.material_count = std::max(1UL, file_size / (5 * 1024 * 1024)); // Rough: 1 material per 5MB
        }
        
    } catch (const std::exception& e) {
        info.error_message = std::string("GR2 parsing failed: ") + e.what();
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_smd(const Path& path) {
    ModelInfo info;
    info.format = "STUDIO MODEL";
    
    // SMD files are text-based
    try {
        std::string content = StringUtils::read_text_file(path).value_or("");
        
        if (content.empty()) {
            info.error_message = "Empty SMD file";
            return info;
        }
        
        // Count vertices by looking for vertex data
        size_t pos = 0;
        while ((pos = content.find("vertex ", pos)) != std::string::npos) {
            // Found a vertex declaration
            // Format: "vertex 0 0.000 0.000 0.000 0.000 0.000"
            size_t end_line = content.find('\n', pos);
            if (end_line != std::string::npos) {
                std::string line = content.substr(pos, end_line - pos);
                // Count this as one vertex
                info.vertex_count++;
            }
            pos += 7; // Move past "vertex "
        }
        
        // Count triangles by looking for triangle data
        pos = 0;
        while ((pos = content.find("triangle ", pos)) != std::string::npos) {
            // Found a triangle definition
            // Format: "triangle 0 0 1 2"
            size_t end_line = content.find('\n', pos);
            if (end_line != std::string::npos) {
                // Count this as one triangle
                info.triangle_count++;
            }
            pos += 9; // Move past "triangle "
        }
        
        // Estimate mesh count (SMD files usually have one model per file)
        info.mesh_count = 1;
        
        // Estimate materials (usually referenced in accompanying .vt files)
        // For now, default to 1
        info.material_count = 1;
        
        // SMD files are often used for skeletal animation
        // Look for skeleton section
        if (content.find("skeleton") != std::string::npos) {
            // Count bones by looking for bone lines
            size_t bone_pos = 0;
            while ((bone_pos = content.find("bone ", bone_pos)) != std::string::npos) {
                info.bone_count++;
                bone_pos += 5; // Move past "bone "
            }
        }
        
    } catch (const std::exception& e) {
        info.error_message = std::string("SMD parsing failed: ") + e.what();
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_dmx(const Path& path) {
    ModelInfo info;
    info.format = "SOURCE DMX";
    
    // DMX files are text-based (XML-like)
    try {
        std::string content = StringUtils::read_text_file(path).value_or("");
        
        if (content.empty()) {
            info.error_message = "Empty DMX file";
            return info;
        }
        
        // Count vertex elements by looking for vertexdata tags
        size_t pos = 0;
        while ((pos = content.find("<vertexdata", pos)) != std::string::npos) {
            // Found vertex data element
            // Look for the count attribute or count the vertices inside
            size_t end_tag = content.find("</vertexdata>", pos);
            if (end_tag != std::string::npos) {
                std::string vertex_data = content.substr(pos, end_tag - pos);
                // Count vertices by looking for <v> or <vertex> tags
                size_t v_count = std::count(vertex_data.begin(), vertex_data.end(), '<') / 3; // Rough
                if (v_count > 0) {
                    info.vertex_count += v_count;
                }
            }
            pos += 11; // Move past "<vertexdata"
        }
        
        // Count index elements (triangles)
        pos = 0;
        while ((pos = content.find("<indices", pos)) != std::string::npos) {
            size_t end_tag = content.find("</indices>", pos);
            if (end_tag != std::string::npos) {
                std::string indices_data = content.substr(pos, end_tag - pos);
                // Count indices by looking for numbers
                // Each triangle has 3 indices
                size_t num_count = std::count_if(indices_data.begin(), indices_data.end(),
                                                [](char c){ return std::isdigit(c); });
                if (num_count > 0) {
                    // Rough estimate: each index is a number, triangles = indices/3
                    info.triangle_count += num_count / 10; // Very rough
                }
            }
            pos += 8; // Move past "<indices"
        }
        
        // Count meshes by looking for mesh elements
        pos = 0;
        while ((pos = content.find("<mesh", pos)) != std::string::npos) {
            info.mesh_count++;
            pos += 5; // Move past "<mesh"
        }
        
        // Count materials
        pos = 0;
        while ((pos = content.find("<material", pos)) != std::string::npos) {
            info.material_count++;
            pos += 9; // Move past "<material"
        }
        
        // DMX files are often used with Source engine
        if (content.find("source") != std::string::npos || 
            content.find("valve") != std::string::npos) {
            info.engine_hint = "Source Engine";
        }
        
    } catch (const std::exception& e) {
        info.error_message = std::string("DMX parsing failed: ") + e.what();
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

ModelInfo ModelAnalyzer::analyze_generic(const Path& path, FileType type) {
    ModelInfo info;
    info.disk_size = fs::file_size(path);
    
    // Try to determine if it's likely a model file based on extension
    std::string ext = StringUtils::to_lower(path.extension().string());
    if (!ext.empty() && ext[0] == '.') {
        ext = ext.substr(1);
    }
    
    // Known model extensions that we might not have specific handlers for
    static const std::unordered_set<std::string> model_exts = {
        "obj", "fbx", "gltf", "glb", "dae", "3ds", "max", "blend", 
        "ma", "mb", "x", "mdl", "md2", "md3", "md5", "nif", "hkx",
        "gr2", "smd", "dmx", "usd", "usda", "usdc", "usdz", "ply", "stl"
    };
    
    bool is_likely_model = model_exts.find(ext) != model_exts.end();
    
    if (is_likely_model || info.disk_size > 1024) { // Assume files >1KB might be models
        // Make reasonable assumptions
        info.format = "UNKNOWN";
        info.is_compiled = false;
        
        // Very rough estimate: assume it's a mesh with vertex data
        // This is just a placeholder - real analysis would be much more sophisticated
        if (info.disk_size > 0) {
            // Assume 12 bytes per vertex (position + normal + texcoord as floats)
            uint64_t estimated_vertices = info.disk_size / 12;
            info.vertex_count = std::min(estimated_vertices, 1000000UL); // Cap at 1M
            
            if (info.vertex_count > 0) {
                // Assume triangle mesh
                info.triangle_count = std::min(info.vertex_count / 3, 500000UL);
            }
            
            // Assume one mesh per file
            info.mesh_count = 1;
            
            // Assume some materials
            info.material_count = std::max(1UL, info.disk_size / (1024 * 1024)); // 1 per MB
        }
    } else {
        // Definitely not a model
        info.vertex_count = 0;
        info.triangle_count = 0;
        info.mesh_count = 0;
        info.material_count = 0;
        info.bone_count = 0;
    }
    
    // Estimate VRAM usage
    info.estimated_vram = estimate_vram(info);
    
    return info;
}

u64 ModelAnalyzer::estimate_vram(const ModelInfo& model) const {
    // Estimate VRAM usage based on vertex data, index data, and textures
    uint64_t vram = 0;
    
    // Vertex buffers: position (3 floats) + normal (3 floats) + texcoord (2 floats) = 8 floats per vertex
    // Assuming 4 bytes per float = 32 bytes per vertex
    vram += model.vertex_count * 32;
    
    // Index buffers: assuming 32-bit indices
    vram += model.triangle_count * 3 * 4; // 3 indices per triangle, 4 bytes each
    
    // Texture estimates: rough estimate based on material count
    // Assume average texture size of 1MB per material
    vram += model.material_count * 1024 * 1024;
    
    // Bone matrices: if we have bones, assume 4x4 matrix per bone (16 floats)
    if (model.bone_count > 0) {
        vram += model.bone_count * 16 * 4; // 16 floats * 4 bytes each
    }
    
    // Add some overhead
    vram = vram * 1.5;
    
    return vram;
}

ModelStats ModelAnalyzer::stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

String ModelAnalyzer::generate_report() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    std::stringstream ss;
    ss << "Model Analysis Report\n";
    ss << "=====================\n";
    ss << "Total Models: " << stats_.total_models << "\n";
    ss << "Total Vertices: " << StringUtils::format_number(stats_.total_vertices) << "\n";
    ss << "Total Triangles: " << StringUtils::format_number(stats_.total_triangles) << "\n";
    ss << "Total Meshes: " << stats_.total_meshes << "\n";
    ss << "Total Materials: " << stats_.total_materials << "\n";
    ss << "Total Bones: " << stats_.total_bones << "\n";
    ss << "Total VRAM Estimate: " << StringUtils::format_bytes(stats_.total_vram) << "\n";
    ss << "Total Disk Size: " << StringUtils::format_bytes(stats_.total_disk_size) << "\n\n";
    
    if (!stats_.format_counts.empty()) {
        ss << "Format Distribution:\n";
        for (const auto& [format, count] : stats_.format_counts) {
            ss << "  " << format << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.engine_counts.empty()) {
        ss << "Engine Distribution:\n";
        for (const auto& [engine, count] : stats_.engine_counts) {
            ss << "  " << engine << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    return ss.str();
}

} // namespace game_req
    }
    
    info.disk_size = fs::file_size(path);
    info.estimated_vram = estimate_vram(info);
    
    return info;
}