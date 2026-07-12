#include <game_req_analyzer/analyzers/shader_analyzer.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <algorithm>
#include <fstream>
#include <regex>
#include <cstring>

namespace game_req {

ShaderAnalyzer::ShaderAnalyzer(const AnalyzerConfig& config) : config_(config) {}

Result<std::vector<ShaderInfo>> ShaderAnalyzer::analyze(const std::vector<FileInfo>& shaders) {
    std::vector<ShaderInfo> results;
    results.reserve(shaders.size());
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_shaders = 0;
        stats_.total_instructions = 0;
        stats_.total_disk_size = 0;
        stats_.total_estimated_vram = 0;
        stats_.language_counts.clear();
        stats_.stage_counts.clear();
        stats_.profile_counts.clear();
        stats_.engine_counts.clear();
        stats_.max_instructions_per_shader = 0;
        stats_.max_registers_per_shader = 0;
    }
    
    for (const auto& shader : shaders) {
        auto result = analyze_single(shader);
        if (result) {
            results.push_back(*result);
            
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_shaders++;
            stats_.total_instructions += result->instruction_count;
            stats_.total_disk_size += result->disk_size;
            stats_.total_estimated_vram += result->estimated_vram;
            stats_.language_counts[result->language]++;
            stats_.stage_counts[result->stage]++;
            stats_.profile_counts[result->profile]++;
            if (!result->engine_hint.empty()) stats_.engine_counts[result->engine_hint]++;
            if (result->instruction_count > stats_.max_instructions_per_shader) {
                stats_.max_instructions_per_shader = result->instruction_count;
            }
            if (result->register_count > stats_.max_registers_per_shader) {
                stats_.max_registers_per_shader = result->register_count;
            }
        } else {
            LOG_WARN("Failed to analyze shader {}: {}", shader.path.string(), result.error().message);
        }
    }
    
    return results;
}

Result<ShaderInfo> ShaderAnalyzer::analyze_single(const FileInfo& shader) {
    ShaderInfo info;
    info.disk_size = shader.size;
    
    try {
        switch (shader.type) {
            case FileType::HLSL:
            case FileType::FX:
            case FileType::CGINC:
                return analyze_hlsl(shader.path);
            case FileType::GLSL:
            case FileType::VSH:
            case FileType::PSH:
            case FileType::GSH:
                return analyze_glsl(shader.path);
            case FileType::SPIRV:
                return analyze_spirv(shader.path);
            case FileType::METAL:
            case FileType::MSL:
                return analyze_msl(shader.path);
            case FileType::CG:
                return analyze_cg(shader.path);
            case FileType::SHADER:
            case FileType::FXO: {
                auto content = read_file_content(shader.path);
                if (!content.empty() && (content[0] == 0x30 || content[0] == 0xFE || content[0] == 0x44)) {
                    return analyze_dxbc(shader.path);
                }
                return analyze_hlsl(shader.path);
            }
            default:
                return analyze_generic(shader.path, shader.type);
        }
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Shader analysis failed: {}", e.what())));
    }
}

String ShaderAnalyzer::read_file_content(const Path& path) const {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return "";
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    String content;
    content.resize(size);
    if (size > 0) {
        file.read(content.data(), size);
    }
    return content;
}

Result<ShaderInfo> ShaderAnalyzer::analyze_hlsl(const Path& path) {
    ShaderInfo info;
    info.language = "HLSL";
    String content = read_file_content(path);
    info.disk_size = content.size();
    
    // Detect shader profile from pragmas or comments
    std::regex profile_regex(R"(#pragma\s+target\s+(\w+)|//\s*target\s+(\w+))");
    std::smatch match;
    if (std::regex_search(content, match, profile_regex)) {
        info.profile = match[1].matched ? match[1].str() : match[2].str();
    }
    
    // Detect entry point
    std::regex entry_regex(R"(\b(vs|ps|cs|gs|hs|ds|ms|as)\s+\w+\s*\()");
    if (std::regex_search(content, match, entry_regex)) {
        info.entry_point = match[0].str();
        info.stage = detect_stage_from_entry(info.entry_point);
    }
    
    // Count instructions (rough estimate from source)
    info.instruction_count = estimate_complexity(content, "HLSL");
    
    // Count registers (rough estimate)
    std::regex reg_regex(R"(\b(r\d+|v\d+|t\d+|s\d+|u\d+|b\d+)\b)");
    info.register_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), reg_regex),
        std::sregex_iterator()
    );
    
    // Count constant buffers
    std::regex cbuffer_regex(R"(cbuffer\s+\w+\s*:?\s*register\s*\(\s*b\d+\s*\))");
    info.constant_buffer_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), cbuffer_regex),
        std::sregex_iterator()
    );
    
    // Count texture/sampler registers
    std::regex tex_regex(R"(Texture2D\s+\w+\s*:?\s*register\s*\(\s*t\d+\s*\)|SamplerState\s+\w+\s*:?\s*register\s*\(\s*s\d+\s*\))");
    info.texture_sampler_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), tex_regex),
        std::sregex_iterator()
    );
    
    // Count UAVs
    std::regex uav_regex(R"(RWTexture2D|RWStructuredBuffer|RWByteAddressBuffer|RWBuffer)");
    info.uav_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), uav_regex),
        std::sregex_iterator()
    );
    
    // Count SRVs
    std::regex srv_regex(R"(Texture2D|StructuredBuffer|ByteAddressBuffer|Buffer)\s+\w+\s*:?\s*register\s*\(\s*t\d+\s*\)");
    info.srv_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), srv_regex),
        std::sregex_iterator()
    );
    
    // Count loops and branches
    std::regex loop_regex(R"(\b(for|while|do)\s*\()");
    info.loop_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), loop_regex),
        std::sregex_iterator()
    );
    
    std::regex branch_regex(R"(\b(if|else|switch)\s*\()");
    info.branch_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), branch_regex),
        std::sregex_iterator()
    );
    
    // Count texture loads
    std::regex tex_load_regex(R"(\.Sample\(|\.SampleLevel\(|\.SampleBias\(|\.SampleGrad\(|\.Load\()");
    info.texture_load_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), tex_load_regex),
        std::sregex_iterator()
    );
    
    // Count math instructions
    std::regex math_regex(R"(\b(mul|add|sub|mad|dp2|dp3|dp4|dot|cross|normalize|length|distance|reflect|refract|rsq|sqrt|rcp|log|exp|sin|cos|tan|asin|acos|atan|pow|fmod|floor|ceil|frac|abs|saturate|sign|min|max|clamp|step|smoothstep)\s*\()");
    info.math_instruction_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), math_regex),
        std::sregex_iterator()
    );
    
    // Count flow control
    std::regex flow_regex(R"(\b(if|else|switch|case|default|for|while|do|break|continue|return|discard|clip)\b)");
    info.flow_control_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), flow_regex),
        std::sregex_iterator()
    );
    
    // Count barriers
    std::regex barrier_regex(R"(GroupMemoryBarrier|DeviceMemoryBarrier|AllMemoryBarrier|GroupMemoryBarrierWithGroupSync|DeviceMemoryBarrierWithGroupSync)");
    info.barrier_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), barrier_regex),
        std::sregex_iterator()
    );
    
    // Estimate cycles (very rough)
    info.estimated_cycles = info.instruction_count * 2 + info.loop_count * 10 + info.branch_count * 3;
    
    detect_engine_hints(info, content);
    info.estimated_vram = estimate_vram_usage(info);
    
    return info;
}

Result<ShaderInfo> ShaderAnalyzer::analyze_glsl(const Path& path) {
    ShaderInfo info;
    info.language = "GLSL";
    String content = read_file_content(path);
    info.disk_size = content.size();
    
    // Detect version/profile
    std::regex version_regex(R"(#version\s+(\d+))");
    std::smatch match;
    if (std::regex_search(content, match, version_regex)) {
        info.profile = "GLSL " + match[1].str();
    }
    
    // Detect entry point (main function)
    std::regex main_regex(R"(void\s+main\s*\()");
    if (std::regex_search(content, match, main_regex)) {
        info.entry_point = "main";
        
        // Determine stage from qualifiers
        std::regex vert_regex(R"(layout\s*\(\s*location\s*=\s*\d+\s*\)\s*in|in\s+vec\d|in\s+mat\d)");
        std::regex frag_regex(R"(layout\s*\(\s*location\s*=\s*\d+\s*\)\s*out|out\s+vec\d)");
        std::regex comp_regex(R"(layout\s*\(\s*local_size)");
        
        if (std::regex_search(content, comp_regex)) {
            info.stage = "compute";
        } else if (std::regex_search(content, vert_regex) && !std::regex_search(content, frag_regex)) {
            info.stage = "vertex";
        } else {
            info.stage = "fragment";
        }
    }
    
    info.instruction_count = estimate_complexity(content, "GLSL");
    
    // Count uniforms
    std::regex uniform_regex(R"(uniform\s+\w+)");
    info.register_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), uniform_regex),
        std::sregex_iterator()
    );
    
    // Count samplers/textures
    std::regex sampler_regex(R"(uniform\s+sampler\w+)");
    info.texture_sampler_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), sampler_regex),
        std::sregex_iterator()
    );
    
    // Count UAVs (image load/store)
    std::regex uav_regex(R"(uniform\s+image\w+)");
    info.uav_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), uav_regex),
        std::sregex_iterator()
    );
    
    // Loops and branches
    std::regex loop_regex(R"(\b(for|while|do)\s*\()");
    info.loop_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), loop_regex),
        std::sregex_iterator()
    );
    
    std::regex branch_regex(R"(\b(if|else|switch)\s*\()");
    info.branch_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), branch_regex),
        std::sregex_iterator()
    );
    
    // Texture loads
    std::regex tex_load_regex(R"(texture\w*\(|imageLoad\(|imageStore\()");
    info.texture_load_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), tex_load_regex),
        std::sregex_iterator()
    );
    
    // Math instructions
    std::regex math_regex(R"(\b(dot|cross|normalize|length|distance|reflect|refract|sqrt|rsqrt|pow|exp|log|sin|cos|tan|asin|acos|atan|floor|ceil|fract|abs|sign|min|max|clamp|mix|step|smoothstep|fma|fmod|mod|radians|degrees|matrixCompMult|outerProduct|transpose|determinant|inverse)\s*\()");
    info.math_instruction_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), math_regex),
        std::sregex_iterator()
    );
    
    // Flow control
    std::regex flow_regex(R"(\b(if|else|switch|case|default|for|while|do|break|continue|return|discard)\b)");
    info.flow_control_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), flow_regex),
        std::sregex_iterator()
    );
    
    // Barriers
    std::regex barrier_regex(R"(memoryBarrier|barrier|groupMemoryBarrier)");
    info.barrier_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), barrier_regex),
        std::sregex_iterator()
    );
    
    info.estimated_cycles = info.instruction_count * 2 + info.loop_count * 10 + info.branch_count * 3;
    
    detect_engine_hints(info, content);
    info.estimated_vram = estimate_vram_usage(info);
    
    return info;
}

Result<ShaderInfo> ShaderAnalyzer::analyze_spirv(const Path& path) {
    ShaderInfo info;
    info.language = "SPIR-V";
    info.is_compiled = true;
    info.bytecode_format = "SPIR-V";
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Cannot open SPIR-V file"));
    }
    
    // Read SPIR-V header (5 words = 20 bytes)
    uint32_t magic, version, generator, bound, schema;
    file.read(reinterpret_cast<char*>(&magic), 4);
    file.read(reinterpret_cast<char*>(&version), 4);
    file.read(reinterpret_cast<char*>(&generator), 4);
    file.read(reinterpret_cast<char*>(&bound), 4);
    file.read(reinterpret_cast<char*>(&schema), 4);
    
    if (magic != 0x07230203) { // SPIR-V magic number
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Not a valid SPIR-V file"));
    }
    
    info.disk_size = fs::file_size(path);
    
    // Read all instructions
    std::vector<uint32_t> words;
    uint32_t word;
    while (file.read(reinterpret_cast<char*>(&word), 4)) {
        words.push_back(word);
    }
    
    // Parse instructions
    size_t i = 5; // Skip header
    while (i < words.size()) {
        uint32_t inst = words[i];
        uint16_t opcode = inst & 0xFFFF;
        uint16_t word_count = inst >> 16;
        
        info.instruction_count++;
        
        // Count specific instruction types
        switch (opcode) {
            case 64:  // OpLoopMerge
            case 65:  // OpSelectionMerge
            case 66:  // OpBranch
            case 67:  // OpBranchConditional
            case 68:  // OpSwitch
            case 69:  // OpKill
                info.flow_control_count++;
                break;
            case 87:  // OpImageSampleImplicitLod
            case 88:  // OpImageSampleExplicitLod
            case 89:  // OpImageSampleDrefImplicitLod
            case 90:  // OpImageSampleDrefExplicitLod
            case 91:  // OpImageSampleProjImplicitLod
            case 92:  // OpImageSampleProjExplicitLod
            case 93:  // OpImageSampleProjDrefImplicitLod
            case 94:  // OpImageSampleProjDrefExplicitLod
            case 95:  // OpImageFetch
            case 96:  // OpImageGather
            case 97:  // OpImageDrefGather
            case 98:  // OpImageRead
            case 99:  // OpImageWrite
                info.texture_load_count++;
                break;
            // Math/bitwise instructions (rough range)
            case 100 ... 255:
                info.math_instruction_count++;
                break;
            case 33:  // OpVariable - count registers
                info.register_count++;
                break;
            case 71:  // OpImageSampleDrefImplicitLod
            case 72:  // OpImageSampleDrefExplicitLod
                info.texture_sampler_count++;
                break;
            case 57:  // OpTypeImage
            case 58:  // OpTypeSampler
                info.texture_sampler_count++;
                break;
            case 60:  // OpTypeBuffer
            case 61:  // OpTypeRuntimeArray
                info.uav_count++;
                break;
        }
        
        i += word_count;
    }
    
    // Estimate stage from execution model (simplified)
    info.stage = "unknown";
    
    info.estimated_cycles = info.instruction_count * 1.5f + info.flow_control_count * 5;
    info.estimated_vram = estimate_vram_usage(info);
    
    return info;
}

Result<ShaderInfo> ShaderAnalyzer::analyze_msl(const Path& path) {
    ShaderInfo info;
    info.language = "Metal (MSL)";
    String content = read_file_content(path);
    info.disk_size = content.size();
    
    // Detect entry point with [[vertex]], [[fragment]], [[kernel]] attributes
    std::regex vertex_regex(R"(\[\[vertex\]\])");
    std::regex fragment_regex(R"(\[\[fragment\]\])");
    std::regex compute_regex(R"(\[\[kernel\]\])");
    
    if (std::regex_search(content, vertex_regex)) {
        info.stage = "vertex";
    } else if (std::regex_search(content, fragment_regex)) {
        info.stage = "fragment";
    } else if (std::regex_search(content, compute_regex)) {
        info.stage = "compute";
    }
    
    // Find function names
    std::regex func_regex(R"((vertex|fragment|kernel)\s+\w+\s*\()");
    std::smatch match;
    if (std::regex_search(content, match, func_regex)) {
        info.entry_point = match[0].str();
    }
    
    // Count registers (buffers, textures)
    std::regex buffer_regex(R"(buffer\s+\w+|constant\s+\w+|device\s+\w+.*&)");
    info.register_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), buffer_regex),
        std::sregex_iterator()
    );
    
    std::regex texture_regex(R"(texture\d*d\s*<\s*\w+\s*,\s*access::)");
    info.texture_sampler_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), texture_regex),
        std::sregex_iterator()
    );
    
    std::regex sampler_regex(R"(sampler\s+\w+)");
    info.texture_sampler_count += std::distance(
        std::sregex_iterator(content.begin(), content.end(), sampler_regex),
        std::sregex_iterator()
    );
    
    // Loops and branches
    std::regex loop_regex(R"(\b(for|while|do)\s*\()");
    info.loop_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), loop_regex),
        std::sregex_iterator()
    );
    
    std::regex branch_regex(R"(\b(if|else|switch)\s*\()");
    info.branch_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), branch_regex),
        std::sregex_iterator()
    );
    
    // Texture loads
    std::regex tex_load_regex(R"(\.sample\(|\.read\(|\.write\()");
    info.texture_load_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), tex_load_regex),
        std::sregex_iterator()
    );
    
    // Math
    std::regex math_regex(R"(metal::|simd::|dot|cross|normalize|length|distance|reflect|refract|sqrt|rsqrt|pow|exp|log|sin|cos|tan|asin|acos|atan|floor|ceil|fract|abs|sign|min|max|clamp|mix|step|smoothstep)");
    info.math_instruction_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), math_regex),
        std::sregex_iterator()
    );
    
    info.instruction_count = estimate_complexity(content, "MSL");
    info.estimated_cycles = info.instruction_count * 2 + info.loop_count * 10 + info.branch_count * 3;
    detect_engine_hints(info, content);
    info.estimated_vram = estimate_vram_usage(info);
    
    return info;
}

Result<ShaderInfo> ShaderAnalyzer::analyze_cg(const Path& path) {
    ShaderInfo info;
    info.language = "Cg";
    String content = read_file_content(path);
    info.disk_size = content.size();
    
    // Cg is similar to HLSL
    std::regex profile_regex(R"(#pragma\s+target\s+(\w+)|//\s*target\s+(\w+))");
    std::smatch match;
    if (std::regex_search(content, match, profile_regex)) {
        info.profile = match[1].matched ? match[1].str() : match[2].str();
    }
    
    std::regex entry_regex(R"(\b(vs|ps|cs|gs|hs|ds)\s+\w+\s*\()");
    if (std::regex_search(content, match, entry_regex)) {
        info.entry_point = match[0].str();
        info.stage = detect_stage_from_entry(info.entry_point);
    }
    
    info.instruction_count = estimate_complexity(content, "Cg");
    
    detect_engine_hints(info, content);
    info.estimated_vram = estimate_vram_usage(info);
    
    return info;
}

Result<ShaderInfo> ShaderAnalyzer::analyze_dxbc(const Path& path) {
    ShaderInfo info;
    info.language = "DXBC (DirectX Bytecode)";
    info.is_compiled = true;
    info.bytecode_format = "DXBC";
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Cannot open DXBC file"));
    }
    
    file.seekg(0, std::ios::end);
    info.disk_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read DXBC header (DXBC signature + 4 chunks)
    char signature[4];
    file.read(signature, 4);
    
    if (std::memcmp(signature, "DXBC", 4) != 0) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Not a valid DXBC file"));
    }
    
    // Parse chunks (simplified)
    uint32_t chunk_count;
    file.read(reinterpret_cast<char*>(&chunk_count), 4);
    
    for (uint32_t i = 0; i < chunk_count; ++i) {
        char chunk_sig[4];
        uint32_t chunk_size;
        file.read(chunk_sig, 4);
        file.read(reinterpret_cast<char*>(&chunk_size), 4);
        
        String chunk_name(chunk_sig, 4);
        
        if (chunk_name == "SHDR") {
            // Shader bytecode chunk
            info.instruction_count += chunk_size / 4; // Rough estimate
        } else if (chunk_name == "RDEF") {
            // Resource definition
            info.register_count += chunk_size / 32; // Rough estimate
        }
        
        // Skip chunk data
        file.seekg(chunk_size, std::ios::cur);
    }
    
    info.estimated_cycles = info.instruction_count * 2;
    info.estimated_vram = estimate_vram_usage(info);
    
    return info;
}

Result<ShaderInfo> ShaderAnalyzer::analyze_generic(const Path& path, FileType type) {
    ShaderInfo info;
    info.disk_size = fs::file_size(path);
    info.language = "Unknown";
    
    String content = read_file_content(path);
    if (!content.empty() && content.size() < 1024 * 1024) {
        info.instruction_count = estimate_complexity(content, "Unknown");
    } else {
        info.instruction_count = 100; // Default
    }
    
    info.estimated_vram = estimate_vram_usage(info);
    
    return info;
}

u64 ShaderAnalyzer::estimate_complexity(const String& content, StringView lang) const {
    u64 complexity = 0;
    
    complexity += content.size() / 10;
    
    std::unordered_set<String> control_keywords = {
        "if", "else", "for", "while", "do", "switch", "case", "try", "catch",
        "foreach", "repeat", "until", "loop", "break", "continue"
    };
    
    for (const auto& kw : control_keywords) {
        size_t pos = 0;
        while ((pos = content.find(kw, pos)) != String::npos) {
            if ((pos == 0 || !std::isalnum(content[pos - 1])) && 
                (pos + kw.size() >= content.size() || !std::isalnum(content[pos + kw.size()]))) {
                complexity += 5;
            }
            pos += kw.size();
        }
    }
    
    complexity += std::count(content.begin(), content.end(), '(') * 2;
    complexity += std::count(content.begin(), content.end(), '[');
    complexity += std::count(content.begin(), content.end(), '{') * 3;
    
    if (lang == "C#" || lang == "TypeScript" || lang == "HLSL" || lang == "GLSL" || lang == "Cg" || lang == "MSL") {
        complexity = static_cast<u64>(complexity * 1.5);
    }
    
    return complexity;
}

u64 ShaderAnalyzer::estimate_vram_usage(const ShaderInfo& shader) const {
    // VRAM usage for shaders is primarily:
    // - Shader bytecode storage
    // - Constant buffers
    // - Resource bindings (textures, buffers, samplers)
    // - Pipeline state objects
    
    u64 vram = shader.instruction_count * 4; // ~4 bytes per instruction in bytecode
    vram += shader.constant_buffer_count * 16 * 1024; // 16KB per cbuffer
    vram += (shader.texture_sampler_count + shader.srv_count + shader.uav_count) * 4 * 1024; // ~4KB per resource
    vram += shader.register_count * 16; // Register allocation
    
    // Pipeline state
    vram += 64 * 1024; // Base PSO size
    
    return vram;
}

void ShaderAnalyzer::detect_engine_hints(ShaderInfo& shader, StringView content) const {
    if (content.find("Unity") != String::npos || content.find("UNITY_") != String::npos) {
        shader.engine_hint = "Unity";
    } else if (content.find("Unreal") != String::npos || content.find("UE4") != String::npos || content.find("UE5") != String::npos) {
        shader.engine_hint = "Unreal Engine";
    } else if (content.find("Godot") != String::npos) {
        shader.engine_hint = "Godot";
    } else if (content.find("CryEngine") != String::npos) {
        shader.engine_hint = "CryEngine";
    } else if (content.find("Frostbite") != String::npos) {
        shader.engine_hint = "Frostbite";
    }
}

String ShaderAnalyzer::detect_stage_from_entry(const String& entry_point) const {
    if (entry_point.find("vs") == 0) return "vertex";
    if (entry_point.find("ps") == 0) return "pixel";
    if (entry_point.find("cs") == 0) return "compute";
    if (entry_point.find("gs") == 0) return "geometry";
    if (entry_point.find("hs") == 0) return "hull";
    if (entry_point.find("ds") == 0) return "domain";
    if (entry_point.find("ms") == 0) return "mesh";
    if (entry_point.find("as") == 0) return "amplification";
    return "unknown";
}

String ShaderAnalyzer::generate_report() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    std::stringstream ss;
    ss << "Shader Analysis Report\n";
    ss << "======================\n";
    ss << "Total Shaders: " << stats_.total_shaders << "\n";
    ss << "Total Instructions: " << StringUtils::format_number(stats_.total_instructions) << "\n";
    ss << "Total Disk Size: " << StringUtils::format_bytes(stats_.total_disk_size) << "\n";
    ss << "Estimated VRAM: " << StringUtils::format_bytes(stats_.total_estimated_vram) << "\n";
    ss << "Max Instructions/Shader: " << stats_.max_instructions_per_shader << "\n";
    ss << "Max Registers/Shader: " << stats_.max_registers_per_shader << "\n\n";
    
    if (!stats_.language_counts.empty()) {
        ss << "Language Distribution:\n";
        for (const auto& [lang, count] : stats_.language_counts) {
            ss << "  " << lang << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.stage_counts.empty()) {
        ss << "Stage Distribution:\n";
        for (const auto& [stage, count] : stats_.stage_counts) {
            ss << "  " << stage << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.profile_counts.empty()) {
        ss << "Profile Distribution:\n";
        for (const auto& [profile, count] : stats_.profile_counts) {
            ss << "  " << profile << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.engine_counts.empty()) {
        ss << "Engine Detection:\n";
        for (const auto& [engine, count] : stats_.engine_counts) {
            ss << "  " << engine << ": " << count << "\n";
        }
    }
    
    return ss.str();
}

String ShaderStats::summary() const {
    std::stringstream ss;
    ss << "Shaders: " << total_shaders 
       << ", Instructions: " << StringUtils::format_number(total_instructions)
       << ", Disk: " << StringUtils::format_bytes(total_disk_size)
       << ", VRAM: " << StringUtils::format_bytes(total_estimated_vram);
    return ss.str();
}

} // namespace game_req