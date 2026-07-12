#include <game_req_analyzer/analyzers/script_analyzer.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <fstream>
#include <regex>
#include <algorithm>
#include <unordered_set>

using namespace game_req;

ScriptAnalyzer::ScriptAnalyzer(const AnalyzerConfig& config) : config_(config) {}

Result<std::vector<ScriptInfo>> ScriptAnalyzer::analyze(const std::vector<FileInfo>& scripts) {
    std::vector<ScriptInfo> results;
    results.reserve(scripts.size());
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_scripts = 0;
        stats_.total_lines = 0;
        stats_.total_functions = 0;
        stats_.total_classes = 0;
        stats_.total_complexity = 0;
        stats_.total_ram_estimate = 0;
        stats_.language_counts.clear();
        stats_.engine_counts.clear();
        stats_.framework_counts.clear();
    }
    
    for (const auto& script : scripts) {
        auto result = analyze_single(script);
        if (result) {
            results.push_back(*result);
            
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_scripts++;
            stats_.total_lines += result->line_count;
            stats_.total_functions += result->function_count;
            stats_.total_classes += result->class_count;
            stats_.total_complexity += result->estimated_complexity;
            stats_.total_ram_estimate += estimate_ram(*result);
            stats_.language_counts[result->language]++;
            if (!result->engine_hint.empty()) stats_.engine_counts[result->engine_hint]++;
            if (!result->framework_hint.empty()) stats_.framework_counts[result->framework_hint]++;
        } else {
            LOG_WARN("Failed to analyze script {}: {}", script.path.string(), result.error().message);
        }
    }
    
    return results;
}

Result<ScriptInfo> ScriptAnalyzer::analyze_single(const FileInfo& script) {
    ScriptInfo info;
    info.disk_size = script.size;
    
    try {
        switch (script.type) {
            case FileType::LUA:
                return analyze_lua(script.path);
            case FileType::PY:
                return analyze_python(script.path);
            case FileType::CS:
                return analyze_csharp(script.path);
            case FileType::JS:
                return analyze_js(script.path);
            case FileType::TS:
                return analyze_ts(script.path);
            case FileType::HLSL:
            case FileType::GLSL:
            case FileType::SPIRV:
            case FileType::CG:
            case FileType::METAL:
            case FileType::MSL:
            case FileType::SHADER:
            case FileType::VSH:
            case FileType::PSH:
            case FileType::GSH:
            case FileType::USH:
            case FileType::USF:
            case FileType::FX:
            case FileType::CGINC:
                return analyze_shader(script.path, script.type);
            case FileType::ANGELSCRIPT:
                return analyze_angel_script(script.path);
            case FileType::SQF:
                return analyze_sqf(script.path);
            case FileType::PAWN:
                return analyze_pawn(script.path);
            default:
                return analyze_generic(script.path, script.type);
        }
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Script analysis failed: {}", e.what())));
    }
}

String ScriptAnalyzer::read_file_content(const Path& path) {
    std::ifstream file(path);
    if (!file) return "";
    std::string content;
    file.seekg(0, std::ios::end);
    content.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return content;
}

Result<ScriptInfo> ScriptAnalyzer::analyze_lua(const Path& path) {
    ScriptInfo info;
    info.language = "Lua";
    String content = read_file_content(path);
    info.char_count = content.size();
    
    info.line_count = std::count(content.begin(), content.end(), '\n') + 1;
    
    std::regex func_regex(R"(function\s+\w+\s*\(|local\s+function\s+\w+\s*\(|\w+\s*=\s*function\s*\()");
    info.function_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), func_regex),
        std::sregex_iterator()
    );
    
    std::regex class_regex(R"(class\s+\w+|\w+\s*=\s*\{\s*__index\s*=)");
    info.class_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), class_regex),
        std::sregex_iterator()
    );
    
    info.estimated_complexity = estimate_complexity(content, "Lua");
    detect_api_calls(info, content);
    
    if (content.find("love.") != String::npos) {
        info.engine_hint = "LÖVE";
        info.framework_hint = "LÖVE 2D";
    } else if (content.find("Corona") != String::npos || content.find("solar2d") != String::npos) {
        info.engine_hint = "Solar2D (Corona)";
    } else if (content.find("defold") != String::npos || content.find("dmScript") != String::npos) {
        info.engine_hint = "Defold";
    } else if (content.find("Factorio") != String::npos || content.find("script.on_event") != String::npos) {
        info.engine_hint = "Factorio";
    } else if (content.find("GMod") != String::npos || content.find("gmod") != String::npos) {
        info.engine_hint = "Garry's Mod";
    } else if (content.find("Roblox") != String::npos || content.find("game:GetService") != String::npos) {
        info.engine_hint = "Roblox";
    } else if (content.find("redis") != String::npos || content.find("ngx.") != String::npos) {
        info.engine_hint = "OpenResty/NGINX";
    }
    
    info.estimated_ram = estimate_ram(info);
    return info;
}

Result<ScriptInfo> ScriptAnalyzer::analyze_python(const Path& path) {
    ScriptInfo info;
    info.language = "Python";
    String content = read_file_content(path);
    info.char_count = content.size();
    info.line_count = std::count(content.begin(), content.end(), '\n') + 1;
    
    std::regex func_regex(R"(^\s*def\s+\w+\s*\()", std::regex_constants::multiline);
    info.function_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), func_regex),
        std::sregex_iterator()
    );
    
    std::regex class_regex(R"(^\s*class\s+\w+)", std::regex_constants::multiline);
    info.class_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), class_regex),
        std::sregex_iterator()
    );
    
    info.estimated_complexity = estimate_complexity(content, "Python");
    detect_api_calls(info, content);
    
    if (content.find("import bpy") != String::npos) {
        info.engine_hint = "Blender";
    } else if (content.find("import unreal") != String::npos || content.find("from unreal") != String::npos) {
        info.engine_hint = "Unreal Engine";
    } else if (content.find("import unity") != String::npos || content.find("from unity") != String::npos) {
        info.engine_hint = "Unity (Python for Unity)";
    } else if (content.find("import godot") != String::npos || content.find("from godot") != String::npos) {
        info.engine_hint = "Godot";
    } else if (content.find("import pygame") != String::npos) {
        info.engine_hint = "Pygame";
    } else if (content.find("import panda3d") != String::npos || content.find("from panda3d") != String::npos) {
        info.engine_hint = "Panda3D";
    } else if (content.find("import arcade") != String::npos) {
        info.engine_hint = "Arcade";
    } else if (content.find("import ursina") != String::npos) {
        info.engine_hint = "Ursina";
    }
    
    info.estimated_ram = estimate_ram(info);
    return info;
}

Result<ScriptInfo> ScriptAnalyzer::analyze_csharp(const Path& path) {
    ScriptInfo info;
    info.language = "C#";
    String content = read_file_content(path);
    info.char_count = content.size();
    info.line_count = std::count(content.begin(), content.end(), '\n') + 1;
    
    std::regex func_regex(R"((public|private|protected|internal)?\s*(static\s+)?\w+\s+\w+\s*\([^)]*\)\s*\{)");
    info.function_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), func_regex),
        std::sregex_iterator()
    );
    
    std::regex class_regex(R"((public|private|protected|internal)?\s*(static\s+)?(class|struct|interface)\s+\w+)");
    info.class_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), class_regex),
        std::sregex_iterator()
    );
    
    info.estimated_complexity = estimate_complexity(content, "C#");
    detect_api_calls(info, content);
    
    if (content.find("UnityEngine") != String::npos || content.find("MonoBehaviour") != String::npos) {
        info.engine_hint = "Unity";
        info.framework_hint = "Unity";
    } else if (content.find("Godot") != String::npos || content.find("GodotSharp") != String::npos) {
        info.engine_hint = "Godot";
        info.framework_hint = "Godot C#";
    } else if (content.find("Microsoft.Xna") != String::npos || content.find("MonoGame") != String::npos) {
        info.engine_hint = "MonoGame";
    } else if (content.find("Stride") != String::npos || content.find("Xenko") != String::npos) {
        info.engine_hint = "Stride";
    } else if (content.find("CryEngine") != String::npos || content.find("CrySystem") != String::npos) {
        info.engine_hint = "CryEngine";
    }
    
    info.estimated_ram = estimate_ram(info);
    return info;
}

Result<ScriptInfo> ScriptAnalyzer::analyze_js(const Path& path) {
    ScriptInfo info;
    info.language = "JavaScript";
    String content = read_file_content(path);
    info.char_count = content.size();
    info.line_count = std::count(content.begin(), content.end(), '\n') + 1;
    
    std::regex func_regex(R"((function\s+\w+\s*\(|const\s+\w+\s*=\s*(\([^)]*\)|[^=]+)\s*=>|let\s+\w+\s*=\s*function\s*\()");
    info.function_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), func_regex),
        std::sregex_iterator()
    );
    
    std::regex class_regex(R"(class\s+\w+)");
    info.class_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), class_regex),
        std::sregex_iterator()
    );
    
    info.estimated_complexity = estimate_complexity(content, "JavaScript");
    detect_api_calls(info, content);
    
    if (content.find("three.") != String::npos || content.find("THREE.") != String::npos) {
        info.engine_hint = "Three.js";
        info.framework_hint = "Three.js";
    } else if (content.find("babylon") != String::npos || content.find("BABYLON") != String::npos) {
        info.engine_hint = "Babylon.js";
    } else if (content.find("phaser") != String::npos || content.find("Phaser") != String::npos) {
        info.engine_hint = "Phaser";
    } else if (content.find("pixi") != String::npos || content.find("PIXI") != String::npos) {
        info.engine_hint = "PixiJS";
    } else if (content.find("playcanvas") != String::npos) {
        info.engine_hint = "PlayCanvas";
    } else if (content.find("cocos") != String::npos || content.find("cc.") != String::npos) {
        info.engine_hint = "Cocos Creator";
    } else if (content.find("godot") != String::npos && content.find("JavaScript") != String::npos) {
        info.engine_hint = "Godot (JavaScript)";
    }
    
    info.estimated_ram = estimate_ram(info);
    return info;
}

Result<ScriptInfo> ScriptAnalyzer::analyze_ts(const Path& path) {
    ScriptInfo info;
    info.language = "TypeScript";
    String content = read_file_content(path);
    info.char_count = content.size();
    info.line_count = std::count(content.begin(), content.end(), '\n') + 1;
    
    std::regex func_regex(R"((function\s+\w+\s*\(|const\s+\w+\s*:\s*\([^)]*\)\s*=>|function\s*\([^)]*\)\s*:\s*\w+\s*\{)");
    info.function_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), func_regex),
        std::sregex_iterator()
    );
    
    std::regex class_regex(R"(class\s+\w+)");
    info.class_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), class_regex),
        std::sregex_iterator()
    );
    
    info.estimated_complexity = estimate_complexity(content, "TypeScript");
    detect_api_calls(info, content);
    
    if (content.find("three.") != String::npos || content.find("THREE.") != String::npos) {
        info.engine_hint = "Three.js";
    } else if (content.find("babylon") != String::npos || content.find("BABYLON") != String::npos) {
        info.engine_hint = "Babylon.js";
    } else if (content.find("phaser") != String::npos) {
        info.engine_hint = "Phaser";
    }
    
    info.estimated_ram = estimate_ram(info);
    return info;
}

Result<ScriptInfo> ScriptAnalyzer::analyze_shader(const Path& path, FileType type) {
    ScriptInfo info;
    info.is_compiled = (type == FileType::SPIRV || type == FileType::FXO);
    info.bytecode_format = (type == FileType::SPIRV) ? "SPIR-V" : 
                          (type == FileType::FXO) ? "FXO" : "Unknown";
    
    String content = read_file_content(path);
    info.char_count = content.size();
    info.line_count = std::count(content.begin(), content.end(), '\n') + 1;
    
    switch (type) {
        case FileType::HLSL: info.language = "HLSL"; break;
        case FileType::GLSL: info.language = "GLSL"; break;
        case FileType::SPIRV: info.language = "SPIR-V"; break;
        case FileType::CG: info.language = "Cg"; break;
        case FileType::METAL: case FileType::MSL: info.language = "Metal"; break;
        default: info.language = "Shader";
    }
    
    std::regex func_regex(R"(\w+\s+\w+\s*\([^)]*\)\s*\{)");
    info.function_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), func_regex),
        std::sregex_iterator()
    );
    
    if (content.find("Unity") != String::npos || content.find("UNITY_") != String::npos) {
        info.engine_hint = "Unity";
    } else if (content.find("Unreal") != String::npos || content.find("UE4") != String::npos || content.find("UE5") != String::npos) {
        info.engine_hint = "Unreal Engine";
    } else if (content.find("Godot") != String::npos) {
        info.engine_hint = "Godot";
    }
    
    info.estimated_complexity = estimate_complexity(content, info.language);
    info.estimated_ram = estimate_ram(info);
    return info;
}

Result<ScriptInfo> ScriptAnalyzer::analyze_angel_script(const Path& path) {
    ScriptInfo info;
    info.language = "AngelScript";
    String content = read_file_content(path);
    info.char_count = content.size();
    info.line_count = std::count(content.begin(), content.end(), '\n') + 1;
    
    std::regex func_regex(R"(\w+\s+\w+\s*\([^)]*\)\s*\{)");
    info.function_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), func_regex),
        std::sregex_iterator()
    );
    
    std::regex class_regex(R"(class\s+\w+)");
    info.class_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), class_regex),
        std::sregex_iterator()
    );
    
    info.engine_hint = "AngelScript (HPL, SOMA, Amnesia)";
    info.estimated_complexity = estimate_complexity(content, "AngelScript");
    info.estimated_ram = estimate_ram(info);
    return info;
}

Result<ScriptInfo> ScriptAnalyzer::analyze_sqf(const Path& path) {
    ScriptInfo info;
    info.language = "SQF";
    String content = read_file_content(path);
    info.char_count = content.size();
    info.line_count = std::count(content.begin(), content.end(), '\n') + 1;
    
    std::regex func_regex(R"(\w+\s*=\s*\{)");
    info.function_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), func_regex),
        std::sregex_iterator()
    );
    
    info.engine_hint = "Arma (Real Virtuality)";
    info.estimated_complexity = estimate_complexity(content, "SQF");
    info.estimated_ram = estimate_ram(info);
    return info;
}

Result<ScriptInfo> ScriptAnalyzer::analyze_pawn(const Path& path) {
    ScriptInfo info;
    info.language = "Pawn";
    String content = read_file_content(path);
    info.char_count = content.size();
    info.line_count = std::count(content.begin(), content.end(), '\n') + 1;
    
    std::regex func_regex(R"(forward\s+\w+;|public\s+\w+\s*\(|native\s+\w+\s*\()");
    info.function_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), func_regex),
        std::sregex_iterator()
    );
    
    info.engine_hint = "SA-MP / AMX Mod X / Pawn";
    info.estimated_complexity = estimate_complexity(content, "Pawn");
    info.estimated_ram = estimate_ram(info);
    return info;
}

Result<ScriptInfo> ScriptAnalyzer::analyze_generic(const Path& path, FileType type) {
    ScriptInfo info;
    info.disk_size = fs::file_size(path);
    info.language = StringUtils::to_upper(path.extension().string().substr(1));
    if (info.language.empty()) info.language = "Unknown";
    
    String content = read_file_content(path);
    info.char_count = content.size();
    info.line_count = std::count(content.begin(), content.end(), '\n') + 1;
    
    std::regex func_regex(R"(\w+\s+\w+\s*\([^)]*\)\s*\{|function\s+\w+\s*\(|def\s+\w+\s*\()");
    info.function_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), func_regex),
        std::sregex_iterator()
    );
    
    std::regex class_regex(R"(class\s+\w+)");
    info.class_count = std::distance(
        std::sregex_iterator(content.begin(), content.end(), class_regex),
        std::sregex_iterator()
    );
    
    info.estimated_complexity = estimate_complexity(content, info.language);
    info.estimated_ram = estimate_ram(info);
    return info;
}

u64 ScriptAnalyzer::estimate_complexity(const String& content, StringView lang) const {
    u64 complexity = 0;
    
    complexity += content.size() / 10;
    
    std::unordered_set<String> control_keywords = {
        "if", "else", "for", "while", "do", "switch", "case", "try", "catch",
        "foreach", "foreach", "repeat", "until", "loop", "break", "continue"
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
    
    if (lang == "C#" || lang == "TypeScript" || lang == "HLSL" || lang == "GLSL") {
        complexity *= 1.5;
    }
    
    return complexity;
}

u64 ScriptAnalyzer::estimate_ram(const ScriptInfo& script) const {
    u64 base = script.char_count * 2;
    base += script.function_count * 1024;
    base += script.class_count * 2048;
    base += script.estimated_complexity * 16;
    
    if (script.is_compiled) {
        base = script.disk_size * 2;
    }
    
    return std::max<u64>(base, 64 * KiB);
}

void ScriptAnalyzer::detect_api_calls(ScriptInfo& script, StringView content) const {
    static const std::vector<std::pair<StringView, StringView>> api_patterns = {
        {"DirectX", "Direct3D|DirectX|ID3D|D3D"},
        {"OpenGL", "glDraw|glBind|glGen|glCreate|glUse|glUniform"},
        {"Vulkan", "vkCmd|vkCreate|vkAllocate|vkBind|vkDraw"},
        {"Metal", "MTLRender|MTLDevice|MTLCommand|MTLBuffer"},
        {"PhysX", "PxPhysics|PxScene|PxRigid|PxShape|PxMaterial"},
        {"Havok", "hkpWorld|hkpRigidBody|hkpShape|hkpConstraint"},
        {"Bullet", "btDynamicsWorld|btRigidBody|btCollisionShape|btConstraint"},
        {"FMOD", "FMOD::|FMOD_|System::createSound|Channel::play"},
        {"Wwise", "AK::|AkSoundEngine|AkGameObject|AkEvent"},
        {"OpenAL", "alGen|alSource|alBuffer|alListener"},
        {"XAudio2", "IXAudio2|XAudio2|CreateSourceVoice|SubmitSourceBuffer"},
        {"Unity", "UnityEngine|MonoBehaviour|GameObject|Transform|Rigidbody"},
        {"Unreal", "AActor|UObject|UWorld|UPrimitiveComponent|TArray|UPROPERTY"},
        {"Godot", "Node|Node2D|Node3D|Resource|GDScript|_ready|_process"},
        {"CryEngine", "IEntity|IEntitySystem|I3DEngine|IRenderer|CryAction"},
    };
    
    for (const auto& [api, pattern] : api_patterns) {
        std::regex re(pattern.data());
        if (std::regex_search(content.begin(), content.end(), re)) {
            script.api_calls.push_back(String(api));
        }
    }
}

String ScriptAnalyzer::generate_report() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    std::stringstream ss;
    ss << "Script Analysis Report\n";
    ss << "=====================\n";
    ss << "Total Scripts: " << stats_.total_scripts << "\n";
    ss << "Total Lines: " << StringUtils::format_number(stats_.total_lines) << "\n";
    ss << "Total Functions: " << StringUtils::format_number(stats_.total_functions) << "\n";
    ss << "Total Classes: " << StringUtils::format_number(stats_.total_classes) << "\n";
    ss << "Total Complexity: " << StringUtils::format_number(stats_.total_complexity) << "\n";
    ss << "Estimated RAM: " << StringUtils::format_bytes(stats_.total_ram_estimate) << "\n\n";
    
    if (!stats_.language_counts.empty()) {
        ss << "Language Distribution:\n";
        for (const auto& [lang, count] : stats_.language_counts) {
            ss << "  " << lang << ": " << count << "\n";
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
    
    if (!stats_.framework_counts.empty()) {
        ss << "Framework Distribution:\n";
        for (const auto& [fw, count] : stats_.framework_counts) {
            ss << "  " << fw << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    return ss.str();
}
