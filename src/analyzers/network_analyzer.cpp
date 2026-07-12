#include <game_req_analyzer/analyzers/network_analyzer.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <algorithm>
#include <fstream>
#include <regex>

namespace game_req {

NetworkAnalyzer::NetworkAnalyzer(const AnalyzerConfig& config) : config_(config) {}

Result<std::vector<NetworkInfo>> NetworkAnalyzer::analyze(const std::vector<FileInfo>& network_files) {
    std::vector<NetworkInfo> results;
    results.reserve(network_files.size());
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_systems = 0;
        stats_.total_connections = 0;
        stats_.total_message_types = 0;
        stats_.total_disk_size = 0;
        stats_.total_estimated_ram = 0;
        stats_.total_estimated_bandwidth = 0;
        stats_.format_counts.clear();
        stats_.engine_counts.clear();
        stats_.library_counts.clear();
        stats_.protocol_counts.clear();
        stats_.serialization_counts.clear();
        stats_.max_players_per_game = 0;
        stats_.max_tick_rate = 0;
    }
    
    for (const auto& network : network_files) {
        auto result = analyze_single(network);
        if (result) {
            results.push_back(*result);
            
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_systems++;
            stats_.total_connections += result->connection_count;
            stats_.total_message_types += result->message_type_count;
            stats_.total_disk_size += result->disk_size;
            stats_.total_estimated_ram += result->estimated_ram;
            stats_.total_estimated_bandwidth += result->estimated_bandwidth;
            stats_.format_counts[result->format]++;
            if (!result->engine_hint.empty()) stats_.engine_counts[result->engine_hint]++;
            if (!result->network_library.empty()) stats_.library_counts[result->network_library]++;
            for (const auto& proto : result->protocols) stats_.protocol_counts[proto]++;
            for (const auto& ser : result->serialization) stats_.serialization_counts[ser]++;
            if (result->max_players > stats_.max_players_per_game) {
                stats_.max_players_per_game = result->max_players;
            }
            if (result->tick_rate > stats_.max_tick_rate) {
                stats_.max_tick_rate = result->tick_rate;
            }
        } else {
            LOG_WARN("Failed to analyze network file {}: {}", network.path.string(), result.error().message);
        }
    }
    
    return results;
}

Result<NetworkInfo> NetworkAnalyzer::analyze_single(const FileInfo& network) {
    NetworkInfo info;
    info.disk_size = network.size;
    
    try {
        // Network files are usually scripts or config files
        String content = read_file_content(network.path);
        if (!content.empty()) {
            detect_network_library(info, content);
        }
        
        // Default estimates
        info.connection_count = 10;
        info.message_type_count = 20;
        info.channel_count = 4;
        info.max_players = 16;
        info.tick_rate = 20;
        
        info.estimated_ram = estimate_ram_usage(info);
        info.estimated_bandwidth = estimate_bandwidth(info);
        
        return info;
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("Network analysis failed: {}", e.what())));
    }
}

String NetworkAnalyzer::read_file_content(const Path& path) const {
    std::ifstream file(path);
    if (!file) return "";
    std::string content;
    file.seekg(0, std::ios::end);
    content.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return content;
}

Result<NetworkInfo> NetworkAnalyzer::analyze_photon(const Path& path) {
    NetworkInfo info;
    info.format = "Photon";
    info.network_library = "Photon";
    info.disk_size = fs::file_size(path);
    
    String content = read_file_content(path);
    if (!content.empty()) {
        // Photon-specific patterns
        if (content.find("PhotonNetwork") != String::npos || content.find("Photon.Pun") != String::npos) {
            info.engine_hint = "Unity (Photon PUN)";
        }
        
        std::regex connect_regex(R"(PhotonNetwork\.Connect|PhotonNetwork\.JoinRoom|OpJoinRoom)");
        info.connection_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), connect_regex),
            std::sregex_iterator()
        );
        
        std::regex rpc_regex(R"(\[PunRPC\]|\[Rpc\]|photonView\.RPC)");
        info.message_type_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), rpc_regex),
            std::sregex_iterator()
        );
        
        info.protocols.push_back("UDP");
        info.protocols.push_back("TCP");
        info.serialization.push_back("Photon Binary Protocol");
        info.features.push_back("Reliable");
        info.features.push_back("Unreliable");
        info.features.push_back("Sequenced");
        
        // Photon PUN defaults
        info.max_players = std::max(info.max_players, 20u);
        info.tick_rate = std::max(info.tick_rate, 10u);
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    info.estimated_bandwidth = estimate_bandwidth(info);
    return info;
}

Result<NetworkInfo> NetworkAnalyzer::analyze_mirror(const Path& path) {
    NetworkInfo info;
    info.format = "Mirror";
    info.network_library = "Mirror";
    info.disk_size = fs::file_size(path);
    
    String content = read_file_content(path);
    if (!content.empty()) {
        if (content.find("Mirror") != String::npos && content.find("NetworkBehaviour") != String::npos) {
            info.engine_hint = "Unity (Mirror)";
        }
        
        std::regex cmd_regex(R"(\[Command\]|\[Client\]|\[Command\(channel)");
        info.message_type_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), cmd_regex),
            std::sregex_iterator()
        );
        
        std::regex rpc_regex(R"(\[ClientRpc\]|\[TargetRpc\]|\[ObserversRpc\])");
        info.message_type_count += std::distance(
            std::sregex_iterator(content.begin(), content.end(), rpc_regex),
            std::sregex_iterator()
        );
        
        std::regex sync_var_regex(R"(\[SyncVar\]|\[SyncVar\(hook)");
        info.message_type_count += std::distance(
            std::sregex_iterator(content.begin(), content.end(), sync_var_regex),
            std::sregex_iterator()
        );
        
        info.protocols.push_back("TCP");
        info.protocols.push_back("UDP (via KCP)");
        info.serialization.push_back("Mirror Binary");
        info.features.push_back("Reliable (TCP)");
        info.features.push_back("Unreliable (KCP)");
        info.features.push_back("Sequenced");
        info.features.push_back("State Sync (SyncVars)");
        
        info.has_authority = content.find("isServer") != String::npos || content.find("hasAuthority") != String::npos;
        info.has_prediction = content.find("NetworkClient") != String::npos && content.find("prediction") != String::npos;
        info.has_interpolation = content.find("NetworkTransform") != String::npos;
        
        info.max_players = std::max(info.max_players, 100u); // Mirror can handle more
        info.tick_rate = std::max(info.tick_rate, 30u);
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    info.estimated_bandwidth = estimate_bandwidth(info);
    return info;
}

Result<NetworkInfo> NetworkAnalyzer::analyze_netcode(const Path& path) {
    NetworkInfo info;
    info.format = "Netcode for GameObjects";
    info.network_library = "Netcode";
    info.disk_size = fs::file_size(path);
    
    String content = read_file_content(path);
    if (!content.empty()) {
        if (content.find("Unity.Netcode") != String::npos || content.find("NetworkBehaviour") != String::npos) {
            info.engine_hint = "Unity (Netcode for GameObjects)";
        }
        
        std::regex rpc_regex(R"(\[Rpc\]|\[ServerRpc\]|\[ClientRpc\]|\[ObserversRpc\])");
        info.message_type_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), rpc_regex),
            std::sregex_iterator()
        );
        
        std::regex netvar_regex(R"(NetworkVariable<|NetworkList<)");
        info.message_type_count += std::distance(
            std::sregex_iterator(content.begin(), content.end(), netvar_regex),
            std::sregex_iterator()
        );
        
        info.protocols.push_back("UDP");
        info.serialization.push_back("Netcode Binary");
        info.features.push_back("Reliable");
        info.features.push_back("Unreliable");
        info.features.push_back("State Sync (NetworkVariables)");
        info.features.push_back("NetworkLists");
        
        info.has_authority = content.find("IsServer") != String::npos || content.find("IsOwner") != String::npos;
        info.has_prediction = content.find("NetworkPredicted") != String::npos;
        info.has_interpolation = content.find("NetworkTransform") != String::npos;
        
        info.max_players = std::max(info.max_players, 64u);
        info.tick_rate = std::max(info.tick_rate, 60u);
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    info.estimated_bandwidth = estimate_bandwidth(info);
    return info;
}

Result<NetworkInfo> NetworkAnalyzer::analyze_steam(const Path& path) {
    NetworkInfo info;
    info.format = "Steam Networking";
    info.network_library = "SteamNetworkingSockets";
    info.disk_size = fs::file_size(path);
    
    String content = read_file_content(path);
    if (!content.empty()) {
        if (content.find("SteamNetworkingSockets") != String::npos || content.find("ISteamNetworking") != String::npos) {
            info.engine_hint = "Steamworks";
        }
        
        info.protocols.push_back("UDP (Steam Relay)");
        info.protocols.push_back("TCP (Steam Relay)");
        info.serialization.push_back("Custom Binary");
        info.features.push_back("Encrypted");
        info.features.push_back("Relay");
        info.features.push_back("NAT Punchthrough");
        info.features.push_back("Connection Migration");
        
        info.has_relay = true;
        info.has_nat_punchthrough = true;
        info.has_matchmaking = true;
        
        info.max_players = std::max(info.max_players, 100u);
        info.tick_rate = std::max(info.tick_rate, 60u);
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    info.estimated_bandwidth = estimate_bandwidth(info);
    return info;
}

Result<NetworkInfo> NetworkAnalyzer::analyze_epic(const Path& path) {
    NetworkInfo info;
    info.format = "Epic Online Services (EOS)";
    info.network_library = "EOS";
    info.disk_size = fs::file_size(path);
    
    String content = read_file_content(path);
    if (!content.empty()) {
        if (content.find("EOS_") != String::npos || content.find("EpicOnlineServices") != String::npos) {
            info.engine_hint = "Unreal Engine (EOS)";
        }
        
        info.protocols.push_back("UDP (EOS Relay)");
        info.serialization.push_back("Custom Binary");
        info.features.push_back("Encrypted");
        info.features.push_back("Relay");
        info.features.push_back("NAT Punchthrough");
        info.features.push_back("Voice Chat (EOS Voice)");
        info.features.push_back("Matchmaking (EOS Matchmaking)");
        
        info.has_relay = true;
        info.has_nat_punchthrough = true;
        info.has_matchmaking = true;
        info.has_voice_chat = true;
        
        info.max_players = std::max(info.max_players, 100u);
        info.tick_rate = std::max(info.tick_rate, 60u);
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    info.estimated_bandwidth = estimate_bandwidth(info);
    return info;
}

Result<NetworkInfo> NetworkAnalyzer::analyze_generic(const Path& path, FileType type) {
    NetworkInfo info;
    info.disk_size = fs::file_size(path);
    info.format = "Custom/Unknown";
    
    String content = read_file_content(path);
    if (!content.empty()) {
        detect_network_library(info, content);
        
        // Generic network patterns
        std::regex socket_regex(R"(socket\(|Socket\(|WSASocket|CreateSocket)");
        info.connection_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), socket_regex),
            std::sregex_iterator()
        );
        
        std::regex send_regex(R"(send\(|sendto\(|WritePacket|SendPacket)");
        info.message_type_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), send_regex),
            std::sregex_iterator()
        );
        
        // Detect protocols
        if (content.find("TCP") != String::npos) info.protocols.push_back("TCP");
        if (content.find("UDP") != String::npos) info.protocols.push_back("UDP");
        if (content.find("WebSocket") != String::npos || content.find("websocket") != String::npos) {
            info.protocols.push_back("WebSocket");
        }
        if (content.find("QUIC") != String::npos || content.find("quic") != String::npos) {
            info.protocols.push_back("QUIC");
        }
        if (content.find("WebRTC") != String::npos || content.find("webrtc") != String::npos) {
            info.protocols.push_back("WebRTC");
        }
        
        // Detect serialization
        if (content.find("protobuf") != String::npos || content.find("Protobuf") != String::npos) {
            info.serialization.push_back("Protocol Buffers");
        }
        if (content.find("MessagePack") != String::npos || content.find("msgpack") != String::npos) {
            info.serialization.push_back("MessagePack");
        }
        if (content.find("JSON") != String::npos || content.find("json") != String::npos) {
            info.serialization.push_back("JSON");
        }
        if (content.find("FlatBuffers") != String::npos || content.find("flatbuffers") != String::npos) {
            info.serialization.push_back("FlatBuffers");
        }
        if (content.find("Bond") != String::npos) {
            info.serialization.push_back("Microsoft Bond");
        }
        
        // Default features
        if (info.protocols.empty()) {
            info.protocols.push_back("TCP");
            info.protocols.push_back("UDP");
        }
        if (info.serialization.empty()) {
            info.serialization.push_back("Custom Binary");
        }
        info.features.push_back("Reliable");
        info.features.push_back("Unreliable");
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    info.estimated_bandwidth = estimate_bandwidth(info);
    return info;
}

u64 NetworkAnalyzer::estimate_ram_usage(const NetworkInfo& network) const {
    // Network system RAM usage:
    // - Connection state: ~1-10 KB per connection
    // - Message buffers: ~4-64 KB per connection
    // - Serialization buffers: ~1-16 KB
    // - Packet queues: ~10-100 KB per connection
    // - Crypto context: ~1-4 KB per connection
    // - Statistics/telemetry: ~1 KB per connection
    // - Global state: ~100 KB - 1 MB
    
    u64 ram = 0;
    ram += network.connection_count * 32 * 1024; // 32 KB per connection (buffers, queues, state)
    ram += network.channel_count * 8 * 1024;      // 8 KB per channel
    ram += network.message_type_count * 256;      // 256 bytes per message type (metadata)
    ram += 1024 * 1024;                           // 1 MB global state
    
    // Serialization overhead
    if (!network.serialization.empty()) {
        ram += 512 * 1024; // 512 KB for serialization buffers
    }
    
    // Encryption overhead
    if (std::find(network.features.begin(), network.features.end(), "Encrypted") != network.features.end()) {
        ram += network.connection_count * 4 * 1024; // 4 KB per connection for crypto
    }
    
    // Voice chat
    if (network.has_voice_chat) {
        ram += 4 * 1024 * 1024; // 4 MB for voice buffers
    }
    
    return std::max<u64>(ram, 2 * 1024 * 1024); // Min 2 MB
}

u64 NetworkAnalyzer::estimate_bandwidth(const NetworkInfo& network) const {
    // Estimate bandwidth in bytes/sec
    // Base: message types * avg message size * tick rate * players
    u64 bandwidth = 0;
    
    u64 avg_message_size = 256; // bytes
    if (!network.serialization.empty()) {
        if (std::find(network.serialization.begin(), network.serialization.end(), "JSON") != network.serialization.end()) {
            avg_message_size = 512; // JSON is more verbose
        } else if (std::find(network.serialization.begin(), network.serialization.end(), "Protocol Buffers") != network.serialization.end()) {
            avg_message_size = 128; // Protobuf is compact
        }
    }
    
    bandwidth = network.message_type_count * avg_message_size * network.tick_rate * network.max_players;
    
    // Add overhead for reliable messages (ACK, retransmission)
    if (std::find(network.features.begin(), network.features.end(), "Reliable") != network.features.end()) {
        bandwidth = bandwidth * 120 / 100; // 20% overhead
    }
    
    // Voice chat
    if (network.has_voice_chat) {
        bandwidth += network.max_players * 32000; // 32 kbps per player Opus
    }
    
    // Relay overhead
    if (network.has_relay) {
        bandwidth = bandwidth * 110 / 100; // 10% overhead
    }
    
    return bandwidth;
}

void NetworkAnalyzer::detect_network_library(NetworkInfo& network, StringView content) const {
    if (content.find("PhotonNetwork") != String::npos || content.find("Photon.Pun") != String::npos) {
        network.network_library = "Photon";
        network.format = "Photon";
    } else if (content.find("Mirror") != String::npos && content.find("NetworkBehaviour") != String::npos) {
        network.network_library = "Mirror";
        network.format = "Mirror";
    } else if (content.find("Unity.Netcode") != String::npos) {
        network.network_library = "Netcode";
        network.format = "Netcode for GameObjects";
    } else if (content.find("SteamNetworkingSockets") != String::npos || content.find("ISteamNetworking") != String::npos) {
        network.network_library = "SteamNetworkingSockets";
        network.format = "Steam Networking";
    } else if (content.find("EOS_") != String::npos || content.find("EpicOnlineServices") != String::npos) {
        network.network_library = "EOS";
        network.format = "Epic Online Services";
    } else if (content.find("RakNet") != String::npos || content.find("RakPeer") != String::npos) {
        network.network_library = "RakNet";
        network.format = "RakNet";
    } else if (content.find("ENet") != String::npos || content.find("enet_") != String::npos) {
        network.network_library = "ENet";
        network.format = "ENet";
    } else if (content.find("LiteNetLib") != String::npos || content.find("NetPeer") != String::npos) {
        network.network_library = "LiteNetLib";
        network.format = "LiteNetLib";
    } else if (content.find("Lidgren") != String::npos) {
        network.network_library = "Lidgren";
        network.format = "Lidgren.Network";
    } else if (content.find("ZeroMQ") != String::npos || content.find("zmq") != String::npos) {
        network.network_library = "ZeroMQ";
        network.format = "ZeroMQ";
    } else if (content.find("gRPC") != String::npos || content.find("grpc") != String::npos) {
        network.network_library = "gRPC";
        network.format = "gRPC";
    }
}

String NetworkAnalyzer::generate_report() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    std::stringstream ss;
    ss << "Network Analysis Report\n";
    ss << "======================\n";
    ss << "Total Network Systems: " << stats_.total_systems << "\n";
    ss << "Total Connections: " << StringUtils::format_number(stats_.total_connections) << "\n";
    ss << "Total Message Types: " << StringUtils::format_number(stats_.total_message_types) << "\n";
    ss << "Total Disk Size: " << StringUtils::format_bytes(stats_.total_disk_size) << "\n";
    ss << "Estimated RAM: " << StringUtils::format_bytes(stats_.total_estimated_ram) << "\n";
    ss << "Estimated Bandwidth: " << StringUtils::format_bytes(stats_.total_estimated_bandwidth) << "/sec\n";
    ss << "Max Players/Game: " << stats_.max_players_per_game << "\n";
    ss << "Max Tick Rate: " << stats_.max_tick_rate << " Hz\n\n";
    
    if (!stats_.format_counts.empty()) {
        ss << "Format Distribution:\n";
        for (const auto& [fmt, count] : stats_.format_counts) {
            ss << "  " << fmt << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.library_counts.empty()) {
        ss << "Network Library Detection:\n";
        for (const auto& [lib, count] : stats_.library_counts) {
            ss << "  " << lib << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.protocol_counts.empty()) {
        ss << "Protocol Distribution:\n";
        for (const auto& [proto, count] : stats_.protocol_counts) {
            ss << "  " << proto << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.serialization_counts.empty()) {
        ss << "Serialization Distribution:\n";
        for (const auto& [ser, count] : stats_.serialization_counts) {
            ss << "  " << ser << ": " << count << "\n";
        }
    }
    
    return ss.str();
}

String NetworkStats::summary() const {
    std::stringstream ss;
    ss << "Network Systems: " << total_systems 
       << ", Connections: " << StringUtils::format_number(total_connections)
       << ", Message Types: " << StringUtils::format_number(total_message_types)
       << ", Disk: " << StringUtils::format_bytes(total_disk_size)
       << ", RAM: " << StringUtils::format_bytes(total_estimated_ram)
       << ", Bandwidth: " << StringUtils::format_bytes(total_estimated_bandwidth) << "/sec";
    return ss.str();
}

} // namespace game_req