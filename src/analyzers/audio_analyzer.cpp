#include <game_req_analyzer/analyzers/audio_analyzer.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>

using namespace game_req;

AudioAnalyzer::AudioAnalyzer(const AnalyzerConfig& config) : config_(config) {}

AudioAnalyzer::~AudioAnalyzer() = default;

Result<std::vector<AudioInfo>> AudioAnalyzer::analyze(const std::vector<FileInfo>& audio_files) {
    std::vector<AudioInfo> results;
    results.reserve(audio_files.size());
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_files = 0;
        stats_.total_duration_ms = 0;
        stats_.total_disk_size = 0;
        stats_.total_estimated_ram = 0;
        stats_.format_counts.clear();
        stats_.codec_counts.clear();
        stats_.sample_rate_counts.clear();
        stats_.max_sample_rate = 0;
        stats_.max_channels = 0;
    }
    
    for (const auto& audio : audio_files) {
        auto result = analyze_single(audio);
        if (result) {
            results.push_back(*result);
            
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_files++;
            stats_.total_duration_ms += result->duration_ms;
            stats_.total_disk_size += result->disk_size;
            stats_.total_estimated_ram += estimate_ram_usage(*result);
            stats_.format_counts[result->format]++;
            stats_.codec_counts[result->codec]++;
            stats_.sample_rate_counts[result->sample_rate]++;
            if (result->sample_rate > stats_.max_sample_rate) stats_.max_sample_rate = result->sample_rate;
            if (result->channels > stats_.max_channels) stats_.max_channels = result->channels;
        } else {
            LOG_WARN("Failed to analyze audio {}: {}", audio.path.string(), result.error().message);
        }
    }
    
    return results;
}

Result<AudioInfo> AudioAnalyzer::analyze_single(const FileInfo& audio) {
    AudioInfo info;
    info.disk_size = audio.size;
    
    try {
        switch (audio.type) {
            case FileType::WAV:
                return analyze_wav(audio.path);
            case FileType::OGG:
                return analyze_ogg(audio.path);
            case FileType::MP3:
                return analyze_mp3(audio.path);
            case FileType::FLAC:
                return analyze_flac(audio.path);
            case FileType::WMA:
                return analyze_wma(audio.path);
            case FileType::AAC:
            case FileType::M4A:
                return analyze_aac(audio.path);
            case FileType::AIFF:
                return analyze_aiff(audio.path);
            case FileType::FSB:
                return analyze_fsb(audio.path);
            case FileType::BNK:
                return analyze_bnk(audio.path);
            case FileType::XWB:
            case FileType::XSB:
                return analyze_xact(audio.path);
            default:
                return analyze_generic(audio.path, audio.type);
        }
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Audio analysis failed: {}", e.what())));
    }
}

Result<AudioInfo> AudioAnalyzer::analyze_wav(const Path& path) {
    AudioInfo info;
    info.format = "WAV";
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open WAV file: {}", path.string())));
    }
    
    char riff[4];
    file.read(riff, 4);
    if (std::memcmp(riff, "RIFF", 4) != 0) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Not a RIFF/WAV file"));
    }
    
    file.seekg(4, std::ios::cur);
    
    char wave[4];
    file.read(wave, 4);
    if (std::memcmp(wave, "WAVE", 4) != 0) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Not a WAVE file"));
    }
    
    while (!file.eof()) {
        char chunk_id[4];
        uint32_t chunk_size;
        
        file.read(chunk_id, 4);
        if (!file) break;
        file.read(reinterpret_cast<char*>(&chunk_size), 4);
        if (!file) break;
        
        if (std::memcmp(chunk_id, "fmt ", 4) == 0) {
            uint16_t audio_format;
            file.read(reinterpret_cast<char*>(&audio_format), 2);
            file.read(reinterpret_cast<char*>(&info.channels), 2);
            file.read(reinterpret_cast<char*>(&info.sample_rate), 4);
            file.seekg(6, std::ios::cur);
            file.read(reinterpret_cast<char*>(&info.bits_per_sample), 2);
            
            switch (audio_format) {
                case 1: info.codec = "PCM"; break;
                case 3: info.codec = "IEEE Float"; break;
                case 6: info.codec = "A-Law"; break;
                case 7: info.codec = "Mu-Law"; break;
                case 0xFFFE: info.codec = "Extensible"; break;
                default: info.codec = std::format("Format 0x{:04X}", audio_format); break;
            }
        } else if (std::memcmp(chunk_id, "data", 4) == 0) {
            info.sample_count = chunk_size / (info.channels * info.bits_per_sample / 8);
            if (info.sample_rate > 0) {
                info.duration_ms = (info.sample_count * 1000) / info.sample_rate;
            }
            break;
        } else {
            file.seekg(chunk_size, std::ios::cur);
            if (chunk_size % 2 != 0) file.seekg(1, std::ios::cur);
        }
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    info.is_streaming = detect_streaming(info, path);
    
    return info;
}

Result<AudioInfo> AudioAnalyzer::analyze_ogg(const Path& path) {
    AudioInfo info;
    info.format = "OGG";
    info.codec = "Vorbis";
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open OGG file: {}", path.string())));
    }
    
    char header[27];
    file.read(header, 27);
    
    if (std::memcmp(header, "OggS", 4) != 0) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Not an OGG file"));
    }
    
    file.seekg(0, std::ios::beg);
    
    while (!file.eof()) {
        char page_header[27];
        file.read(page_header, 27);
        if (!file || std::memcmp(page_header, "OggS", 4) != 0) break;
        
        uint8_t segment_count = page_header[26];
        std::vector<uint8_t> segment_table(segment_count);
        file.read(reinterpret_cast<char*>(segment_table.data()), segment_count);
        
        uint32_t page_size = 0;
        for (uint8_t seg : segment_table) page_size += seg;
        
        if (page_size > 0) {
            std::vector<char> page_data(page_size);
            file.read(page_data.data(), page_size);
            
            if (page_data.size() >= 7 && std::memcmp(page_data.data(), "\x01vorbis", 7) == 0) {
                if (page_data.size() >= 28) {
                    info.channels = page_data[11];
                    info.sample_rate = *reinterpret_cast<uint32_t*>(&page_data[12]);
                    info.bits_per_sample = 16;
                    
                    uint32_t total_samples = *reinterpret_cast<uint32_t*>(&page_data[24]);
                    if (info.sample_rate > 0) {
                        info.duration_ms = (total_samples * 1000) / info.sample_rate;
                    }
                    info.sample_count = total_samples * info.channels;
                }
                break;
            }
        }
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    info.is_streaming = detect_streaming(info, path);
    
    return info;
}

Result<AudioInfo> AudioAnalyzer::analyze_mp3(const Path& path) {
    AudioInfo info;
    info.format = "MP3";
    info.codec = "MPEG Layer 3";
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open MP3 file: {}", path.string())));
    }
    
    uint64_t file_size = fs::file_size(path);
    uint64_t audio_data_size = file_size;
    
    while (!file.eof() && file.tellg() < static_cast<std::streampos>(file_size - 10)) {
        uint8_t byte1 = file.get();
        if (file.eof()) break;
        uint8_t byte2 = file.get();
        
        if (byte1 == 0xFF && (byte2 & 0xE0) == 0xE0) {
            uint8_t byte3 = file.get();
            uint8_t byte4 = file.get();
            
            uint8_t version = (byte3 >> 3) & 0x03;
            uint8_t layer = (byte3 >> 1) & 0x03;
            uint8_t bitrate_index = (byte3 >> 4) & 0x0F;
            uint8_t sample_rate_index = (byte3 >> 2) & 0x03;
            uint8_t padding = (byte3 >> 1) & 0x01;
            uint8_t mode = (byte4 >> 6) & 0x03;
            
            static const int bitrate_table[2][16] = {
                {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1},
                {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, -1}
            };
            
            static const int sample_rate_table[3] = {44100, 48000, 32000};
            
            int bitrate = bitrate_table[version == 3 ? 0 : 1][bitrate_index];
            int sample_rate = sample_rate_table[sample_rate_index];
            
            if (bitrate > 0 && sample_rate > 0) {
                info.sample_rate = sample_rate;
                info.bits_per_sample = 16;
                info.channels = (mode == 3) ? 1 : 2;
                
                int frame_size = (version == 3) ? 
                    (144 * bitrate * 1000 / sample_rate + padding) :
                    (72 * bitrate * 1000 / sample_rate + padding);
                
                if (frame_size > 0) {
                    int num_frames = audio_data_size / frame_size;
                    info.sample_count = num_frames * 1152 * info.channels;
                    info.duration_ms = (info.sample_count * 1000) / (info.sample_rate * info.channels);
                }
                break;
            }
        }
        file.seekg(-1, std::ios::cur);
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    info.is_streaming = detect_streaming(info, path);
    
    return info;
}

Result<AudioInfo> AudioAnalyzer::analyze_flac(const Path& path) {
    AudioInfo info;
    info.format = "FLAC";
    info.codec = "FLAC";
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Cannot open FLAC file: {}", path.string())));
    }
    
    char header[4];
    file.read(header, 4);
    if (std::memcmp(header, "fLaC", 4) != 0) {
        return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Not a FLAC file"));
    }
    
    while (!file.eof()) {
        uint8_t block_header;
        file.read(reinterpret_cast<char*>(&block_header), 1);
        if (!file) break;
        
        bool last_block = (block_header & 0x80) != 0;
        uint8_t block_type = block_header & 0x7F;
        
        uint8_t size_bytes[3];
        file.read(reinterpret_cast<char*>(size_bytes), 3);
        uint32_t block_size = (size_bytes[0] << 16) | (size_bytes[1] << 8) | size_bytes[2];
        
        if (block_type == 0) {
            std::vector<char> streaminfo(block_size);
            file.read(streaminfo.data(), block_size);
            
            if (block_size >= 18) {
                info.sample_rate = (static_cast<uint32_t>(streaminfo[10] & 0x0F) << 16) |
                                   (static_cast<uint32_t>(streaminfo[11]) << 8) |
                                   static_cast<uint32_t>(streaminfo[12]);
                info.channels = ((streaminfo[12] >> 1) & 0x07) + 1;
                info.bits_per_sample = ((streaminfo[12] & 0x01) << 4) | ((streaminfo[13] >> 4) & 0x0F) + 1;
                
                uint64_t total_samples = 0;
                for (int i = 0; i < 8; ++i) {
                    total_samples = (total_samples << 8) | static_cast<uint8_t>(streaminfo[14 + i]);
                }
                info.sample_count = total_samples * info.channels;
                if (info.sample_rate > 0) {
                    info.duration_ms = (total_samples * 1000) / info.sample_rate;
                }
            }
        } else {
            file.seekg(block_size, std::ios::cur);
        }
        
        if (last_block) break;
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    info.is_streaming = detect_streaming(info, path);
    
    return info;
}

Result<AudioInfo> AudioAnalyzer::analyze_wma(const Path& path) {
    AudioInfo info;
    info.format = "WMA";
    info.codec = "Windows Media Audio";
    info.disk_size = fs::file_size(path);
    info.estimated_ram = info.disk_size * 4;
    info.is_streaming = true;
    return info;
}

Result<AudioInfo> AudioAnalyzer::analyze_aac(const Path& path) {
    AudioInfo info;
    info.format = "AAC/M4A";
    info.codec = "AAC";
    info.disk_size = fs::file_size(path);
    info.estimated_ram = info.disk_size * 4;
    info.is_streaming = true;
    return info;
}

Result<AudioInfo> AudioAnalyzer::analyze_aiff(const Path& path) {
    AudioInfo info;
    info.format = "AIFF";
    info.codec = "PCM";
    info.disk_size = fs::file_size(path);
    info.estimated_ram = estimate_ram_usage(info);
    info.is_streaming = detect_streaming(info, path);
    return info;
}

Result<AudioInfo> AudioAnalyzer::analyze_fsb(const Path& path) {
    AudioInfo info;
    info.format = "FSB";
    info.codec = "FMOD Sample Bank";
    info.is_streaming = true;
    info.disk_size = fs::file_size(path);
    info.estimated_ram = info.disk_size;
    return info;
}

Result<AudioInfo> AudioAnalyzer::analyze_bnk(const Path& path) {
    AudioInfo info;
    info.format = "BNK";
    info.codec = "Wwise SoundBank";
    info.is_streaming = true;
    info.disk_size = fs::file_size(path);
    info.estimated_ram = info.disk_size;
    return info;
}

Result<AudioInfo> AudioAnalyzer::analyze_xact(const Path& path) {
    AudioInfo info;
    info.format = "XACT";
    info.codec = "XACT Wave/Sound Bank";
    info.is_streaming = true;
    info.disk_size = fs::file_size(path);
    info.estimated_ram = info.disk_size;
    return info;
}

Result<AudioInfo> AudioAnalyzer::analyze_generic(const Path& path, FileType type) {
    AudioInfo info;
    info.disk_size = fs::file_size(path);
    info.format = StringUtils::to_upper(path.extension().string().substr(1));
    info.codec = "Unknown";
    info.is_streaming = info.disk_size > 5 * MiB;
    info.estimated_ram = info.is_streaming ? info.disk_size / 4 : info.disk_size;
    return info;
}

u64 AudioAnalyzer::estimate_ram_usage(const AudioInfo& audio) const {
    if (audio.is_streaming) {
        return 64 * MiB;
    }
    
    if (audio.sample_count > 0 && audio.bits_per_sample > 0 && audio.channels > 0) {
        return (audio.sample_count * audio.bits_per_sample / 8);
    }
    
    if (audio.duration_ms > 0 && audio.sample_rate > 0 && audio.channels > 0) {
        uint64_t samples = (audio.duration_ms * audio.sample_rate * audio.channels) / 1000;
        return samples * (audio.bits_per_sample > 0 ? audio.bits_per_sample / 8 : 2);
    }
    
    return audio.disk_size;
}

bool AudioAnalyzer::detect_streaming(const AudioInfo& audio, const Path& path) const {
    if (audio.disk_size > 10 * MiB) return true;
    if (audio.duration_ms > 60000) return true;
    
    String ext = StringUtils::to_lower(path.extension().string());
    static const std::unordered_set<String> streaming_formats = {
        ".fsb", ".bnk", ".xwb", ".xsb", ".pkg", ".wav", ".ogg"
    };
    
    return streaming_formats.find(ext) != streaming_formats.end();
}

AudioStats AudioAnalyzer::stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

String AudioAnalyzer::generate_report() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    std::stringstream ss;
    ss << "Audio Analysis Report\n";
    ss << "====================\n";
    ss << "Total Files: " << stats_.total_files << "\n";
    ss << "Total Duration: " << (stats_.total_duration_ms / 1000) << "s\n";
    ss << "Total Disk Size: " << StringUtils::format_bytes(stats_.total_disk_size) << "\n";
    ss << "Estimated RAM: " << StringUtils::format_bytes(stats_.total_estimated_ram) << "\n";
    ss << "Max Sample Rate: " << stats_.max_sample_rate << " Hz\n";
    ss << "Max Channels: " << stats_.max_channels << "\n\n";
    
    if (!stats_.format_counts.empty()) {
        ss << "Format Distribution:\n";
        for (const auto& [fmt, count] : stats_.format_counts) {
            ss << "  " << fmt << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.codec_counts.empty()) {
        ss << "Codec Distribution:\n";
        for (const auto& [codec, count] : stats_.codec_counts) {
            ss << "  " << codec << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    return ss.str();
}
