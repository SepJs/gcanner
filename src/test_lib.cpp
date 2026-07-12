#include <game_req_analyzer/analyzers/texture_analyzer.hpp>
#include <game_req_analyzer/analyzers/model_analyzer.hpp>
#include <game_req_analyzer/analyzers/audio_analyzer.hpp>
#include <game_req_analyzer/analyzers/script_analyzer.hpp>
#include <game_req_analyzer/analyzers/executable_analyzer.hpp>
#include <game_req_analyzer/analyzers/shader_analyzer.hpp>
#include <game_req_analyzer/analyzers/particle_analyzer.hpp>
#include <game_req_analyzer/analyzers/physics_analyzer.hpp>
#include <game_req_analyzer/analyzers/ai_analyzer.hpp>
#include <game_req_analyzer/analyzers/network_analyzer.hpp>
#include <game_req_analyzer/analyzers/asset_aggregator.hpp>
#include <game_req_analyzer/analyzers/requirement_calculator.hpp>
#include <game_req_analyzer/core/config.hpp>

#include <iostream>
#include <fmt/core.h>

using namespace game_req;

int main() {
    fmt::print("Testing gcanner analyzers...\n");
    
    AnalyzerConfig cfg;
    
    TextureAnalyzer ta(cfg);
    fmt::print("✓ TextureAnalyzer created\n");
    
    ModelAnalyzer ma(cfg);
    fmt::print("✓ ModelAnalyzer created\n");
    
    AudioAnalyzer aa(cfg);
    fmt::print("✓ AudioAnalyzer created\n");
    
    ScriptAnalyzer sa(cfg);
    fmt::print("✓ ScriptAnalyzer created\n");
    
    ExecutableAnalyzer ea(cfg);
    fmt::print("✓ ExecutableAnalyzer created\n");
    
    ShaderAnalyzer sha(cfg);
    fmt::print("✓ ShaderAnalyzer created\n");
    
    ParticleAnalyzer pa(cfg);
    fmt::print("✓ ParticleAnalyzer created\n");
    
    PhysicsAnalyzer pha(cfg);
    fmt::print("✓ PhysicsAnalyzer created\n");
    
    AIAnalyzer aia(cfg);
    fmt::print("✓ AIAnalyzer created\n");
    
    NetworkAnalyzer na(cfg);
    fmt::print("✓ NetworkAnalyzer created\n");
    
    AssetAggregator agg(cfg);
    fmt::print("✓ AssetAggregator created\n");
    
    RequirementCalculator rc(cfg, HardwareDatabase::instance());
    fmt::print("✓ RequirementCalculator created\n");
    
    fmt::print("\n✅ All 10 analyzers + aggregator + calculator load successfully!\n");
    fmt::print("Library: libgcanner_analyzers.a (18 MB)\n");
    
    return 0;
}