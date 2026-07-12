#include <game_req_analyzer/analyzers/ai_analyzer.hpp>
#include <game_req_analyzer/utils/file_utils.hpp>
#include <game_req_analyzer/utils/string_utils.hpp>
#include <game_req_analyzer/core/logger.hpp>
#include <algorithm>
#include <fstream>
#include <regex>

namespace game_req {

AIAnalyzer::AIAnalyzer(const AnalyzerConfig& config) : config_(config) {}

Result<std::vector<AIInfo>> AIAnalyzer::analyze(const std::vector<FileInfo>& ai_files) {
    std::vector<AIInfo> results;
    results.reserve(ai_files.size());
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_ai_systems = 0;
        stats_.total_agents = 0;
        stats_.total_bt_nodes = 0;
        stats_.total_states = 0;
        stats_.total_navmeshes = 0;
        stats_.total_disk_size = 0;
        stats_.total_estimated_ram = 0;
        stats_.format_counts.clear();
        stats_.engine_counts.clear();
        stats_.technique_counts.clear();
        stats_.max_agents_per_scene = 0;
    }
    
    for (const auto& ai : ai_files) {
        auto result = analyze_single(ai);
        if (result) {
            results.push_back(*result);
            
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_ai_systems++;
            stats_.total_agents += result->agent_count;
            stats_.total_bt_nodes += result->behavior_tree_nodes;
            stats_.total_states += result->state_machine_states;
            stats_.total_navmeshes += result->navigation_meshes;
            stats_.total_disk_size += result->disk_size;
            stats_.total_estimated_ram += result->estimated_ram;
            stats_.format_counts[result->format]++;
            if (!result->engine_hint.empty()) stats_.engine_counts[result->engine_hint]++;
            for (const auto& tech : result->ai_techniques) {
                stats_.technique_counts[tech]++;
            }
            if (result->agent_count > stats_.max_agents_per_scene) {
                stats_.max_agents_per_scene = result->agent_count;
            }
        } else {
            LOG_WARN("Failed to analyze AI file {}: {}", ai.path.string(), result.error().message);
        }
    }
    
    return results;
}

Result<AIInfo> AIAnalyzer::analyze_single(const FileInfo& ai) {
    AIInfo info;
    info.disk_size = ai.size;
    
    try {
        // AI files don't have a specific FileType, use generic analysis
        return analyze_generic(ai.path, ai.type);
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, 
            std::format("AI analysis failed: {}", e.what())));
    }
}

String AIAnalyzer::read_file_content(const Path& path) const {
    std::ifstream file(path);
    if (!file) return "";
    std::string content;
    file.seekg(0, std::ios::end);
    content.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return content;
}

Result<AIInfo> AIAnalyzer::analyze_navmesh(const Path& path) {
    AIInfo info;
    info.format = "NavMesh";
    info.disk_size = fs::file_size(path);
    info.has_navmesh = true;
    info.has_pathfinding = true;
    
    String content = read_file_content(path);
    if (!content.empty()) {
        detect_ai_system(info, content);
        
        // Recast/Detour patterns
        std::regex navmesh_regex(R"(dtNavMesh|dtNavMeshQuery|rcPolyMesh|rcPolyMeshDetail)");
        info.navigation_meshes = std::distance(
            std::sregex_iterator(content.begin(), content.end(), navmesh_regex),
            std::sregex_iterator()
        );
        
        // Agent count from crowd simulation
        std::regex crowd_regex(R"(dtCrowd|dtCrowdAgent)");
        info.agent_count = std::distance(
            std::sregex_iterator(content.begin(), content.end(), crowd_regex),
            std::sregex_iterator()
        );
        
        // Pathfinding queries
        std::regex query_regex(R"(dtQueryFilter|findPath|raycast)");
        info.pathfinding_queries = std::distance(
            std::sregex_iterator(content.begin(), content.end(), query_regex),
            std::sregex_iterator()
        );
        
        // Techniques
        if (content.find("Recast") != String::npos || content.find("Detour") != String::npos) {
            info.ai_system = "Recast/Detour";
            info.ai_techniques.push_back("NavMesh");
        }
        if (content.find("A* Pathfinding Project") != String::npos) {
            info.ai_system = "Aron Granberg A* Pathfinding Project";
            info.ai_techniques.push_back("NavMesh");
        }
        if (content.find("crowd") != String::npos || content.find("Crowd") != String::npos) {
            info.has_crowd_simulation = true;
            info.ai_techniques.push_back("Crowd Simulation");
        }
    }
    
    if (info.navigation_meshes == 0) info.navigation_meshes = 1;
    if (info.agent_count == 0) info.agent_count = 100; // Default estimate
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

Result<AIInfo> AIAnalyzer::analyze_behavior_tree(const Path& path) {
    AIInfo info;
    info.format = "BehaviorTree";
    info.disk_size = fs::file_size(path);
    info.has_behavior_trees = true;
    
    String content = read_file_content(path);
    if (!content.empty()) {
        detect_ai_system(info, content);
        
        // Behavior tree node patterns
        std::regex sequence_regex(R"(Sequence|sequence)");
        std::regex selector_regex(R"(Selector|selector)");
        std::regex parallel_regex(R"(Parallel|parallel)");
        std::regex decorator_regex(R"(Decorator|decorator|Inverter|Succeeder|Failer|Repeater|UntilSuccess|UntilFail)");
        std::regex action_regex(R"(Action|action|Task|task)");
        std::regex condition_regex(R"(Condition|condition|Check|check)");
        
        info.behavior_tree_nodes += std::distance(
            std::sregex_iterator(content.begin(), content.end(), sequence_regex),
            std::sregex_iterator()
        );
        info.behavior_tree_nodes += std::distance(
            std::sregex_iterator(content.begin(), content.end(), selector_regex),
            std::sregex_iterator()
        );
        info.behavior_tree_nodes += std::distance(
            std::sregex_iterator(content.begin(), content.end(), parallel_regex),
            std::sregex_iterator()
        );
        info.behavior_tree_nodes += std::distance(
            std::sregex_iterator(content.begin(), content.end(), decorator_regex),
            std::sregex_iterator()
        );
        info.behavior_tree_nodes += std::distance(
            std::sregex_iterator(content.begin(), content.end(), action_regex),
            std::sregex_iterator()
        );
        info.behavior_tree_nodes += std::distance(
            std::sregex_iterator(content.begin(), content.end(), condition_regex),
            std::sregex_iterator()
        );
        
        // Detect specific BT systems
        if (content.find("BehaviorDesigner") != String::npos || content.find("Behavior Designer") != String::npos) {
            info.ai_system = "Behavior Designer";
        } else if (content.find("NodeCanvas") != String::npos) {
            info.ai_system = "NodeCanvas";
        } else if (content.find("RAIN") != String::npos) {
            info.ai_system = "RAIN AI";
        } else if (content.find("BehaviorTree") != String::npos) {
            info.ai_system = "BehaviorTree (Generic)";
        }
        
        info.ai_techniques.push_back("Behavior Trees");
    }
    
    if (info.behavior_tree_nodes == 0) info.behavior_tree_nodes = 50; // Default
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

Result<AIInfo> AIAnalyzer::analyze_state_machine(const Path& path) {
    AIInfo info;
    info.format = "StateMachine";
    info.disk_size = fs::file_size(path);
    info.has_state_machines = true;
    
    String content = read_file_content(path);
    if (!content.empty()) {
        detect_ai_system(info, content);
        
        // FSM patterns
        std::regex state_regex(R"(State|state)");
        std::regex transition_regex(R"(Transition|transition|AnyState)");
        std::regex entry_regex(R"(OnEnter|onEnter|EnterState)");
        std::regex exit_regex(R"(OnExit|onExit|ExitState)");
        std::regex update_regex(R"(OnUpdate|onUpdate|UpdateState)");
        
        info.state_machine_states += std::distance(
            std::sregex_iterator(content.begin(), content.end(), state_regex),
            std::sregex_iterator()
        );
        
        // Detect specific FSM systems
        if (content.find("PlayMaker") != String::npos) {
            info.ai_system = "PlayMaker";
        } else if (content.find("StateMachine") != String::npos || content.find("FiniteStateMachine") != String::npos) {
            info.ai_system = "StateMachine (Generic)";
        }
        
        info.ai_techniques.push_back("Finite State Machine");
    }
    
    if (info.state_machine_states == 0) info.state_machine_states = 20; // Default
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

Result<AIInfo> AIAnalyzer::analyze_htn(const Path& path) {
    AIInfo info;
    info.format = "HTN";
    info.disk_size = fs::file_size(path);
    info.has_htn = true;
    
    String content = read_file_content(path);
    if (!content.empty()) {
        detect_ai_system(info, content);
        
        // HTN patterns (Hierarchical Task Network)
        std::regex task_regex(R"(Task|task|Method|method|Compound|compound)");
        std::regex decomposition_regex(R"(Decomposition|decomposition|subtask)");
        
        info.behavior_tree_nodes += std::distance(
            std::sregex_iterator(content.begin(), content.end(), task_regex),
            std::sregex_iterator()
        );
        
        info.ai_techniques.push_back("Hierarchical Task Network");
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

Result<AIInfo> AIAnalyzer::analyze_goap(const Path& path) {
    AIInfo info;
    info.format = "GOAP";
    info.disk_size = fs::file_size(path);
    info.has_goap = true;
    
    String content = read_file_content(path);
    if (!content.empty()) {
        detect_ai_system(info, content);
        
        // GOAP patterns (Goal-Oriented Action Planning)
        std::regex action_regex(R"(Action|action|Goal|goal|Plan|plan|Planner|planner)");
        std::regex condition_regex(R"(Precondition|precondition|Effect|effect)");
        
        info.behavior_tree_nodes += std::distance(
            std::sregex_iterator(content.begin(), content.end(), action_regex),
            std::sregex_iterator()
        );
        
        info.ai_techniques.push_back("Goal-Oriented Action Planning");
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

Result<AIInfo> AIAnalyzer::analyze_utility_ai(const Path& path) {
    AIInfo info;
    info.format = "UtilityAI";
    info.disk_size = fs::file_size(path);
    info.has_utility_ai = true;
    
    String content = read_file_content(path);
    if (!content.empty()) {
        detect_ai_system(info, content);
        
        // Utility AI patterns
        std::regex consideration_regex(R"(Consideration|consideration|Score|score|Utility|utility)");
        std::regex response_regex(R"(Response|response|Action|action)");
        
        info.behavior_tree_nodes += std::distance(
            std::sregex_iterator(content.begin(), content.end(), consideration_regex),
            std::sregex_iterator()
        );
        
        info.ai_techniques.push_back("Utility AI");
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

Result<AIInfo> AIAnalyzer::analyze_neural_net(const Path& path) {
    AIInfo info;
    info.format = "NeuralNetwork";
    info.disk_size = fs::file_size(path);
    info.has_neural_networks = true;
    
    String content = read_file_content(path);
    if (!content.empty()) {
        detect_ai_system(info, content);
        
        // Neural network patterns
        std::regex layer_regex(R"(Layer|layer|Dense|Conv|LSTM|GRU|Transformer)");
        std::regex tensor_regex(R"(Tensor|tensor|Weight|weight|Bias|bias)");
        
        info.behavior_tree_nodes += std::distance(
            std::sregex_iterator(content.begin(), content.end(), layer_regex),
            std::sregex_iterator()
        );
        
        // Detect frameworks
        if (content.find("TensorFlow") != String::npos || content.find("tf.") != String::npos) {
            info.ai_system = "TensorFlow";
        } else if (content.find("PyTorch") != String::npos || content.find("torch.") != String::npos) {
            info.ai_system = "PyTorch";
        } else if (content.find("ONNX") != String::npos) {
            info.ai_system = "ONNX";
        } else if (content.find("Barracuda") != String::npos) {
            info.ai_system = "Unity Barracuda";
        }
        
        info.ai_techniques.push_back("Neural Networks");
    }
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

Result<AIInfo> AIAnalyzer::analyze_generic(const Path& path, FileType type) {
    AIInfo info;
    info.disk_size = fs::file_size(path);
    info.format = "Unknown";
    
    String content = read_file_content(path);
    if (!content.empty()) {
        detect_ai_system(info, content);
        
        // Generic AI detection
        if (content.find("NavMesh") != String::npos || content.find("navmesh") != String::npos) {
            info.has_navmesh = true;
            info.ai_techniques.push_back("NavMesh");
        }
        if (content.find("BehaviorTree") != String::npos || content.find("behavior.tree") != String::npos) {
            info.has_behavior_trees = true;
            info.ai_techniques.push_back("Behavior Trees");
        }
        if (content.find("StateMachine") != String::npos || content.find("state.machine") != String::npos) {
            info.has_state_machines = true;
            info.ai_techniques.push_back("State Machine");
        }
        if (content.find("Neural") != String::npos || content.find("neural") != String::npos) {
            info.has_neural_networks = true;
            info.ai_techniques.push_back("Neural Networks");
        }
    }
    
    // Default estimates
    info.agent_count = 50;
    info.navigation_meshes = 1;
    info.behavior_tree_nodes = 20;
    info.state_machine_states = 10;
    
    info.estimated_ram = estimate_ram_usage(info);
    return info;
}

u64 AIAnalyzer::estimate_ram_usage(const AIInfo& ai) const {
    // AI system RAM usage:
    // - Agents: ~1-10 KB each (state, blackboard, sensors)
    // - NavMesh: ~10-100 MB depending on world size
    // - Behavior Trees: ~1-10 KB per node
    // - State Machines: ~0.5-2 KB per state
    // - Pathfinding cache: ~1-10 MB
    // - Neural networks: Model size + inference buffers
    
    u64 ram = 0;
    ram += ai.agent_count * 8 * 1024;              // 8 KB per agent
    ram += ai.navigation_meshes * 25 * 1024 * 1024; // 25 MB per navmesh (average)
    ram += ai.behavior_tree_nodes * 2 * 1024;       // 2 KB per BT node
    ram += ai.state_machine_states * 1024;          // 1 KB per state
    ram += ai.pathfinding_queries * 256;            // 256 bytes per query
    
    // Crowd simulation
    if (ai.has_crowd_simulation) {
        ram += ai.agent_count * 4 * 1024; // 4 KB per agent for crowd
    }
    
    // Neural networks (rough estimate)
    if (ai.has_neural_networks) {
        ram += 50 * 1024 * 1024; // 50 MB for model + inference
    }
    
    return std::max<u64>(ram, 4 * 1024 * 1024); // Min 4 MB
}

void AIAnalyzer::detect_ai_system(AIInfo& ai, StringView content) const {
    if (content.find("Recast") != String::npos || content.find("Detour") != String::npos) {
        ai.ai_system = "Recast/Detour";
    } else if (content.find("A* Pathfinding Project") != String::npos) {
        ai.ai_system = "Aron Granberg A* Pathfinding Project";
    } else if (content.find("BehaviorDesigner") != String::npos || content.find("Behavior Designer") != String::npos) {
        ai.ai_system = "Behavior Designer";
    } else if (content.find("NodeCanvas") != String::npos) {
        ai.ai_system = "NodeCanvas";
    } else if (content.find("RAIN") != String::npos) {
        ai.ai_system = "RAIN AI";
    } else if (content.find("PlayMaker") != String::npos) {
        ai.ai_system = "PlayMaker";
    } else if (content.find("BehaviorTree") != String::npos) {
        ai.ai_system = "BehaviorTree (Generic)";
    } else if (content.find("Unity") != String::npos && content.find("AI") != String::npos) {
        ai.ai_system = "Unity AI";
    } else if (content.find("Unreal") != String::npos && (content.find("AI") != String::npos || content.find("Behavior") != String::npos)) {
        ai.ai_system = "Unreal Engine AI";
    }
}

String AIAnalyzer::generate_report() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    std::stringstream ss;
    ss << "AI Analysis Report\n";
    ss << "==================\n";
    ss << "Total AI Systems: " << stats_.total_ai_systems << "\n";
    ss << "Total Agents: " << StringUtils::format_number(stats_.total_agents) << "\n";
    ss << "Total Behavior Tree Nodes: " << StringUtils::format_number(stats_.total_bt_nodes) << "\n";
    ss << "Total State Machine States: " << StringUtils::format_number(stats_.total_states) << "\n";
    ss << "Total NavMeshes: " << stats_.total_navmeshes << "\n";
    ss << "Total Disk Size: " << StringUtils::format_bytes(stats_.total_disk_size) << "\n";
    ss << "Estimated RAM: " << StringUtils::format_bytes(stats_.total_estimated_ram) << "\n";
    ss << "Max Agents/Scene: " << stats_.max_agents_per_scene << "\n\n";
    
    if (!stats_.format_counts.empty()) {
        ss << "Format Distribution:\n";
        for (const auto& [fmt, count] : stats_.format_counts) {
            ss << "  " << fmt << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.engine_counts.empty()) {
        ss << "AI System Detection:\n";
        for (const auto& [engine, count] : stats_.engine_counts) {
            ss << "  " << engine << ": " << count << "\n";
        }
        ss << "\n";
    }
    
    if (!stats_.technique_counts.empty()) {
        ss << "AI Technique Distribution:\n";
        for (const auto& [tech, count] : stats_.technique_counts) {
            ss << "  " << tech << ": " << count << "\n";
        }
    }
    
    return ss.str();
}

String AIStats::summary() const {
    std::stringstream ss;
    ss << "AI Systems: " << total_ai_systems 
       << ", Agents: " << StringUtils::format_number(total_agents)
       << ", BT Nodes: " << StringUtils::format_number(total_bt_nodes)
       << ", States: " << StringUtils::format_number(total_states)
       << ", NavMeshes: " << total_navmeshes
       << ", Disk: " << StringUtils::format_bytes(total_disk_size)
       << ", RAM: " << StringUtils::format_bytes(total_estimated_ram);
    return ss.str();
}

} // namespace game_req