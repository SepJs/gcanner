#include "ScanWorker.hpp"

#include <QMessageBox>
#include <QApplication>
#include <QScreen>

void AboutDialog::show(QWidget* parent) {
    QMessageBox::about(parent, "About gcanner",
        "<html>"
        "<head><style>"
        "body { background: #1e1e2e; color: #cdd6f4; font-family: system-ui; }"
        "h1 { color: #89b4fa; margin: 0; }"
        "p { color: #a6adc8; }"
        "a { color: #89b4fa; }"
        ".version { color: #7f849c; font-size: 14px; }"
        ".features { margin: 16px 0; }"
        ".feature { margin: 8px 0; }"
        "</style></head>"
        "<body>"
        "<div style='text-align: center; padding: 20px;'>"
        "<h1>gcanner</h1>"
        "<p class='version'>Version 1.0.0</p>"
        "<p>Premium Game System Requirements Analyzer</p>"
        "<p>Cross-platform terminal UX with beautiful GUI</p>"
        ""
        "<div class='features'>"
        "<div class='feature'>🔍 <b>10 Asset Analyzers:</b> Textures, Models, Audio, Scripts, Executables, Shaders, Particles, Physics, AI, Network</div>"
        "<div class='feature'>📊 <b>Detailed Reports:</b> Minimum, Recommended, High, Ultra, Ray Tracing tiers</div>"
        "<div class='feature'>🖥️ <b>Hardware Database:</b> CPUs, GPUs, RAM, Storage with performance-per-dollar</div>"
        "<div class='feature'>🎨 <b>Modern UI:</b> Dark/Light themes, charts, export to JSON/CSV/HTML/Markdown</div>"
        "<div class='feature'>⚡ <b>Fast & Parallel:</b> Multi-threaded scanning with memory-mapped files</div>"
        "<div class='feature'>🔧 <b>Configurable:</b> Extensive settings, custom analyzers, output formats</div>"
        "</div>"
        ""
        "<hr style='border-color: #313244; margin: 16px 0;'>"
        "<p>Built with Qt6, C++20, fmt, nlohmann_json</p>"
        "<p><a href='https://github.com/gcanner/gcanner'>GitHub</a> | "
        "<a href='https://github.com/gcanner/gcanner/issues'>Report Issue</a></p>"
        "<p>© 2024-2026 gcanner contributors</p>"
        "</div>"
        "</body>"
        "</html>"
    );
}