#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>

namespace game_req {

struct NetworkInfo {
    u64 connection_count = 0;
    u64 message_type_count = 0;
    u64 channel_count = 0;
    u64 disk_size = 0;
    u64 estimated_ram = 0;
    u64 estimated_bandwidth = 0; // bytes/sec
    String format;                // Photon, Mirror, Netcode, Steam, custom, etc.
    String engine_hint;
    String network_library;
    std::vector<String> protocols;      // TCP, UDP, WebSocket, WebRTC, QUIC
    std::vector<String> serialization;  // Protobuf, MessagePack, JSON, custom binary
    std::vector<String> features;       // Reliable, Unreliable, Sequenced, Fragmented, Encrypted, Compressed
    u32 max_players = 0;
    u32 tick_rate = 0;
    bool has_authority = false;
    bool has_prediction = false;
    bool has_interpolation = false;
    bool has_lag_compensation = false;
    bool has_voice_chat = false;
    bool has_matchmaking = false;
    bool has_relay = false;
    bool has_nat_punchthrough = false;
};

struct NetworkStats {
    u64 total_systems = 0;
    u64 total_connections = 0;
    u64 total_message_types = 0;
    u64 total_disk_size = 0;
    u64 total_estimated_ram = 0;
    u64 total_estimated_bandwidth = 0;
    std::unordered_map<String, u32> format_counts;
    std::unordered_map<String, u32> engine_counts;
    std::unordered_map<String, u32> library_counts;
    std::unordered_map<String, u32> protocol_counts;
    std::unordered_map<String, u32> serialization_counts;
    u32 max_players_per_game = 0;
    u32 max_tick_rate = 0;
    
    [[nodiscard]] String summary() const;
};

class NetworkAnalyzer {
public:
    explicit NetworkAnalyzer(const AnalyzerConfig& config);
    
    Result<std::vector<NetworkInfo>> analyze(const std::vector<FileInfo>& network_files);
    Result<NetworkInfo> analyze_single(const FileInfo& network);
    [[nodiscard]] NetworkStats stats() const { return stats_; }
    [[nodiscard]] String generate_report() const;

private:
    AnalyzerConfig config_;
    NetworkStats stats_;
    mutable std::mutex stats_mutex_;
    
    Result<NetworkInfo> analyze_photon(const Path& path);
    Result<NetworkInfo> analyze_mirror(const Path& path);
    Result<NetworkInfo> analyze_netcode(const Path& path);
    Result<NetworkInfo> analyze_steam(const Path& path);
    Result<NetworkInfo> analyze_epic(const Path& path);
    Result<NetworkInfo> analyze_generic(const Path& path, FileType type);
    
    String read_file_content(const Path& path) const;
    u64 estimate_ram_usage(const NetworkInfo& network) const;
    u64 estimate_bandwidth(const NetworkInfo& network) const;
    void detect_network_library(NetworkInfo& network, StringView content) const;
};

} // namespace game_req