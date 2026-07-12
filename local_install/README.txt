gcanner 1.0.0 - Game System Requirements Analyzer
===================================================

This package contains the core analyzer library with all 10 asset analyzers.

Contents:
  lib/libgcanner_analyzers.a   - Static library (18 MB) with all analyzers
  include/                      - C++ headers for all analyzers
  share/gcanner/data/           - Hardware database (JSON)

Analyzers Included (10 total):
  1. TextureAnalyzer    - DDS, KTX, ASTC, PNG, JPEG, EXR, HDR, etc.
  2. ModelAnalyzer      - FBX, OBJ, GLTF, COLLADA, USD, Blend, etc.
  3. AudioAnalyzer      - WAV, OGG, MP3, FLAC, XMA, etc.
  4. ScriptAnalyzer     - C#, Lua, Python, JS, HLSL/GLSL, etc.
  5. ExecutableAnalyzer - PE, ELF, Mach-O
  6. ShaderAnalyzer     - HLSL, GLSL, SPIR-V, MSL, DXBC, DXIL
  7. ParticleAnalyzer   - PFX, PTX, PAR, EMITTER
  8. PhysicsAnalyzer    - PhysX, Havok, Bullet, ODE, Newton
  9. AIAnalyzer         - NavMesh, BehaviorTree, StateMachine, HTN, GOAP, NeuralNet
  10. NetworkAnalyzer   - Photon, Mirror, Netcode, Steam, EOS, RakNet, ENet

Building the CLI or GUI:
  # Requires Qt6 for GUI, CMake 3.20+, C++20 compiler
  cmake -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build --config Release --target gcanner_gui

Installing on Linux:
  # AppImage (recommended)
  chmod +x gcanner-1.0.0-x86_64.AppImage
  ./gcanner-1.0.0-x86_64.AppImage /path/to/game

  # DEB package
  sudo dpkg -i gcanner-1.0.0-x86_64.deb

  # RPM package
  sudo rpm -i gcanner-1.0.0-x86_64.rpm

  # From source
  cmake --install build --prefix /usr/local

License: MIT
Website: https://github.com/SepJs/gcanner
