#!/bin/bash
echo "Checking library symbols..."
nm lib/libgcanner_analyzers.a | grep -c "TextureAnalyzer\|ModelAnalyzer\|AudioAnalyzer\|ScriptAnalyzer\|ExecutableAnalyzer\|ShaderAnalyzer\|ParticleAnalyzer\|PhysicsAnalyzer\|AIAnalyzer\|NetworkAnalyzer" | head -1
echo "Total analyzer symbols found: $(nm lib/libgcanner_analyzers.a | grep -c "T _ZN8game_req.*Analyzer")"
echo ""
echo "New analyzers (5):"
nm lib/libgcanner_analyzers.a | grep "T _ZN8game_req" | grep -E "Shader|Particle|Physics|AIAnalyzer|Network" | grep "C1ERKNS_14AnalyzerConfigE" | sed 's/.*game_req//' | sed 's/C1ERKNS.*//' | sort -u
