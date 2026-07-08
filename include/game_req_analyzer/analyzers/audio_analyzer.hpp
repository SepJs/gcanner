#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>

namespace game_req {

struct AudioInfo {
    u64 sample_count = 0;
    u32 sample_rate = 0;
    u16 channels = 0;
    u16 bits_per_sample = 0;
    u64 duration_ms = 0;
    u64 disk_size = 0;
    u64 estimated_ram = 0;
    String format;
    String codec;
    bool is_streaming = false;
    bool is_looped = false;
    String language;
    std::vector<String> tags;
    
    struct LoopInfo {
        u64 start_sample = 0;
        u64 end_sample = 0;
        u32 loop_count = 0;
    };
    Opt<LoopInfo> loop_info;
};

struct AudioStats {
    u64 total_files = 0;
    u64 total_duration_ms = 0;
    u64 total_disk_size = 0;
    u64 total_estimated_ram = 0;
    std::unordered_map<String, u32> format_counts;
    std::unordered_map<String, u32> codec_counts;
    std::unordered_map<u32, u32> sample_rate_counts;
    u32 max_sample_rate = 0;
    u32 max_channels = 0;
    
    [[nodiscard]] String summary() const;
};

class AudioAnalyzer {
public:
    explicit AudioAnalyzer(const AnalyzerConfig& config);
    
    Result<std::vector<AudioInfo>> analyze(const std::vector<FileInfo>& audio_files);
    Result<AudioInfo> analyze_single(const FileInfo& audio);
    [[nodiscard]] AudioStats stats() const { return stats_; }
    [[nodiscard]] String generate_report() const;

private:
    AnalyzerConfig config_;
    AudioStats stats_;
    std::mutex stats_mutex_;
    
    Result<AudioInfo> analyze_wav(const Path& path);
    Result<AudioInfo> analyze_ogg(const Path& path);
    Result<AudioInfo> analyze_mp3(const Path& path);
    Result<AudioInfo> analyze_flac(const Path& path);
    Result<AudioInfo> analyze_wma(const Path& path);
    Result<AudioInfo> analyze_adpcm(const Path& path);
    Result<AudioInfo> analyze_xma(const Path& path);
    Result<AudioInfo> analyze_atrac(const Path& path);
    Result<AudioInfo> analyze_generic(const Path& path, FileType type);
    
    u64 estimate_ram_usage(const AudioInfo& audio) const;
    bool detect_streaming(const AudioInfo& audio, const Path& path) const;
};

} // namespace game_req