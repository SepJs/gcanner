# Gcammer

Premium Game System Requirements Analyzer - Cross-platform terminal UX

## Overview

GameReqAnalyzer is a powerful tool for analyzing game directories and generating detailed system requirements recommendations. It scans game files, analyzes assets (textures, models, audio, scripts, executables), and provides minimum and recommended hardware specifications based on the complexity and requirements of the game.

## Features

- **Comprehensive Analysis**: Analyzes textures, 3D models, audio files, scripts, and executables
- **Accurate Requirements Calculation**: Calculates CPU, GPU, RAM, and storage requirements based on actual asset analysis
- **Hardware Database**: Includes up-to-date CPU, GPU, RAM, and storage benchmark data for accurate recommendations
- **Interactive Mode**: User-friendly terminal interface for easy navigation and configuration
- **Multiple Output Formats**: Generate reports in terminal, JSON, CSV, HTML, and Markdown formats
- **Cross-platform**: Works on Windows, macOS, and Linux
- **Extensible Design**: Modular architecture makes it easy to add new file format analyzers

## Installation

### Prerequisites

- CMake 3.20 or higher
- C++20 compatible compiler (GCC 10+, Clang 11+, MSVC 2019+)
- Required dependencies:
  - nlohmann_json 3.10+
  - fmt 9.0+
  - spdlog 1.11+
  - Optional: OpenSSL, libcurl, libmagic for enhanced functionality

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/GameReqAnalyzer.git
cd GameReqAnalyzer

# Create and navigate to build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build . --config Release

# Install (optional)
sudo make install  # Linux/macOS
# or
cmake --install . --config Release  # Windows
```

## Usage

### Command Line Interface

```bash
# Basic usage - analyze a game directory
game-req-analyzer /path/to/game

# Specify output format and file
game-req-analyzer -f json -o requirements.json /path/to/game

# Enable verbose output
game-req-analyzer -v /path/to/game

# Disable color output
game-req-analyzer --no-color /path/to/game

# Run in interactive mode
game-req-analyzer -i
```

### Interactive Mode

Launch the interactive interface with:

```bash
game-req-analyzer -i
```

The interactive menu provides options to:
- Scan game directories
- Analyze folders
- Browse hardware database
- Configure settings
- Update hardware database
- Export results
- Compare hardware components

### Output Formats

Specify the output format with `-f` or `--format`:

- `terminal` (default): Colorful terminal output
- `json`: Machine-readable JSON format
- `csv`: Comma-separated values for spreadsheet import
- `html`: Styled HTML report
- `markdown`: GitHub-flavored markdown

Example:
```bash
game-req-analyzer -f markdown -o game_report.md /path/to/game
```

## Configuration

GameReqAnalyzer can be configured through command-line arguments or a configuration file.

### Command-line Options

- `-h, --help`: Show help message
- `-v, --version`: Show version information
- `-o, --output <file>`: Output results to file
- `-f, --format <fmt>`: Output format (json, csv, markdown, html, text)
- `-q, --quiet`: Quiet mode (minimal output)
- `-v, --verbose`: Verbose output
- `--no-color`: Disable colored output
- `--no-progress`: Disable progress bar
- `-i, --interactive`: Run in interactive mode

### Configuration File

Create a configuration file to customize analysis parameters:

```json
{
  "scan": {
    "recursive": true,
    "follow_symlinks": false,
    "max_depth": 20,
    "max_file_size": 107374182400, // 100GB
    "thread_count": 0, // 0 = hardware concurrency
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

Save this as `config.json` and use it with:
```bash
game-req-analyzer --config config.json /path/to/game
```

## How It Works

1. **File Scanning**: Recursively scans the specified directory for game files
2. **File Type Detection**: Identifies file types using extensions and magic numbers
3. **Asset Analysis**: Analyzes different asset types:
   - Textures: Resolution, format, compression, VRAM usage
   - Models: Vertex count, triangle count, complexity
   - Audio: Format, bitrate, duration, quantity
   - Scripts: Language, complexity, line count
   - Executables: Architecture, dependencies, engine detection
4. **Aggregation**: Combines individual asset statistics to estimate total resource requirements
5. **Requirement Calculation**: Uses established formulas to convert asset metrics to hardware requirements
6. **Hardware Matching**: Compares requirements against hardware database to suggest suitable components
7. **Report Generation**: Formats results in the requested output format

## Hardware Database

The hardware database includes benchmark data for:
- CPUs: Single-thread and multi-thread performance, core/thread counts, clock speeds
- GPUs: 3D performance, VRAM capacity, ray tracing capabilities, DLSS/FSR/XeSS support
- RAM: Capacity, speed, type (DDR4, DDR5, etc.), ECC support
- Storage: Sequential/random read/write speeds, capacity, interface type

Data is sourced from public benchmarks and updated regularly through the update mechanism.

## Customization

### Adding New File Format Support

To add support for a new file format:

1. Create a new analyzer class inheriting from the appropriate base analyzer
2. Implement the analysis methods for your file type
3. Register the analyzer in the AssetAggregator
4. Update the file type detector to recognize your file extension

### Adjusting Calculation Formulas

The requirement calculation formulas are in `requirement_calculator.cpp`. You can adjust the constants and multipliers to better match your specific needs or hardware preferences.

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -am 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

Please ensure your code follows the existing style and includes appropriate tests.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Thanks to the creators of the excellent libraries used: nlohmann_json, fmt, and spdlog
- Inspired by various system requirement analysis tools and game benchmarking resources
- Special thanks to the gaming community for providing feedback and test data
