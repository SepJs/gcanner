#include <game_req_analyzer/ui/terminal_ui.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/analyzers/asset_aggregator.hpp>
#include <game_req_analyzer/analyzers/requirement_calculator.hpp>
#include <game_req_analyzer/hardware/hardware_database.hpp>
#include <fmt/format.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>

using namespace game_req;

ResultsFormatter::ResultsFormatter(const OutputConfig& config) : config_(config) {}

String ResultsFormatter::format_scan_result(const ScanResult& result) const {
    std::ostringstream oss;
    oss << "Scan Results\n";
    oss << "============\n";
    oss << "Files scanned: " << result.file_count() << "\n";
    oss << "Total size: " << format_bytes(result.total_size()) << "\n";
    oss << "Duration: " << format_duration(result.duration()) << "\n";
    if (result.duration().count() > 0) {
        double mb_per_sec = static_cast<double>(result.total_size()) / 1024 / 1024 / 
                            std::max(result.duration().count(), 1ms);
        oss << "Throughput: " << std::fixed << std::setprecision(2) << mb_per_sec << " MB/s\n";
    }
    return oss.str();
}

String ResultsFormatter::format_assets(const AggregatedAssets& assets) const {
    std::ostringstream oss;
    oss << "Asset Analysis Results\n";
    oss << "=====================\n";
    oss << "Textures: " << assets.textures.total_textures << " (" 
        << format_bytes(assets.textures.total_vram) << " VRAM)\n";
    oss << "Models: " << assets.models.total_models << " (" 
        << format_number(assets.models.total_vertices) << " vertices, "
        << format_number(assets.models.total_triangles) << " triangles)\n";
    oss << "Audio: " << assets.audio.total_files << " files ("
        << format_bytes(assets.audio.total_size) << ")\n";
    oss << "Scripts: " << assets.scripts.total_files << " files ("
        << format_number(assets.scripts.total_lines) << " lines)\n";
    oss << "Executables: " << assets.executables.total_files << " files\n";
    oss << "\nEstimated Workload:\n";
    oss << "  CPU work: " << format_number(assets.total_cpu_work) << " points\n";
    oss << "  GPU work: " << format_number(assets.total_gpu_work) << " points\n";
    oss << "\nRatios:\n";
    oss << "  Texture to Model: " << std::fixed << std::setprecision(2) 
        << assets.texture_to_model_ratio << "\n";
    oss << "  Audio to Total: " << std::fixed << std::setprecision(2) 
        << assets.audio_to_total_ratio << "\n";
    oss << "  Script to Total: " << std::fixed << std::setprecision(2) 
        << assets.script_to_total_ratio << "\n";
    oss << "  Executable to Total: " << std::fixed << std::setprecision(2) 
        << assets.executable_to_total_ratio << "\n";
    oss << "\nEngine Detection:\n";
    oss << "  Primary Engine: " << (assets.primary_engine.empty() ? "Unknown" : assets.primary_engine) << "\n";
    oss << "  Primary Renderer: " << (assets.primary_renderer.empty() ? "Unknown" : assets.primary_renderer) << "\n";
    oss << "  Primary Audio: " << (assets.primary_audio.empty() ? "Unknown" : assets.primary_audio) << "\n";
    oss << "  Primary Physics: " << (assets.primary_physics.empty() ? "Unknown" : assets.primary_physics) << "\n";
    oss << "  Primary Scripting: " << (assets.primary_scripting.empty() ? "Unknown" : assets.primary_scripting) << "\n";
    return oss.str();
}

String ResultsFormatter::format_requirements(const RequirementResult& req) const {
    std::ostringstream oss;
    oss << "System Requirements Analysis\n";
    oss << "============================\n\n";

    // Minimum requirements
    oss << "MINIMUM REQUIREMENTS:\n";
    oss << "  OS: " << req.minimum.os << "\n";
    oss << "  CPU: " << req.minimum.cpu.name << " @ " 
        << std::fixed << std::setprecision(1) << req.minimum.cpu.base_clock 
        << " GHz (" << req.minimum.cpu.cores << " cores/" 
        << req.minimum.cpu.threads << " threads)\n";
    oss << "  GPU: " << req.minimum.gpu.name << " ("
        << req.minimum.gpu.vram / 1024 << " GB VRAM)\n";
    oss << "  RAM: " << req.minimum.ram << " GB\n";
    oss << "  Storage: " << req.minimum.storage.type << " " 
        << req.minimum.storage.name << " (" 
        << req.minimum.storage.capacity << " GB available)\n";
    oss << "  DirectX: " << req.minimum.directx << "\n";
    oss << "\n";

    // Recommended requirements
    oss << "RECOMMENDED REQUIREMENTS:\n";
    oss << "  OS: " << req.recommended.os << "\n";
    oss << "  CPU: " << req.recommended.cpu.name << " @ " 
        << std::fixed << std::setprecision(1) << req.recommended.cpu.base_clock 
        << " GHz (" << req.recommended.cpu.cores << " cores/" 
        << req.recommended.cpu.threads << " threads)\n";
    oss << "  GPU: " << req.recommended.gpu.name << " ("
        << req.recommended.gpu.vram / 1024 << " GB VRAM)\n";
    oss << "  RAM: " << req.recommended.ram << " GB\n";
    oss << "  Storage: " << req.recommended.storage.type << " " 
        << req.recommended.storage.name << " (" 
        << req.recommended.storage.capacity << " GB available)\n";
    oss << "  DirectX: " << req.recommended.directx << "\n";
    oss << "\n";

    // High requirements (if available)
    if (req.high.cpu.name != "") {
        oss << "HIGH REQUIREMENTS:\n";
        oss << "  CPU: " << req.high.cpu.name << " @ " 
            << std::fixed << std::setprecision(1) << req.high.cpu.base_clock 
            << " GHz (" << req.high.cpu.cores << " cores/" 
            << req.high.cpu.threads << " threads)\n";
        oss << "  GPU: " << req.high.gpu.name << " ("
            << req.high.gpu.vram / 1024 << " GB VRAM)\n";
        oss << "  RAM: " << req.high.ram << " GB\n";
        oss << "  Storage: " << req.high.storage.type << " " 
            << req.high.storage.name << " (" 
            << req.high.storage.capacity << " GB available)\n";
        oss << "  DirectX: " << req.high.directx << "\n";
        oss << "\n";
    }

    // Ultra requirements (if available)
    if (req.ultra.cpu.name != "") {
        oss << "ULTRA REQUIREMENTS:\n";
        oss << "  CPU: " << req.ultra.cpu.name << " @ " 
            << std::fixed << std::setprecision(1) << req.ultra.cpu.base_clock 
            << " GHz (" << req.ultra.cpu.cores << " cores/" 
            << req.ultra.cpu.threads << " threads)\n";
        oss << "  GPU: " << req.ultra.gpu.name << " ("
            << req.ultra.gpu.vram / 1024 << " GB VRAM)\n";
        oss << "  RAM: " << req.ultra.ram << " GB\n";
        oss << "  Storage: " << req.ultra.storage.type << " " 
            << req.ultra.storage.name << " (" 
            << req.ultra.storage.capacity << " GB available)\n";
        oss << "  DirectX: " << req.ultra.directx << "\n";
        oss << "\n";
    }

    // Ray tracing requirements (if available)
    if (req.ray_tracing.has_value()) {
        oss << "RAY TRACING REQUIREMENTS:\n";
        oss << "  CPU: " << req.ray_tracing->cpu.name << " @ " 
            << std::fixed << std::setprecision(1) << req.ray_tracing->cpu.base_clock 
            << " GHz (" << req.ray_tracing->cpu.cores << " cores/" 
            << req.ray_tracing->cpu.threads << " threads)\n";
        oss << "  GPU: " << req.ray_tracing->gpu.name << " ("
            << req.ray_tracing->gpu.vram / 1024 << " GB VRAM)\n";
        oss << "  RAM: " << req.ray_tracing->ram << " GB\n";
        oss << "  Storage: " << req.ray_tracing->storage.type << " " 
            << req.ray_tracing->storage.name << " (" 
            << req.ray_tracing->storage.capacity << " GB available)\n";
        oss << "  DirectX: " << req.ray_tracing->directx << "\n";
        oss << "\n";
    }

    // Confidence and notes
    oss << "Analysis Confidence: " << std::fixed << std::setprecision(1) 
        << (req.confidence * 100.0) << "%\n";
    if (!req.analysis_notes.empty()) {
        oss << "Analysis Notes: " << req.analysis_notes << "\n";
    }
    if (!req.warnings.empty()) {
        oss << "Warnings:\n";
        for (const auto& warning : req.warnings) {
            oss << "  - " << warning << "\n";
        }
    }
    if (!req.assumptions.empty()) {
        oss << "Assumptions:\n";
        for (const auto& assumption : req.assumptions) {
            oss << "  - " << assumption << "\n";
        }
    }

    return oss.str();
}

String ResultsFormatter::format_json(const RequirementResult& req) const {
    json j;
    j["minimum"] = req.minimum.to_json();
    j["recommended"] = req.recommended.to_json();
    j["high"] = req.high.to_json();
    j["ultra"] = req.ultra.to_json();
    if (req.ray_tracing.has_value()) {
        j["ray_tracing"] = req.ray_tracing->to_json();
    } else {
        j["ray_tracing"] = nullptr;
    }
    j["confidence"] = req.confidence;
    j["analysis_notes"] = req.analysis_notes;
    j["warnings"] = req.warnings;
    j["assumptions"] = req.assumptions;
    
    return j.dump(2);
}

String ResultsFormatter::format_csv(const RequirementResult& req) const {
    std::ostringstream oss;
    // CSV format: Level, Component, Spec, Value
    oss << "Level,Component,Spec,Value\n";
    
    auto add_row = [&](const std::string& level, const std::string& component, 
                      const std::string& spec, const std::string& value) {
        oss << level << "," << component << "," << spec << "," << value << "\n";
    };
    
    // Minimum
    add_row("Minimum", "OS", "Version", req.minimum.os);
    add_row("Minimum", "CPU", "Name", req.minimum.cpu.name);
    add_row("Minimum", "CPU", "Base Clock (GHz)", std::to_string(req.minimum.cpu.base_clock));
    add_row("Minimum", "CPU", "Boost Clock (GHz)", std::to_string(req.minimum.cpu.boost_clock));
    add_row("Minimum", "CPU", "Cores", std::to_string(req.minimum.cpu.cores));
    add_row("Minimum", "CPU", "Threads", std::to_string(req.minimum.cpu.threads));
    add_row("Minimum", "CPU", "L3 Cache (KB)", std::to_string(req.minimum.cpu.l3_cache));
    add_row("Minimum", "CPU", "Instruction Set", req.minimum.cpu.instruction_set);
    add_row("Minimum", "CPU", "Passmark Score", std::to_string(req.minimum.cpu.passmark_score));
    add_row("Minimum", "CPU", "Passmark MT Score", std::to_string(req.minimum.cpu.passmark_mt_score));
    add_row("Minimum", "GPU", "Name", req.minimum.gpu.name);
    add_row("Minimum", "GPU", "Vendor", req.minimum.gpu.vendor);
    add_row("Minimum", "GPU", "Architecture", req.minimum.gpu.architecture);
    add_row("Minimum", "GPU", "VRAM (MB)", std::to_string(req.minimum.gpu.vram));
    add_row("Minimum", "GPU", "VRAM Type", req.minimum.gpu.vram_type);
    add_row("Minimum", "GPU", "Base Clock (MHz)", std::to_string(req.minimum.gpu.base_clock));
    add_row("Minimum", "GPU", "Boost Clock (MHz)", std::to_string(req.minimum.gpu.boost_clock));
    add_row("Minimum", "GPU", "Compute Units", std::to_string(req.minimum.gpu.compute_units));
    add_row("Minimum", "GPU", "Passmark Score", std::to_string(req.minimum.gpu.passmark_score));
    add_row("Minimum", "RAM", "Capacity (GB)", std::to_string(req.minimum.ram));
    add_row("Minimum", "RAM", "Type", req.minimum.ram_type);
    add_row("Minimum", "RAM", "Speed (MT/s)", std::to_string(req.minimum.ram_speed));
    add_row("Minimum", "RAM", "Channels", std::to_string(req.minimum.ram_channels));
    add_row("Minimum", "RAM", "ECC", req.minimum.ram_ecc ? "Yes" : "No");
    add_row("Minimum", "Storage", "Capacity (GB)", std::to_string(req.minimum.storage.capacity));
    add_row("Minimum", "Storage", "Type", req.minimum.storage.type);
    add_row("Minimum", "Storage", "Read Speed (MB/s)", std::to_string(req.minimum.storage.read_speed));
    add_row("Minimum", "Storage", "Write Speed (MB/s)", std::to_string(req.minimum.storage.write_speed));
    add_row("Minimum", "DirectX", "Version", req.minimum.directx);
    
    // Recommended
    add_row("Recommended", "OS", "Version", req.recommended.os);
    add_row("Recommended", "CPU", "Name", req.recommended.cpu.name);
    add_row("Recommended", "CPU", "Base Clock (GHz)", std::to_string(req.recommended.cpu.base_clock));
    add_row("Recommended", "CPU", "Boost Clock (GHz)", std::to_string(req.recommended.cpu.boost_clock));
    add_row("Recommended", "CPU", "Cores", std::to_string(req.recommended.cpu.cores));
    add_row("Recommended", "CPU", "Threads", std::to_string(req.recommended.cpu.threads));
    add_row("Recommended", "CPU", "L3 Cache (KB)", std::to_string(req.recommended.cpu.l3_cache));
    add_row("Recommended", "CPU", "Instruction Set", req.recommended.cpu.instruction_set);
    add_row("Recommended", "CPU", "Passmark Score", std::to_string(req.recommended.cpu.passmark_score));
    add_row("Recommended", "CPU", "Passmark MT Score", std::to_string(req.recommended.cpu.passmark_mt_score));
    add_row("Recommended", "GPU", "Name", req.recommended.gpu.name);
    add_row("Recommended", "GPU", "Vendor", req.recommended.gpu.vendor);
    add_row("Recommended", "GPU", "Architecture", req.recommended.gpu.architecture);
    add_row("Recommended", "GPU", "VRAM (MB)", std::to_string(req.recommended.gpu.vram));
    add_row("Recommended", "GPU", "VRAM Type", req.recommended.gpu.vram_type);
    add_row("Recommended", "GPU", "Base Clock (MHz)", std::to_string(req.recommended.gpu.base_clock));
    add_row("Recommended", "GPU", "Boost Clock (MHz)", std::to_string(req.recommended.gpu.boost_clock));
    add_row("Recommended", "GPU", "Compute Units", std::to_string(req.recommended.gpu.compute_units));
    add_row("Recommended", "GPU", "Passmark Score", std::to_string(req.recommended.gpu.passmark_score));
    add_row("Recommended", "RAM", "Capacity (GB)", std::to_string(req.recommended.ram));
    add_row("Recommended", "RAM", "Type", req.recommended.ram_type);
    add_row("Recommended", "RAM", "Speed (MT/s)", std::to_string(req.recommended.ram_speed));
    add_row("Recommended", "RAM", "Channels", std::to_string(req.recommended.ram_channels));
    add_row("Recommended", "RAM", "ECC", req.recommended.ram_ecc ? "Yes" : "No");
    add_row("Recommended", "Storage", "Capacity (GB)", std::to_string(req.recommended.storage.capacity));
    add_row("Recommended", "Storage", "Type", req.recommended.storage.type);
    add_row("Recommended", "Storage", "Read Speed (MB/s)", std::to_string(req.recommended.storage.read_speed));
    add_row("Recommended", "Storage", "Write Speed (MB/s)", std::to_string(req.recommended.storage.write_speed));
    add_row("Recommended", "DirectX", "Version", req.recommended.directx);
    
    // High (if available)
    if (req.high.cpu.name != "") {
        add_row("High", "OS", "Version", req.high.os);
        add_row("High", "CPU", "Name", req.high.cpu.name);
        add_row("High", "CPU", "Base Clock (GHz)", std::to_string(req.high.cpu.base_clock));
        add_row("High", "CPU", "Boost Clock (GHz)", std::to_string(req.high.cpu.boost_clock));
        add_row("High", "CPU", "Cores", std::to_string(req.high.cpu.cores));
        add_row("High", "CPU", "Threads", std::to_string(req.high.cpu.threads));
        add_row("High", "CPU", "L3 Cache (KB)", std::to_string(req.high.cpu.l3_cache));
        add_row("High", "CPU", "Instruction Set", req.high.cpu.instruction_set);
        add_row("High", "CPU", "Passmark Score", std::to_string(req.high.cpu.passmark_score));
        add_row("High", "CPU", "Passmark MT Score", std::to_string(req.high.cpu.passmark_mt_score));
        add_row("High", "GPU", "Name", req.high.gpu.name);
        add_row("High", "GPU", "Vendor", req.high.gpu.vendor);
        add_row("High", "GPU", "Architecture", req.high.gpu.architecture);
        add_row("High", "GPU", "VRAM (MB)", std::to_string(req.high.gpu.vram));
        add_row("High", "GPU", "VRAM Type", req.high.gpu.vram_type);
        add_row("High", "GPU", "Base Clock (MHz)", std::to_string(req.high.gpu.base_clock));
        add_row("High", "GPU", "Boost Clock (MHz)", std::to_string(req.high.gpu.boost_clock));
        add_row("High", "GPU", "Compute Units", std::to_string(req.high.gpu.compute_units));
        add_row("High", "GPU", "Passmark Score", std::to_string(req.high.gpu.passmark_score));
        add_row("High", "RAM", "Capacity (GB)", std::to_string(req.high.ram));
        add_row("High", "RAM", "Type", req.high.ram_type);
        add_row("High", "RAM", "Speed (MT/s)", std::to_string(req.high.ram_speed));
        add_row("High", "RAM", "Channels", std::to_string(req.high.ram_channels));
        add_row("High", "RAM", "ECC", req.high.ram_ecc ? "Yes" : "No");
        add_row("High", "Storage", "Capacity (GB)", std::to_string(req.high.storage.capacity));
        add_row("High", "Storage", "Type", req.high.storage.type);
        add_row("High", "Storage", "Read Speed (MB/s)", std::to_string(req.high.storage.read_speed));
        add_row("High", "Storage", "Write Speed (MB/s)", std::to_string(req.high.storage.write_speed));
        add_row("High", "DirectX", "Version", req.high.directx);
    }
    
    // Ultra (if available)
    if (req.ultra.cpu.name != "") {
        add_row("Ultra", "OS", "Version", req.ultra.os);
        add_row("Ultra", "CPU", "Name", req.ultra.cpu.name);
        add_row("Ultra", "CPU", "Base Clock (GHz)", std::to_string(req.ultra.cpu.base_clock));
        add_row("Ultra", "CPU", "Boost Clock (GHz)", std::to_string(req.ultra.cpu.boost_clock));
        add_row("Ultra", "CPU", "Cores", std::to_string(req.ultra.cpu.cores));
        add_row("Ultra", "CPU", "Threads", std::to_string(req.ultra.cpu.threads));
        add_row("Ultra", "CPU", "L3 Cache (KB)", std::to_string(req.ultra.cpu.l3_cache));
        add_row("Ultra", "CPU", "Instruction Set", req.ultra.cpu.instruction_set);
        add_row("Ultra", "CPU", "Passmark Score", std::to_string(req.ultra.cpu.passmark_score));
        add_row("Ultra", "CPU", "Passmark MT Score", std::to_string(req.ultra.cpu.passmark_mt_score));
        add_row("Ultra", "GPU", "Name", req.ultra.gpu.name);
        add_row("Ultra", "GPU", "Vendor", req.ultra.gpu.vendor);
        add_row("Ultra", "GPU", "Architecture", req.ultra.gpu.architecture);
        add_row("Ultra", "GPU", "VRAM (MB)", std::to_string(req.ultra.gpu.vram));
        add_row("Ultra", "GPU", "VRAM Type", req.ultra.gpu.vram_type);
        add_row("Ultra", "GPU", "Base Clock (MHz)", std::to_string(req.ultra.gpu.base_clock));
        add_row("Ultra", "GPU", "Boost Clock (MHz)", std::to_string(req.ultra.gpu.boost_clock));
        add_row("Ultra", "GPU", "Compute Units", std::to_string(req.ultra.gpu.compute_units));
        add_row("Ultra", "GPU", "Passmark Score", std::to_string(req.ultra.gpu.passmark_score));
        add_row("Ultra", "RAM", "Capacity (GB)", std::to_string(req.ultra.ram));
        add_row("Ultra", "RAM", "Type", req.ultra.ram_type);
        add_row("Ultra", "RAM", "Speed (MT/s)", std::to_string(req.ultra.ram_speed));
        add_row("Ultra", "RAM", "Channels", std::to_string(req.ultra.ram_channels));
        add_row("Ultra", "RAM", "ECC", req.ultra.ram_ecc ? "Yes" : "No");
        add_row("Ultra", "Storage", "Capacity (GB)", std::to_string(req.ultra.storage.capacity));
        add_row("Ultra", "Storage", "Type", req.ultra.storage.type);
        add_row("Ultra", "Storage", "Read Speed (MB/s)", std::to_string(req.ultra.storage.read_speed));
        add_row("Ultra", "Storage", "Write Speed (MB/s)", std::to_string(req.ultra.storage.write_speed));
        add_row("Ultra", "DirectX", "Version", req.ultra.directx);
    }
    
    // Ray tracing (if available)
    if (req.ray_tracing.has_value()) {
        add_row("Ray Tracing", "OS", "Version", req.ray_tracing->os);
        add_row("Ray Tracing", "CPU", "Name", req.ray_tracing->cpu.name);
        add_row("Ray Tracing", "CPU", "Base Clock (GHz)", std::to_string(req.ray_tracing->cpu.base_clock));
        add_row("Ray Tracing", "CPU", "Boost Clock (GHz)", std::to_string(req.ray_tracing->cpu.boost_clock));
        add_row("Ray Tracing", "CPU", "Cores", std::to_string(req.ray_tracing->cpu.cores));
        add_row("Ray Tracing", "CPU", "Threads", std::to_string(req.ray_tracing->cpu.threads));
        add_row("Ray Tracing", "CPU", "L3 Cache (KB)", std::to_string(req.ray_tracing->cpu.l3_cache));
        add_row("Ray Tracing", "CPU", "Instruction Set", req.ray_tracing->cpu.instruction_set);
        add_row("Ray Tracing", "CPU", "Passmark Score", std::to_string(req.ray_tracing->cpu.passmark_score));
        add_row("Ray Tracing", "CPU", "Passmark MT Score", std::to_string(req.ray_tracing->cpu.passmark_mt_score));
        add_row("Ray Tracing", "GPU", "Name", req.ray_tracing->gpu.name);
        add_row("Ray Tracing", "GPU", "Vendor", req.ray_tracing->gpu.vendor);
        add_row("Ray Tracing", "GPU", "Architecture", req.ray_tracing->gpu.architecture);
        add_row("Ray Tracing", "GPU", "VRAM (MB)", std::to_string(req.ray_tracing->gpu.vram));
        add_row("Ray Tracing", "GPU", "VRAM Type", req.ray_tracing->gpu.vram_type);
        add_row("Ray Tracing", "GPU", "Base Clock (MHz)", std::to_string(req.ray_tracing->gpu.base_clock));
        add_row("Ray Tracing", "GPU", "Boost Clock (MHz)", std::to_string(req.ray_tracing->gpu.boost_clock));
        add_row("Ray Tracing", "GPU", "Compute Units", std::to_string(req.ray_tracing->gpu.compute_units));
        add_row("Ray Tracing", "GPU", "Passmark Score", std::to_string(req.ray_tracing->gpu.passmark_score));
        add_row("Ray Tracing", "RAM", "Capacity (GB)", std::to_string(req.ray_tracing->ram));
        add_row("Ray Tracing", "RAM", "Type", req.ray_tracing->ram_type);
        add_row("Ray Tracing", "RAM", "Speed (MT/s)", std::to_string(req.ray_tracing->ram_speed));
        add_row("Ray Tracing", "RAM", "Channels", std::to_string(req.ray_tracing->ram_channels));
        add_row("Ray Tracing", "RAM", "ECC", req.ray_tracing->ram_ecc ? "Yes" : "No");
        add_row("Ray Tracing", "Storage", "Capacity (GB)", std::to_string(req.ray_tracing->storage.capacity));
        add_row("Ray Tracing", "Storage", "Type", req.ray_tracing->storage.type);
        add_row("Ray Tracing", "Storage", "Read Speed (MB/s)", std::to_string(req.ray_tracing->storage.read_speed));
        add_row("Ray Tracing", "Storage", "Write Speed (MB/s)", std::to_string(req.ray_tracing->storage.write_speed));
        add_row("Ray Tracing", "DirectX", "Version", req.ray_tracing->directx);
    }
    
    return oss.str();
}

String ResultsFormatter::format_markdown(const RequirementResult& req) const {
    std::ostringstream oss;
    oss << "# GameReqAnalyzer Report\n\n";
    oss << "## System Requirements Analysis\n\n";
    
    // Minimum requirements
    oss << "### Minimum Requirements\n\n";
    oss << "- **OS**: " << req.minimum.os << "\n";
    oss << "- **CPU**: " << req.minimum.cpu.name << " @ " 
        << std::fixed << std::setprecision(1) << req.minimum.cpu.base_clock 
        << " GHz (" << req.minimum.cpu.cores << " cores/" 
        << req.minimum.cpu.threads << " threads)\n";
    oss << "- **GPU**: " << req.minimum.gpu.name << " ("
        << req.minimum.gpu.vram / 1024 << " GB VRAM)\n";
    oss << "- **RAM**: " << req.minimum.ram << " GB\n";
    oss << "- **Storage**: " << req.minimum.storage.type << " " 
        << req.minimum.storage.name << " (" 
        << req.minimum.storage.capacity << " GB available)\n";
    oss << "- **DirectX**: " << req.minimum.directx << "\n\n";
    
    // Recommended requirements
    oss << "### Recommended Requirements\n\n";
    oss << "- **OS**: " << req.recommended.os << "\n";
    oss << "- **CPU**: " << req.recommended.cpu.name << " @ " 
        << std::fixed << std::setprecision(1) << req.recommended.cpu.base_clock 
        << " GHz (" << req.recommended.cpu.cores << " cores/" 
        << req.recommended.cpu.threads << " threads)\n";
    oss << "- **GPU**: " << req.recommended.gpu.name << " ("
        << req.recommended.gpu.vram / 1024 << " GB VRAM)\n";
    oss << "- **RAM**: " << req.recommended.ram << " GB\n";
    oss << "- **Storage**: " << req.recommended.storage.type << " " 
        << req.recommended.storage.name << " (" 
        << req.recommended.storage.capacity << " GB available)\n";
    oss << "- **DirectX**: " << req.recommended.directx << "\n\n";
    
    // High requirements (if available)
    if (req.high.cpu.name != "") {
        oss << "### High Requirements\n\n";
        oss << "- **CPU**: " << req.high.cpu.name << " @ " 
            << std::fixed << std::setprecision(1) << req.high.cpu.base_clock 
            << " GHz (" << req.high.cpu.cores << " cores/" 
            << req.high.cpu.threads << " threads)\n";
        oss << "- **GPU**: " << req.high.gpu.name << " ("
            << req.high.gpu.vram / 1024 << " GB VRAM)\n";
        oss << "- **RAM**: " << req.high.ram << " GB\n";
        oss << "- **Storage**: " << req.high.storage.type << " " 
            << req.high.storage.name << " (" 
            << req.high.storage.capacity << " GB available)\n";
        oss << "- **DirectX**: " << req.high.directx << "\n\n";
    }
    
    // Ultra requirements (if available)
    if (req.ultra.cpu.name != "") {
        oss << "### Ultra Requirements\n\n";
        oss << "- **CPU**: " << req.ultra.cpu.name << " @ " 
            << std::fixed << std::setprecision(1) << req.ultra.cpu.base_clock 
            << " GHz (" << req.ultra.cpu.cores << " cores/" 
            << req.ultra.cpu.threads << " threads)\n";
        oss << "- **GPU**: " << req.ultra.gpu.name << " ("
            << req.ultra.gpu.vram / 1024 << " GB VRAM)\n";
        oss << "- **RAM**: " << req.ultra.ram << " GB\n";
        oss << "- **Storage**: " << req.ultra.storage.type << " " 
            << req.ultra.storage.name << " (" 
            << req.ultra.storage.capacity << " GB available)\n";
        oss << "- **DirectX**: " << req.ultra.directx << "\n\n";
    }
    
    // Ray tracing requirements (if available)
    if (req.ray_tracing.has_value()) {
        oss << "### Ray Tracing Requirements\n\n";
        oss << "- **CPU**: " << req.ray_tracing->cpu.name << " @ " 
            << std::fixed << std::setprecision(1) << req.ray_tracing->cpu.base_clock 
            << " GHz (" << req.ray_tracing->cpu.cores << " cores/" 
            << req.ray_tracing->cpu.threads << " threads)\n";
        oss << "- **GPU**: " << req.ray_tracing->gpu.name << " ("
            << req.ray_tracing->gpu.vram / 1024 << " GB VRAM)\n";
        oss << "- **RAM**: " << req.ray_tracing->ram << " GB\n";
        oss << "- **Storage**: " << req.ray_tracing->storage.type << " " 
            << req.ray_tracing->storage.name << " (" 
            << req.ray_tracing->storage.capacity << " GB available)\n";
        oss << "- **DirectX**: " << req.ray_tracing->directx << "\n\n";
    }
    
    // Confidence and notes
    oss << "---\n";
    oss << "**Analysis Confidence**: " << std::fixed << std::setprecision(1) 
        << (req.confidence * 100.0) << "%\n\n";
    
    if (!req.analysis_notes.empty()) {
        oss << "**Analysis Notes**: " << req.analysis_notes << "\n\n";
    }
    
    if (!req.warnings.empty()) {
        oss << "**Warnings**:\n";
        for (const auto& warning : req.warnings) {
            oss << "- " << warning << "\n";
        }
        oss << "\n";
    }
    
    if (!req.assumptions.empty()) {
        oss << "**Assumptions**:\n";
        for (const auto& assumption : req.assumptions) {
            oss << "- " << assumption << "\n";
        }
        oss << "\n";
    }
    
    return oss.str();
}

String ResultsFormatter::format_html(const RequirementResult& req) const {
    std::ostringstream oss;
    oss << "<!DOCTYPE html>\n<html>\n<head>\n";
    oss << "<title>GameReqAnalyzer Report</title>\n";
    oss << "<style>\n";
    oss << "body { font-family: Arial, sans-serif; margin: 40px; }\n";
    oss << "h1, h2, h3 { color: #2c3e50; }\n";
    oss << ".section { margin-bottom: 30px; padding: 20px; border: 1px solid #ddd; border-radius: 5px; }\n";
    oss << ".requirement { margin: 10px 0; }\n";
    oss << ".label { font-weight: bold; color: #3498db; }\n";
    oss << ".value { margin-left: 10px; }\n";
    oss << ".confidence { background: #ecf0f1; padding: 10px; border-radius: 5px; }\n";
    oss << ".warning { color: #e74c3c; }\n";
    oss << ".assumption { color: #f39c12; }\n";
    oss << "</style>\n";
    oss << "</head>\n<body>\n";
    oss << "<h1>GameReqAnalyzer Report</h1>\n";
    oss << "<h2>System Requirements Analysis</h2>\n";

    auto format_req_html = [&](const HardwareRequirement& r, const String& label) {
        std::ostringstream s;
        s << "<div class=\"section\">\n";
        s << "<h3>" << label << "</h3>\n";
        s << "<div class=\"requirement\"><span class=\"label\">OS:</span><span class=\"value\"> " 
            << r.os_minimum << " / " << r.os_recommended << "</span></div>\n";
        s << "<div class=\"requirement\"><span class=\"label\">CPU:</span><span class=\"value\"> " 
            << r.cpu_vendor << " " << r.cpu_arch << " @ " 
            << std::fixed << std::setprecision(1) << r.cpu_base_clock 
            << " GHz (" << r.cpu_cores << " cores/" 
            << r.cpu_threads << " threads)</span></div>\n";
        s << "<div class=\"requirement\"><span class=\"label\">GPU:</span><span class=\"value\"> " 
            << r.gpu_vendor << " " << r.gpu_architecture << " ("
            << r.gpu_vram / 1024 << " GB " << r.gpu_vram_type << " VRAM)</span></div>\n";
        s << "<div class=\"requirement\"><span class=\"label\">RAM:</span><span class=\"value\"> " 
            << r.ram_capacity / 1024 << " GB " << r.ram_type << " @ " << r.ram_speed << " MT/s</span></div>\n";
        s << "<div class=\"requirement\"><span class=\"label\">Storage:</span><span class=\"value\"> " 
            << r.storage_type << " " << r.storage_capacity << " GB ("
            << r.storage_read_speed << " MB/s read)</span></div>\n";
        s << "<div class=\"requirement\"><span class=\"label\">API:</span><span class=\"value\"> " 
            << "DirectX " << r.dx_version << ", Vulkan " << r.vk_version << ", Metal " << r.metal_version << "</span></div>\n";
        s << "</div>\n";
        return s.str();
    };

    oss << format_req_html(req.minimum, "Minimum Requirements");
    oss << format_req_html(req.recommended, "Recommended Requirements");
    
    if (!req.high.specific_cpus.empty() || req.high.cpu_cores > 0) {
        oss << format_req_html(req.high, "High Requirements");
    }
    
    if (!req.ultra.specific_cpus.empty() || req.ultra.cpu_cores > 0) {
        oss << format_req_html(req.ultra, "Ultra Requirements");
    }
    
    if (req.ray_tracing.has_value()) {
        oss << format_req_html(req.ray_tracing.value(), "Ray Tracing Requirements");
    }
    
    // Confidence and notes
    oss << "<div class=\"section\">\n";
    oss << "<h3>Analysis Summary</h3>\n";
    oss << "<div class=\"confidence\">\n";
    oss << "<strong>Analysis Confidence: </span><span class=\"value\"> " 
        << std::fixed << std::setprecision(1) << (req.confidence * 100.0) << "%</span>\n";
    oss << "</div>\n";
    
    if (!req.analysis_notes.empty()) {
        oss << "<div class=\"requirement\"><span class=\"label\">Analysis Notes:</span><span class=\"value\"> " 
            << req.analysis_notes << "</span></div>\n";
    }
    
    if (!req.warnings.empty()) {
        oss << "<div class=\"requirement\"><span class=\"label\">Warnings:</span></div>\n";
        oss << "<ul>\n";
        for (const auto& warning : req.warnings) {
            oss << "<li class=\"warning\">" << warning << "</li>\n";
        }
        oss << "</ul>\n";
    }
    
    if (!req.assumptions.empty()) {
        oss << "<div class=\"requirement\"><span class=\"label\">Assumptions:</span></div>\n";
        oss << "<ul>\n";
        for (const auto& assumption : req.assumptions) {
            oss << "<li class=\"assumption\">" << assumption << "</li>\n";
        }
        oss << "</ul>\n";
    }
    
    oss << "</div>\n";
    oss << "</body>\n</html>\n";
    
    return oss.str();
}

Result<void> ResultsFormatter::write_to_file(const String& content, const Path& path) const {
    std::ofstream file(path);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to open file for writing"));
    }
    file << content;
    return {};
}

String ResultsFormatter::format_hardware_req(const HardwareRequirement& req, RequirementLevel level) const {
    std::ostringstream oss;
    oss << "Hardware Requirement (" << static_cast<int>(level) << "):\n";
    oss << "  CPU: " << req.cpu_vendor << " " << req.cpu_arch << " @ " 
        << std::fixed << std::setprecision(1) << req.cpu_base_clock 
        << " GHz (" << req.cpu_cores << " cores/" 
        << req.cpu_threads << " threads)\n";
    oss << "  GPU: " << req.gpu_vendor << " " << req.gpu_architecture << " ("
        << req.gpu_vram / 1024 << " GB " << req.gpu_vram_type << " VRAM)\n";
    oss << "  RAM: " << req.ram_capacity / 1024 << " GB " << req.ram_type << " @ " << req.ram_speed << " MT/s\n";
    oss << "  Storage: " << req.storage_type << " " << req.storage_capacity << " GB ("
        << req.storage_read_speed << " MB/s read)\n";
    oss << "  DirectX: " << req.dx_version << ", Vulkan: " << req.vk_version << "\n";
    return oss.str();
}

String ResultsFormatter::format_cpu_recommendations(const std::vector<const CPUEntry*>& cpus) const {
    if (cpus.empty()) {
        return "No CPU recommendations available\n";
    }
    
    std::ostringstream oss;
    oss << "CPU Recommendations:\n";
    oss << "-------------------------\n";
    oss << std::left << std::setw(4) << "#" 
        << std::setw(25) << "Model" 
        << std::setw(8) << "Cores" 
        << std::setw(6) << "Threads" 
        << std::setw(6) << "GHz" 
        << std::setw(8) << "Score" 
        << std::setw(8) << "$" 
        << std::setw(10) << "$/Score" 
        << std::setw(10) << "Perf/$" << "\n";
    oss << "-------------------------\n";
    
    for (size_t i = 0; i < cpus.size() && i < 10; ++i) {
        const auto& cpu = cpus[i];
        oss << std::left << std::setw(4) << (i+1)
            << std::setw(25) << cpu->name.substr(0, 24)
            << std::setw(8) << cpu->cores
            << std::setw(6) << cpu->threads
            << std::setw(6) << std::fixed << std::setprecision(1) << cpu->base_clock
            << std::setw(8) << cpu->passmark_mt
            << std::setw(8) << std::fixed << std::setprecision(1) << cpu->price_usd
            << std::setw(10) << (cpu->price_usd > 0 ? cpu->passmark_mt / cpu->price_usd : 0.0)
            << std::setw(10) << cpu->performance_per_dollar() << "\n";
    }
    
    return oss.str();
}

String ResultsFormatter::format_gpu_recommendations(const std::vector<const GPUEntry*>& gpus) const {
    if (gpus.empty()) {
        return "No GPU recommendations available\n";
    }
    
    std::ostringstream oss;
    oss << "GPU Recommendations:\n";
    oss << "-------------------------\n";
    oss << std::left << std::setw(4) << "#" 
        << std::setw(25) << "Model" 
        << std::setw(6) << "VRAM" 
        << std::setw(6) << "Bandw" 
        << std::setw(6) << "Clock" 
        << std::setw(8) << "Score" 
        << std::setw(8) << "$" 
        << std::setw(10) << "$/Score" 
        << std::setw(10) << "Perf/$" << "\n";
    oss << "-------------------------\n";
    
    for (size_t i = 0; i < gpus.size() && i < 10; ++i) {
        const auto& gpu = gpus[i];
        oss << std::left << std::setw(4) << (i+1)
            << std::setw(25) << gpu->name.substr(0, 24)
            << std::setw(6) << (gpu->vram / 1024)
            << std::setw(6) << static_cast<int>(gpu->vram_bandwidth)
            << std::setw(6) << static_cast<int>(gpu->boost_clock)
            << std::setw(8) << gpu->passmark_g3d
            << std::setw(8) << std::fixed << std::setprecision(1) << gpu->price_usd
            << std::setw(10) << (gpu->price_usd > 0 ? gpu->passmark_g3d / gpu->price_usd : 0.0)
            << std::setw(10) << gpu->performance_per_dollar() << "\n";
    }
    
    return oss.str();
}

String ResultsFormatter::format_ram_recommendations(const std::vector<const RAMEntry*>& ram) const {
    if (ram.empty()) {
        return "No RAM recommendations available\n";
    }
    
    std::ostringstream oss;
    oss << "RAM Recommendations:\n";
    oss << "--------------------\n";
    oss << std::left << std::setw(4) << "#" 
        << std::setw(25) << "Model" 
        << std::setw(8) << "Capacity" 
        << std::setw(8) << "Speed" 
        << std::setw(6) << "Channels" 
        << std::setw(8) << "$" 
        << std::setw(10) << "$/GB" << "\n";
    oss << "--------------------\n";
    
    for (size_t i = 0; i < ram.size() && i < 10; ++i) {
        const auto& r = ram[i];
        oss << std::left << std::setw(4) << (i+1)
            << std::setw(25) << r->name.substr(0, 24)
            << std::setw(8) << r->capacity << " GB"
            << std::setw(8) << r->speed << " MT/s"
            << std::setw(6) << r->channels
            << std::setw(8) << std::fixed << std::setprecision(1) << r->price_usd
            << std::setw(10) << (r->price_usd > 0 ? r->price_usd / r->capacity : 0.0) << "\n";
    }
    
    return oss.str();
}

String ResultsFormatter::format_storage_recommendations(const std::vector<const StorageEntry*>& storage) const {
    if (storage.empty()) {
        return "No storage recommendations available\n";
    }
    
    std::ostringstream oss;
    oss << "Storage Recommendations:\n";
    oss << "-------------------------\n";
    oss << std::left << std::setw(4) << "#" 
        << std::setw(25) << "Model" 
        << std::setw(10) << "Capacity" 
        << std::setw(10) << "Read Speed" 
        << std::setw(10) << "Write Speed" 
        << std::setw(8) << "$" 
        << std::setw(10) << "$/GB" << "\n";
    oss << "-------------------------\n";
    
    for (size_t i = 0; i < storage.size() && i < 10; ++i) {
        const auto& s = storage[i];
        oss << std::left << std::setw(4) << (i+1)
            << std::setw(25) << s->name.substr(0, 24)
            << std::setw(10) << s->capacity << " GB"
            << std::setw(10) << std::fixed << std::setprecision(0) << s->seq_read << " MB/s"
            << std::setw(10) << std::fixed << std::setprecision(0) << s->seq_write << " MB/s"
            << std::setw(8) << std::fixed << std::setprecision(1) << s->price_usd
            << std::setw(10) << (s->price_usd > 0 ? s->price_usd / s->capacity : 0.0) << "\n";
    }
    
    return oss.str();
}

// Helper functions for formatting
String ResultsFormatter::format_bytes(u64 bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double val = static_cast<double>(bytes);
    
    while (val >= 1024.0 && i < 4) {
        val /= 1024.0;
        i++;
    }
    
    return fmt::format("{:.2f} {}", val, units[i]);
}

String ResultsFormatter::format_number(u64 num) const {
    if (!config_.use_grouping_separators) {
        return fmt::format("{}", num);
    }
    
    // Add thousands separators
    std::string str = std::to_string(num);
    int insert_pos = static_cast<int>(str.length()) - 3;
    
    while (insert_pos > 0) {
        str.insert(insert_pos, ",");
        insert_pos -= 3;
    }
    
    return str;
}

String ResultsFormatter::format_duration(Duration d) const {
    using namespace std::chrono;
    
    switch (config_.duration_format) {
        case OutputConfig::DurationFormat::Short: {
            auto ms = duration_cast<milliseconds>(d).count();
            return fmt::format("{} ms", ms);
        }
        case OutputConfig::DurationFormat::Long: {
            auto s = duration_cast<seconds>(d).count();
            auto ms = duration_cast<milliseconds>(d).count() % 1000;
            if (ms > 0) {
                return fmt::format("{} s {} ms", s, ms);
            } else {
                return fmt::format("{} s", s);
            }
        }
        case OutputConfig::DurationFormat::ISO:
        default: {
            // Human readable format
            auto days = duration_cast<days>(d);
            d -= days;
            auto hours = duration_cast<hours>(d);
            d -= hours;
            auto minutes = duration_cast<minutes>(d);
            d -= minutes;
            auto seconds = duration_cast<seconds>(d);
            
            std::string result;
            if (days.count() > 0) {
                result += fmt::format("{}d ", days.count());
            }
            if (hours.count() > 0 || !result.empty()) {
                result += fmt::format("{}h ", hours.count());
            }
            if (minutes.count() > 0 || !result.empty()) {
                result += fmt::format("{}m ", minutes.count());
            }
            result += fmt::format("{}s", seconds.count());
            
            return result;
        }
    }
}
