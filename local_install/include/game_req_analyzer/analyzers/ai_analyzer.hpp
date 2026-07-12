#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>

namespace game_req {

struct AIInfo {
    u64 agent_count = 0;
    u64 behavior_tree_nodes = 0;
    u64 state_machine_states = 0;
    u64 navigation_meshes = 0;
    u64 pathfinding_queries = 0;
    u64 disk_size = 0;
    u64 estimated_ram = 0;
    String format;           // NavMesh, BehaviorTree, StateMachine, HTN, GOAP, Utility, custom
    String engine_hint;
    String ai_system;        // Recast/Detour, A* Pathfinding Project, Behavior Designer, NodeCanvas, RAIN, custom
    std::vector<String> ai_techniques;  // FSM, BT, HTN, GOAP, Utility, NeuralNet, etc.
    u32 max_agents_per_scene = 0;
    bool has_navmesh = false;
    bool has_pathfinding = false;
    bool has_behavior_trees = false;
    bool has_state_machines = false;
    bool has_htn = false;
    bool has_goap = false;
    bool has_utility_ai = false;
    bool has_neural_networks = false;
    bool has_crowd_simulation = false;
    bool has_formation = false;
};

struct AIStats {
    u64 total_ai_systems = 0;
    u64 total_agents = 0;
    u64 total_bt_nodes = 0;
    u64 total_states = 0;
    u64 total_navmeshes = 0;
    u64 total_disk_size = 0;
    u64 total_estimated_ram = 0;
    std::unordered_map<String, u32> format_counts;
    std::unordered_map<String, u32> engine_counts;
    std::unordered_map<String, u32> technique_counts;
    u32 max_agents_per_scene = 0;
    
    [[nodiscard]] String summary() const;
};

class AIAnalyzer {
public:
    explicit AIAnalyzer(const AnalyzerConfig& config);
    
    Result<std::vector<AIInfo>> analyze(const std::vector<FileInfo>& ai_files);
    Result<AIInfo> analyze_single(const FileInfo& ai);
    [[nodiscard]] AIStats stats() const { return stats_; }
    [[nodiscard]] String generate_report() const;

private:
    AnalyzerConfig config_;
    AIStats stats_;
    mutable std::mutex stats_mutex_;
    
    Result<AIInfo> analyze_navmesh(const Path& path);
    Result<AIInfo> analyze_behavior_tree(const Path& path);
    Result<AIInfo> analyze_state_machine(const Path& path);
    Result<AIInfo> analyze_htn(const Path& path);
    Result<AIInfo> analyze_goap(const Path& path);
    Result<AIInfo> analyze_utility_ai(const Path& path);
    Result<AIInfo> analyze_neural_net(const Path& path);
    Result<AIInfo> analyze_generic(const Path& path, FileType type);
    
    String read_file_content(const Path& path) const;
    u64 estimate_ram_usage(const AIInfo& ai) const;
    void detect_ai_system(AIInfo& ai, StringView content) const;
};

} // namespace game_req