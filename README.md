# gcanner

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![License](https://img.shields.io/badge/license-MIT-blue)]()
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)]()
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey)]()
[![GUI](https://img.shields.io/badge/GUI-Qt6-brightgreen)]()
[![Installer](https://img.shields.io/badge/installer-AppImage%20%7C%20DEB%20%7C%20RPM%20%7C%20DMG%20%7C%20PKG%20%7C%20NSIS-blue)]()

Premium Game System Requirements Analyzer - Cross-platform terminal UX with beautiful Qt6 GUI

## Overview

**gcanner** is a powerful tool for analyzing game directories and generating detailed system requirements recommendations. It scans game files, analyzes assets (textures, models, audio, scripts, executables, shaders, particles, physics, AI, and network), and provides minimum/recommended hardware specifications based on the actual complexity and requirements of the game.

Available as both a **command-line tool** and a **beautiful Qt6 GUI application** with modern dark/light themes, interactive charts, and hardware browser.

## Features

- **Comprehensive Asset Analysis**: Analyzes 10 asset types:
  - Textures (DDS, KTX, ASTC, PNG, JPEG, etc.)
  - 3D Models (FBX, OBJ, GLTF, COLLADA, etc.)
  - Audio (WAV, OGG, MP3, FLAC, XMA, etc.)
  - Scripts (C#, Lua, Python, JavaScript, HLSL/GLSL, etc.)
  - Executables (PE, ELF, Mach-O)
  - **Shaders** (HLSL, GLSL, SPIR-V, MSL, Cg, DXBC, DXIL)
  - **Particles** (PFX, PTX, PAR, EMITTER)
  - **Physics** (PhysX, Havok, Bullet, ODE, Newton)
  - **AI** (NavMesh, Behavior Trees, State Machines, HTN, GOAP, Neural Nets)
  - **Network** (Photon, Mirror, Netcode, Steam, EOS, RakNet, ENet)
- **Accurate Requirements Calculation**: CPU, GPU, RAM, VRAM, and storage requirements based on actual asset metrics
- **Hardware Database**: Built-in CPU, GPU, RAM, and storage benchmark data with performance-per-dollar recommendations
- **Modern Qt6 GUI**: Beautiful dark/light themes, interactive charts, hardware browser, scan progress tracking
- **Interactive Mode**: User-friendly terminal interface with menus and progress tracking
- **Multiple Output Formats**: Terminal (colorized), JSON, CSV, HTML, Markdown
- **Cross-platform**: Windows, macOS, Linux
- **Extensible Architecture**: Modular analyzer design for easy format additions
- **Archive Support**: ZIP, TAR, GZIP, RAR, 7Z, and game-specific formats (BSA, VPK, MPQ, CASC, FORGE, BIG, PAK)
- **Professional Installers**: AppImage, DEB, RPM, DMG, PKG, NSIS

## Installation

### Quick Install (Recommended)

**Linux/macOS:**
```bash
curl -fsSL https://get.gcanner.org | bash
```

**Windows (PowerShell):**
```powershell
irm https://get.gcanner.org/install.ps1 | iex
```

### Pre-built Installers

Download the latest release for your platform from [GitHub Releases](https://github.com/gcanner/gcanner/releases):

| Platform | Installer |
|----------|-----------|
| **Linux** | `gcanner-1.0.0-x86_64.AppImage` • `gcanner-1.0.0-x86_64.deb` • `gcanner-1.0.0-x86_64.rpm` |
| **macOS** | `gcanner-1.0.0-macos.dmg` • `gcanner-1.0.0-macos.pkg` |
| **Windows** | `gcanner-1.0.0-windows-x64.exe` (NSIS installer) |

**Linux (AppImage):**
```bash
chmod +x gcanner-1.0.0-x86_64.AppImage
./gcanner-1.0.0-x86_64.AppImage
```

**Linux (DEB/RPM):**
```bash
sudo dpkg -i gcanner-1.0.0-x86_64.deb       deb
# or
sudo rpm -i gcanner-1.0.0-x86_64.rpm
```

### Building from Source

**Prerequisites:**
- CMake 3.20+
- C++20 compiler (GCC 10+, Clang 11+, MSVC 2019+)
- Qt6 (Core, Widgets, Charts, Svg) - for GUI
- Dependencies (auto-fetched via CMake):
  - nlohmann_json 3.10+
  - fmt 9.0+

**Build:**
```bash
# Clone the repository
git clone https://github.com/gcanner/gcanner.git
cd gcanner

# Create and navigate to build directory
mkdir build && cd build

# Configure and build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release --parallel

# The executables are at:
# - build/src/gui/gcanner_gui    (Qt6 GUI application)
# - build/src/gcanner            (CLI, if enabled)
```

## Usage

### GUI Application (Recommended)

Launch the beautiful Qt6 GUI:
```bash
gcanner_gui
```

The GUI features:
- **Welcome Page**: Quick access to scan, hardware browser, settings
- **Scan Configuration**: Visual directory picker, analyzer toggles, output format selection
- **Live Progress**: Real-time progress bars, file counter, elapsed time, ETA
- **Results Dashboard**: Summary cards, detailed tables, interactive charts (pie, bar, comparison)
- **Hardware Browser**: Searchable CPU/GPU/RAM/Storage tables with performance metrics
- **Export**: JSON, CSV, HTML, Markdown reports
- **Dark/Light Themes**: Automatic system theme detection or manual toggle

### Command Line Interface

```bash
# Basic usage - analyze a game directory
gcanner /path/to/game

# Specify output format and file
gcanner -f json -o requirements.json /path/to/game

# Enable verbose output
gcanner -v /path/to/game

# Disable color output
gcanner --no-color /path/to/game

# Run in interactive mode
gcanner -i
```

### Command-line Options

| Option | Description |
|--------|-------------|
| `-h, --help` | Show help message |
| `-v, --version` | Show version information |
| `-o, --output <file>` | Output results to file |
| `-f, --format <fmt>` | Output format: `terminal`, `json`, `csv`, `markdown`, `html` |
| `-q, --quiet` | Quiet mode (minimal output) |
| `--verbose` | Verbose output |
| `--no-color` | Disable colored output |
| `--no-progress` | Disable progress bar |
| `-i, --interactive` | Run in interactive mode |
| `--config <file>` | Use custom configuration file |

### Interactive Mode

Launch with `gcanner -i` for an interactive menu with options to:
- Scan game directories
- Analyze folders
- Browse hardware database
- Configure settings
- Update hardware database
- Export results
- Compare hardware components

### Output Formats

```bash
# Terminal (default, colorized)
gcanner /path/to/game

# JSON for programmatic use
gcanner -f json -o report.json /path/to/game

# Markdown for documentation
gcanner -f markdown -o REPORT.md /path/to/game

# HTML for sharing
gcanner -f html -o report.html /path/to/game

# CSV for spreadsheets
gcanner -f csv -o report.csv /path/to/game
```

## Configuration

Create a `config.json` to customize analysis parameters:

```json
{
  "scan": {
    "recursive": true,
    "follow_symlinks": false,
    "max_depth": 20,
    "max_file_size": 107374182400,
    "thread_count": 0,
    "use_mmap": true,
    "verify_checksums": false,
    "excluded_dirs": [".git", ".svn", "node_modules", "__pycache__", ".vs", "build", "dist"],
    "excluded_exts": [".tmp", ".temp", ".bak", ".backup", ".log", ".pid", ".lock"]
  },
  "analyzer": {
    "analyze_textures": true,
    "analyze_models": true,
    "analyze_audio": true,
    "analyze_scripts": true,
    "analyze_executables": true,
    "analyze_shaders": true,
    "analyze_particles": true,
    "analyze_physics": true,
    "analyze_ai": true,
    "analyze_network": true,
    "texture_sample_count": 1000,
    "model_sample_count": 500,
    "compression_assumption": 0.3,
    "lod_factor": 0.7
  },
  "hardware": {
    "auto_update": true,
    "update_interval_days": 7,
    "cache_dir": "./hardware_cache",
    "preferred_source": "passmark",
    "include_mobile": false,
    "include_workstation": true,
    "include_server": false,
    "min_benchmark_score": 1000
  },
  "output": {
    "format": "terminal",
    "output_file": "",
    "verbose": false,
    "show_progress": true,
    "color_output": true,
    "max_display_items": 20,
    "show_recommendations": true,
    "show_alternatives": true,
    "show_bottlenecks": true
  }
}
```

Use with:
```bash
gcanner --config config.json /path/to/game
```

## How It Works

1. **File Scanning**: Recursively scans the specified directory for game files
2. **File Type Detection**: Identifies file types using extensions and magic numbers
3. **Asset Analysis**: Analyzes 10 different asset categories with format-specific parsers
4. **Aggregation**: Combines individual asset statistics to estimate total resource requirements
5. **Requirement Calculation**: Uses established formulas to convert asset metrics to hardware requirements
6. **Hardware Matching**: Compares requirements against hardware database for component recommendations
7. **Report Generation**: Formats results in the requested output format

## Hardware Database

The built-in hardware database includes benchmark data for:
- **CPUs**: Single/multi-thread performance, core/thread counts, clock speeds, TDP
- **GPUs**: 3DMark scores, VRAM capacity, ray tracing, DLSS/FSR/XeSS support
- **RAM**: Capacity, speed (MT/s), type (DDR4/DDR5/LPDDR5X), ECC support
- **Storage**: Sequential/random R/W speeds, capacity, interface (NVMe/SATA/HDD)

## Architecture

```
gcanner/
├── src/
│   ├── core/           # Application, config, logging, error handling
│   ├── scanner/        # File scanning, type detection, archive handling
│   ├── analyzers/      # 10 asset analyzers + aggregator + requirement calculator
│   ├── hardware/       # Hardware database (CPU, GPU, RAM, Storage)
│   ├── gui/            # Qt6 GUI (MainWindow, ScanWidget, ResultsWidget, etc.)
│   └── utils/          # Platform, file, hash, math, JSON, string, time utilities
├── include/            # Public headers
├── third_party/        # Bundled dependencies (fmt, nlohmann_json)
├── data/               # Embedded hardware database
├── installer/          # Platform-specific installers
└── .github/workflows/  # CI/CD pipelines
```

## Extending gcanner

### Adding New File Format Support

1. Create a new analyzer in `src/analyzers/` inheriting from `BaseAnalyzer`
2. Implement `analyze_single()` and `generate_report()` methods
3. Register in `AssetAggregator::analyze()`
4. Add file extensions to `FileTypeDetector`

### Adjusting Calculation Formulas

Modify constants in `src/analyzers/requirement_calculator.cpp`:
- `CPU_MHZ_PER_VERTEX`, `GPU_MHZ_PER_TRIANGLE`
- `VRAM_PER_TEXTURE_MB`, `RAM_PER_AUDIO_MB`
- `STORAGE_COMPRESSION_RATIO`, etc.

## Creating Installers

### Linux
```bash
cd installer/linux
./build_installer.sh
# Creates: .AppImage, .deb, .rpm
```

### macOS
```bash
cd installer/macos
./build_installer.sh
# Creates: .dmg, .pkg
```

### Windows
```cmd
cd installer/windows
makensis gcanner_installer.nsi
# Creates: gcanner-1.0.0-windows-x64.exe (NSIS installer)
```

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -am 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

Please follow the existing code style (C++20, clang-format) and add tests for new features.

## License

MIT License - see [LICENSE](LICENSE) for details.

## Acknowledgments

- [nlohmann/json](https://github.com/nlohmann/json) - JSON for Modern C++
- [fmt](https://github.com/fmtlib/fmt) - Modern formatting library
- [spdlog](https://github.com/gabime/spdlog) - Fast C++ logging library
- [Qt6](https://www.qt.io/) - Cross-platform application framework
- Inspired by game benchmarking tools and system requirement analyzers
- Thanks to the gaming community for test data and feedback