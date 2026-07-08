#include <game_req_analyzer/analyzers/executable_analyzer.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>

using namespace game_req;

ExecutableAnalyzer::ExecutableAnalyzer(const AnalyzerConfig& config) : config_(config) {}

ExecutableAnalyzer::~ExecutableAnalyzer() = default;

Result<std::vector<ExecutableInfo>> ExecutableAnalyzer::analyze(const std::vector<FileInfo>& executables) {
    std::vector<ExecutableInfo> results;
    results.reserve(executables.size());
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_executables = 0;
        stats_.total_code_size = 0;
        stats_.total_data_size = 0;
        stats_.total_estimated_ram = 0;
        stats_.architecture_counts.clear();
        stats_.platform_counts.clear();
        stats_.compiler_counts.clear();
        stats_.engine_counts.clear();
    }
    
    for (const auto& exe : executables) {
        auto result = analyze_single(exe);
        if (result) {
            results.push_back(*result);
            
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_executables++;
            stats_.total_code_size += result->code_size;
            stats_.total_data_size += result->data_size;
            stats_.total_estimated_ram += result->estimated_ram;
            stats_.architecture_counts[result->architecture]++;
            stats_.platform_counts[result->platform]++;
            stats_.compiler_counts[result->compiler]++;
            if (!result->engine_hint.empty()) {
                stats_.engine_counts[result->engine_hint]++;
            }
        } else {
            LOG_WARN("Failed to analyze executable {}: {}", exe.path.string(), result.error().message);
        }
    }
    
    return results;
}

Result<ExecutableInfo> ExecutableAnalyzer::analyze_single(const FileInfo& exe) {
    ExecutableInfo info;
    info.disk_size = exe.size;
    
    try {
        switch (exe.type) {
            case FileType::PE32:
            case FileType::PE64:
            case FileType::DLL:
                return analyze_pe(exe.path);
            case FileType::ELF32:
            case FileType::ELF64:
            case FileType::SO:
                return analyze_elf(exe.path);
            case FileType::MachO32:
            case FileType::MachO64:
            case FileType::DYLIB:
                return analyze_macho(exe.path);
            default:
                return analyze_generic(exe.path);
        }
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Executable analysis failed: {}", e.what())));
    }
}

Result<ExecutableInfo> ExecutableAnalyzer::analyze_pe(const Path& path) {
    ExecutableInfo info;
    info.platform = "Windows";
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open PE file: {}", path.string())));
    }
    
    file.seekg(0, std::ios::end);
    info.disk_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    uint16_t dos_magic;
    file.read(reinterpret_cast<char*>(&dos_magic), 2);
    if (dos_magic != 0x5A4D) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Not a valid PE file (no MZ signature)"));
    }
    
    file.seekg(0x3C);
    uint32_t pe_offset;
    file.read(reinterpret_cast<char*>(&pe_offset), 4);
    
    file.seekg(pe_offset);
    uint32_t pe_magic;
    file.read(reinterpret_cast<char*>(&pe_magic), 4);
    if (pe_magic != 0x00004550) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Not a valid PE file (no PE signature)"));
    }
    
    uint16_t machine;
    file.read(reinterpret_cast<char*>(&machine), 2);
    
    switch (machine) {
        case 0x014c: info.architecture = "x86"; info.is_64bit = false; break;
        case 0x8664: info.architecture = "x86_64"; info.is_64bit = true; break;
        case 0x01c0: info.architecture = "ARM"; break;
        case 0xaa64: info.architecture = "ARM64"; info.is_64bit = true; break;
        case 0x01c4: info.architecture = "ARM Thumb-2"; break;
        case 0x0ebc: info.architecture = "EBC"; break;
        case 0x01f0: info.architecture = "PowerPC"; break;
        case 0x0200: info.architecture = "IA64"; info.is_64bit = true; break;
        default: info.architecture = std::format("Unknown (0x{:04X})", machine); break;
    }
    
    uint16_t num_sections;
    file.read(reinterpret_cast<char*>(&num_sections), 2);
    file.seekg(12, std::ios::cur);
    uint16_t optional_header_size;
    file.read(reinterpret_cast<char*>(&optional_header_size), 2);
    uint16_t characteristics;
    file.read(reinterpret_cast<char*>(&characteristics), 2);
    
    info.has_aslr = (characteristics & 0x0040) != 0;
    info.has_dep = (characteristics & 0x0100) != 0;
    info.has_cfg = (characteristics & 0x4000) != 0;
    
    uint16_t magic;
    file.read(reinterpret_cast<char*>(&magic), 2);
    
    bool is_pe32plus = (magic == 0x20B);
    
    if (is_pe32plus) {
        info.is_64bit = true;
        file.seekg(24, std::ios::cur);
        info.image_base = 0;
        uint64_t ib;
        file.read(reinterpret_cast<char*>(&ib), 8);
        info.image_base = ib;
    } else {
        info.is_64bit = false;
        file.seekg(20, std::ios::cur);
        uint32_t ib;
        file.read(reinterpret_cast<char*>(&ib), 4);
        info.image_base = ib;
    }
    
    file.seekg(4, std::ios::cur);
    file.read(reinterpret_cast<char*>(&info.entry_point), is_pe32plus ? 8 : 4);
    file.seekg(is_pe32plus ? 24 : 20, std::ios::cur);
    file.read(reinterpret_cast<char*>(&info.subsystem), 2);
    
    file.seekg(pe_offset + 24 + optional_header_size, std::ios::beg);
    
    for (int i = 0; i < num_sections; ++i) {
        char name[9] = {0};
        file.read(name, 8);
        String section_name(name);
        info.sections.push_back(section_name);
        
        uint32_t virtual_size, virtual_address, raw_size, raw_ptr;
        file.read(reinterpret_cast<char*>(&virtual_size), 4);
        file.read(reinterpret_cast<char*>(&virtual_address), 4);
        file.read(reinterpret_cast<char*>(&raw_size), 4);
        file.read(reinterpret_cast<char*>(&raw_ptr), 4);
        file.seekg(16, std::ios::cur);
        
        if (section_name.find(".text") != String::npos || 
            section_name.find(".code") != String::npos ||
            section_name.find("CODE") != String::npos) {
            info.code_size += virtual_size;
        } else if (section_name.find(".data") != String::npos ||
                   section_name.find(".rdata") != String::npos ||
                   section_name.find(".bss") != String::npos) {
            info.data_size += virtual_size;
        }
    }
    
    uint32_t import_dir_rva = 0, import_dir_size = 0;
    if (is_pe32plus) {
        file.seekg(pe_offset + 24 + 120, std::ios::beg);
    } else {
        file.seekg(pe_offset + 24 + 112, std::ios::beg);
    }
    file.read(reinterpret_cast<char*>(&import_dir_rva), 4);
    file.read(reinterpret_cast<char*>(&import_dir_size), 4);
    
    if (import_dir_rva > 0 && import_dir_size > 0) {
        auto imports = parse_pe_imports(file, pe_offset, import_dir_rva, num_sections);
        info.imported_dlls = std::move(imports);
    }
    
    detect_engine_hints(info, info.imported_dlls);
    info.compiler = detect_compiler(info);
    info.estimated_ram = estimate_ram(info);
    
    return info;
}

std::vector<String> ExecutableAnalyzer::parse_pe_imports(std::ifstream& file, uint32_t pe_offset, uint32_t import_rva, uint16_t num_sections) {
    std::vector<String> dlls;
    std::vector<uint32_t> section_rva, section_raw, section_vsize;
    
    file.seekg(pe_offset + 24 + 240, std::ios::beg);
    for (int i = 0; i < num_sections; ++i) {
        file.seekg(8, std::ios::cur);
        uint32_t vsize, vaddr, rsize, rptr;
        file.read(reinterpret_cast<char*>(&vsize), 4);
        file.read(reinterpret_cast<char*>(&vaddr), 4);
        file.read(reinterpret_cast<char*>(&rsize), 4);
        file.read(reinterpret_cast<char*>(&rptr), 4);
        file.seekg(16, std::ios::cur);
        section_rva.push_back(vaddr);
        section_raw.push_back(rptr);
        section_vsize.push_back(vsize);
    }
    
    auto rva_to_offset = [&](uint32_t rva) -> std::optional<uint64_t> {
        for (size_t i = 0; i < section_rva.size(); ++i) {
            if (rva >= section_rva[i] && rva < section_rva[i] + section_vsize[i]) {
                return section_raw[i] + (rva - section_rva[i]);
            }
        }
        return std::nullopt;
    };
    
    auto import_offset = rva_to_offset(import_rva);
    if (!import_offset) return dlls;
    
    file.seekg(*import_offset);
    
    while (true) {
        uint32_t original_first_thunk, time_date_stamp, forwarder_chain, name_rva, first_thunk;
        file.read(reinterpret_cast<char*>(&original_first_thunk), 4);
        file.read(reinterpret_cast<char*>(&time_date_stamp), 4);
        file.read(reinterpret_cast<char*>(&forwarder_chain), 4);
        file.read(reinterpret_cast<char*>(&name_rva), 4);
        file.read(reinterpret_cast<char*>(&first_thunk), 4);
        
        if (name_rva == 0) break;
        
        auto name_offset = rva_to_offset(name_rva);
        if (name_offset) {
            std::streampos pos = file.tellg();
            file.seekg(*name_offset);
            String dll_name;
            char c;
            while (file.get(c) && c != '\0') dll_name += c;
            if (!dll_name.empty()) dlls.push_back(dll_name);
            file.seekg(pos);
        }
    }
    
    return dlls;
}

Result<ExecutableInfo> ExecutableAnalyzer::analyze_elf(const Path& path) {
    ExecutableInfo info;
    info.platform = "Linux/Unix";
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open ELF file: {}", path.string())));
    }
    
    file.seekg(0, std::ios::end);
    info.disk_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    unsigned char ident[16];
    file.read(reinterpret_cast<char*>(ident), 16);
    
    if (ident[0] != 0x7F || ident[1] != 'E' || ident[2] != 'L' || ident[3] != 'F') {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Not a valid ELF file"));
    }
    
    bool is_64bit = (ident[4] == 2);
    info.is_64bit = is_64bit;
    
    file.seekg(0x10);
    uint16_t type, machine;
    file.read(reinterpret_cast<char*>(&type), 2);
    file.read(reinterpret_cast<char*>(&machine), 2);
    
    switch (machine) {
        case 0x03: info.architecture = "x86"; break;
        case 0x3E: info.architecture = "x86_64"; break;
        case 0x28: info.architecture = "ARM"; break;
        case 0xB7: info.architecture = "AArch64"; break;
        case 0x08: info.architecture = "MIPS"; break;
        case 0x14: info.architecture = "PowerPC"; break;
        case 0x15: info.architecture = "PowerPC64"; break;
        case 0x32: info.architecture = "RISC-V"; break;
        default: info.architecture = std::format("Unknown (0x{:04X})", machine); break;
    }
    
    if (is_64bit) {
        file.seekg(0x20);
        info.entry_point = 0;
        uint64_t ep;
        file.read(reinterpret_cast<char*>(&ep), 8);
        info.entry_point = ep;
        
        file.seekg(0x30);
        uint64_t phoff, shoff;
        file.read(reinterpret_cast<char*>(&phoff), 8);
        file.read(reinterpret_cast<char*>(&shoff), 8);
        
        file.seekg(0x38);
        uint16_t phentsize, phnum;
        file.read(reinterpret_cast<char*>(&phentsize), 2);
        file.read(reinterpret_cast<char*>(&phnum), 2);
        
        file.seekg(phoff);
        for (int i = 0; i < phnum; ++i) {
            uint32_t ptype, pflags;
            uint64_t poffset, pvaddr, ppaddr, pfilesz, pmemsz, palign;
            file.read(reinterpret_cast<char*>(&ptype), 4);
            file.read(reinterpret_cast<char*>(&pflags), 4);
            file.read(reinterpret_cast<char*>(&poffset), 8);
            file.read(reinterpret_cast<char*>(&pvaddr), 8);
            file.read(reinterpret_cast<char*>(&ppaddr), 8);
            file.read(reinterpret_cast<char*>(&pfilesz), 8);
            file.read(reinterpret_cast<char*>(&pmemsz), 8);
            file.read(reinterpret_cast<char*>(&palign), 8);
            
            String seg_name = (ptype == 1) ? "LOAD" : "OTHER";
            info.sections.push_back(seg_name);
            
            if (pflags & 4) info.code_size += pmemsz;
            if (pflags & 2) info.data_size += pmemsz;
        }
    } else {
        file.seekg(0x18);
        uint32_t ep;
        file.read(reinterpret_cast<char*>(&ep), 4);
        info.entry_point = ep;
        
        file.seekg(0x1C);
        uint32_t phoff, shoff;
        file.read(reinterpret_cast<char*>(&phoff), 4);
        file.read(reinterpret_cast<char*>(&shoff), 4);
        
        file.seekg(0x2C);
        uint16_t phentsize, phnum;
        file.read(reinterpret_cast<char*>(&phentsize), 2);
        file.read(reinterpret_cast<char*>(&phnum), 2);
        
        file.seekg(phoff);
        for (int i = 0; i < phnum; ++i) {
            uint32_t ptype, poffset, pvaddr, ppaddr, pfilesz, pmemsz, pflags, palign;
            file.read(reinterpret_cast<char*>(&ptype), 4);
            file.read(reinterpret_cast<char*>(&poffset), 4);
            file.read(reinterpret_cast<char*>(&pvaddr), 4);
            file.read(reinterpret_cast<char*>(&ppaddr), 4);
            file.read(reinterpret_cast<char*>(&pfilesz), 4);
            file.read(reinterpret_cast<char*>(&pmemsz), 4);
            file.read(reinterpret_cast<char*>(&pflags), 4);
            file.read(reinterpret_cast<char*>(&palign), 4);
            
            String seg_name = (ptype == 1) ? "LOAD" : "OTHER";
            info.sections.push_back(seg_name);
            
            if (pflags & 4) info.code_size += pmemsz;
            if (pflags & 2) info.data_size += pmemsz;
        }
    }
    
    detect_engine_hints(info, {});
    info.compiler = detect_compiler(info);
    info.estimated_ram = estimate_ram(info);
    
    return info;
}

Result<ExecutableInfo> ExecutableAnalyzer::analyze_macho(const Path& path) {
    ExecutableInfo info;
    info.platform = "macOS/iOS";
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open Mach-O file: {}", path.string())));
    }
    
    file.seekg(0, std::ios::end);
    info.disk_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), 4);
    
    bool is_64bit = false;
    bool is_swapped = false;
    
    switch (magic) {
        case 0xFEEDFACE: is_64bit = false; break;
        case 0xFEEDFACF: is_64bit = true; break;
        case 0xCEFAEDFE: is_64bit = false; is_swapped = true; break;
        case 0xCFFAEDFE: is_64bit = true; is_swapped = true; break;
        default:
            return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Not a valid Mach-O file"));
    }
    
    info.is_64bit = is_64bit;
    
    uint32_t cputype, cpusubtype, filetype, ncmds, sizeofcmds, flags;
    file.read(reinterpret_cast<char*>(&cputype), 4);
    file.read(reinterpret_cast<char*>(&cpusubtype), 4);
    file.read(reinterpret_cast<char*>(&filetype), 4);
    file.read(reinterpret_cast<char*>(&ncmds), 4);
    file.read(reinterpret_cast<char*>(&sizeofcmds), 4);
    file.read(reinterpret_cast<char*>(&flags), 4);
    
    if (is_swapped) {
        cputype = __builtin_bswap32(cputype);
        cpusubtype = __builtin_bswap32(cpusubtype);
        filetype = __builtin_bswap32(filetype);
        ncmds = __builtin_bswap32(ncmds);
        sizeofcmds = __builtin_bswap32(sizeofcmds);
        flags = __builtin_bswap32(flags);
    }
    
    switch (cputype) {
        case 0x01000007: info.architecture = "x86_64"; break;
        case 0x0100000C: info.architecture = "ARM64"; break;
        case 0x00000007: info.architecture = "x86"; break;
        case 0x0000000C: info.architecture = "ARM"; break;
        case 0x01000016: info.architecture = "RISC-V64"; break;
        default: info.architecture = std::format("Unknown (0x{:08X})", cputype); break;
    }
    
    file.seekg(sizeofcmds, std::ios::cur);
    
    info.estimated_ram = estimate_ram(info);
    
    return info;
}

Result<ExecutableInfo> ExecutableAnalyzer::analyze_generic(const Path& path) {
    ExecutableInfo info;
    info.disk_size = fs::file_size(path);
    info.architecture = "Unknown";
    info.platform = "Unknown";
    info.compiler = "Unknown";
    info.estimated_ram = info.disk_size / 4;
    return info;
}

u64 ExecutableAnalyzer::estimate_ram(const ExecutableInfo& exe) const {
    u64 ram = exe.code_size + exe.data_size + exe.bss_size;
    ram += 64 * MiB;
    ram = (ram + 1023) / 1024 * 1024;
    return ram;
}

void ExecutableAnalyzer::detect_engine_hints(ExecutableInfo& exe, const std::vector<String>& imports) const {
    for (const auto& dll : imports) {
        String lower = StringUtils::to_lower(dll);
        if (lower.find("unity") != String::npos || lower.find("mono") != String::npos) {
            exe.engine_hint = "Unity";
            break;
        }
        if (lower.find("unreal") != String::npos || lower.find("ue4") != String::npos || lower.find("ue5") != String::npos) {
            exe.engine_hint = "Unreal Engine";
            break;
        }
        if (lower.find("godot") != String::npos) {
            exe.engine_hint = "Godot";
            break;
        }
        if (lower.find("cry") != String::npos) {
            exe.engine_hint = "CryEngine";
            break;
        }
        if (lower.find("frostbite") != String::npos) {
            exe.engine_hint = "Frostbite";
            break;
        }
        if (lower.find("source") != String::npos || lower.find("tier0") != String::npos || lower.find("vstdlib") != String::npos) {
            exe.engine_hint = "Source Engine";
            break;
        }
        if (lower.find("anvil") != String::npos) {
            exe.engine_hint = "Anvil (Ubisoft)";
            break;
        }
        if (lower.find("idtech") != String::npos) {
            exe.engine_hint = "id Tech";
            break;
        }
        if (lower.find("creationengine") != String::npos) {
            exe.engine_hint = "Creation Engine (Bethesda)";
            break;
        }
    }
    
    if (exe.engine_hint.empty()) {
        if (imports.size() > 0) {
            exe.engine_hint = "Native/Custom";
        }
    }
}

String ExecutableAnalyzer::detect_compiler(const ExecutableInfo& exe) const {
    if (exe.platform == "Windows") {
        if (exe.is_managed) return "Roslyn / .NET Compiler";
        return "MSVC / Clang-CL / MinGW";
    } else if (exe.platform == "Linux/Unix") {
        return "GCC / Clang";
    } else if (exe.platform == "macOS/iOS") {
        return "Clang (Xcode)";
    }
    return "Unknown";
}

ExecutableStats ExecutableAnalyzer::stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

String ExecutableAnalyzer::generate_report() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    std::stringstream ss;
    ss << "Executable Analysis Report\n";
    ss << "==========================\n";
    ss << "Total Executables: " << stats_.total_executables << "\n";
    ss << "Total Code Size: " << StringUtils::format_bytes(stats_.total_code_size) << "\n";
    ss << "Total Data Size: " << StringUtils::format_bytes(stats_.total_data_size) << "\n";
    ss << "Estimated RAM: " << StringUtils::format_bytes(stats_.total_estimated_ram) << "\n\n";
    
    if (!stats_.architecture_counts.empty()) {
        ss << "Architecture Distribution:\n";
        for (const auto& [arch, count] : stats_.architecture_counts) {
            ss << "  " << arch << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.platform_counts.empty()) {
        ss << "Platform Distribution:\n";
        for (const auto& [plat, count] : stats_.platform_counts) {
            ss << "  " << plat << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.compiler_counts.empty()) {
        ss << "Compiler Distribution:\n";
        for (const auto& [comp, count] : stats_.compiler_counts) {
            ss << "  " << comp << ": " << count << "\n";
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
