#!/bin/bash
# gcanner Universal Installer
# Run with: curl -fsSL https://get.gcanner.org | bash
# Or download and run: bash gcanner-installer.sh

set -e

APP_NAME="gcanner"
REPO="SepJs/gcanner"
INSTALL_DIR="/opt/gcanner"
BIN_DIR="/usr/local/bin"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_banner() {
    echo -e "${BLUE}"
    cat << 'EOF'
    ██████╗ ██╗██╗     ███████╗██████╗ ███████╗████████╗
    ██╔══██╗██║██║     ██╔════╝██╔══██╗██╔════╝╚══██╔══╝
    ██████╔╝██║██║     █████╗  ██║  ██║█████╗     ██║   
    ██╔══██╗██║██║     ██╔══╝  ██║  ██║██╔══╝     ██║   
    ██████╔╝██║███████╗███████╗██████╔╝███████╗   ██║   
    ╚═════╝ ╚═╝╚══════╝╚══════╝╚═════╝ ╚══════╝   ╚═╝   
                                                        
    Game System Requirements Analyzer
EOF
    echo -e "${NC}"
}

detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        OS="linux"
        if [[ -f /etc/os-release ]]; then
            . /etc/os-release
            DISTRO=$ID
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"
    elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
        OS="windows"
    else
        OS="unknown"
    fi
}

get_latest_version() {
    VERSION=$(curl -s "https://api.github.com/repos/${REPO}/releases/latest" | grep '"tag_name"' | cut -d'"' -f4)
    if [[ -z "$VERSION" ]]; then
        echo -e "${RED}Failed to fetch latest version${NC}"
        exit 1
    fi
    echo "$VERSION"
}

download_file() {
    local url=$1
    local output=$2
    echo -e "${YELLOW}Downloading $output...${NC}"
    if command -v curl &> /dev/null; then
        curl -fsSL -o "$output" "$url"
    elif command -v wget &> /dev/null; then
        wget -q -O "$output" "$url"
    else
        echo -e "${RED}curl or wget required${NC}"
        exit 1
    fi
}

install_linux() {
    echo -e "${GREEN}Installing gcanner on Linux...${NC}"

    # Check if running as root for system install
    if [[ $EUID -ne 0 ]]; then
        echo -e "${YELLOW}Installing to ~/.local/bin (user install)${NC}"
        INSTALL_DIR="$HOME/.local/share/gcanner"
        BIN_DIR="$HOME/.local/bin"
        DESKTOP_DIR="$HOME/.local/share/applications"
        ICON_DIR="$HOME/.local/share/icons/hicolor/256x256/apps"
        SUDO=""
    else
        INSTALL_DIR="/opt/gcanner"
        BIN_DIR="/usr/local/bin"
        DESKTOP_DIR="/usr/share/applications"
        ICON_DIR="/usr/share/icons/hicolor/256x256/apps"
        SUDO="sudo"
    fi

    VERSION=$(get_latest_version)
    VERSION=${VERSION#v}

    # Download AppImage
    APPIMAGE_URL="https://github.com/${REPO}/releases/download/v${VERSION}/gcanner-${VERSION}-x86_64.AppImage"
    download_file "$APPIMAGE_URL" "gcanner.AppImage"
    chmod +x gcanner.AppImage

    # Install
    $SUDO mkdir -p "$INSTALL_DIR"
    $SUDO mv gcanner.AppImage "$INSTALL_DIR/gcanner"
    $SUDO chmod +x "$INSTALL_DIR/gcanner"

    # Create launcher
    $SUDO mkdir -p "$BIN_DIR"
    cat | $SUDO tee "$BIN_DIR/gcanner" > /dev/null << 'LAUNCHER_EOF'
#!/bin/bash
exec /opt/gcanner/gcanner "$@"
LAUNCHER_EOF
    $SUDO chmod +x "$BIN_DIR/gcanner"

    # Desktop entry
    $SUDO mkdir -p "$DESKTOP_DIR"
    cat | $SUDO tee "$DESKTOP_DIR/gcanner.desktop" > /dev/null << EOF
[Desktop Entry]
Type=Application
Name=gcanner
GenericName=Game System Requirements Analyzer
Comment=Scan game directories for system requirements
Exec=gcanner
Icon=gcanner
Terminal=false
Categories=Utility;Development;
StartupNotify=true
EOF

    # Extract icon from AppImage
    cd /tmp
    "$INSTALL_DIR/gcanner" --appimage-extract gcanner.png 2>/dev/null || true
    if [[ -f squashfs-root/usr/share/icons/hicolor/256x256/apps/gcanner.png ]]; then
        $SUDO mkdir -p "$ICON_DIR"
        $SUDO cp squashfs-root/usr/share/icons/hicolor/256x256/apps/gcanner.png "$ICON_DIR/gcanner.png"
    fi
    rm -rf squashfs-root

    # Update desktop database
    $SUDO update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
    $SUDO gtk-update-icon-cache "$ICON_DIR" 2>/dev/null || true

    echo -e "${GREEN}Installation complete!${NC}"
    echo "Run 'gcanner' from terminal or find it in your applications menu."
}

install_macos() {
    echo -e "${GREEN}Installing gcanner on macOS...${NC}"

    VERSION=$(get_latest_version)
    VERSION=${VERSION#v}

    # Download DMG
    DMG_URL="https://github.com/${REPO}/releases/download/v${VERSION}/gcanner-${VERSION}-macos.dmg"
    download_file "$DMG_URL" "gcanner.dmg"

    # Mount and install
    echo -e "${YELLOW}Mounting DMG...${NC}"
    MOUNT_POINT=$(hdiutil attach gcanner.dmg -nobrowse -quiet | tail -1 | awk '{print $3}')

    if [[ -d "$MOUNT_POINT/gcanner.app" ]]; then
        echo -e "${YELLOW}Installing to /Applications...${NC}"
        cp -R "$MOUNT_POINT/gcanner.app" /Applications/
        echo -e "${GREEN}Installation complete!${NC}"
    else
        # Try PKG
        PKG_PATH=$(find "$MOUNT_POINT" -name "*.pkg" | head -1)
        if [[ -n "$PKG_PATH" ]]; then
            sudo installer -pkg "$PKG_PATH" -target /
            echo -e "${GREEN}Installation complete!${NC}"
        else
            echo -e "${RED}Could not find app or pkg in DMG${NC}"
            exit 1
        fi
    fi

    hdiutil detach "$MOUNT_POINT" -quiet
    rm gcanner.dmg
}

install_windows() {
    echo -e "${GREEN}Installing gcanner on Windows...${NC}"

    VERSION=$(get_latest_version)
    VERSION=${VERSION#v}

    EXE_URL="https://github.com/${REPO}/releases/download/v${VERSION}/gcanner-${VERSION}-windows-x64.exe"
    download_file "$EXE_URL" "gcanner_installer.exe"

    # Run installer silently
    ./gcanner_installer.exe /S

    echo -e "${GREEN}Installation complete!${NC}"
    echo "Find gcanner in your Start Menu."
}

main() {
    print_banner
    detect_os

    echo -e "${BLUE}Detected OS: $OS${NC}"

    case $OS in
        linux)
            install_linux
            ;;
        macos)
            install_macos
            ;;
        windows)
            install_windows
            ;;
        *)
            echo -e "${RED}Unsupported OS: $OS${NC}"
            echo "Please download the installer manually from:"
            echo "https://github.com/${REPO}/releases"
            exit 1
            ;;
    esac

    echo ""
    echo -e "${GREEN}🎉 gcanner installed successfully!${NC}"
    echo ""
    echo "Usage:"
    echo "  gcanner                    # Launch GUI"
    echo "  gcanner /path/to/game      # Scan a game directory"
    echo "  gcanner --help             # Show help"
    echo ""
    echo "For more information, visit: https://github.com/${REPO}"
}

# Check for --help
if [[ "$1" == "--help" || "$1" == "-h" ]]; then
    echo "gcanner Universal Installer"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --help, -h     Show this help"
    echo "  --version      Show version and exit"
    echo ""
    echo "This script automatically detects your OS and downloads"
    echo "the appropriate installer from the latest GitHub release."
    exit 0
fi

if [[ "$1" == "--version" ]]; then
    get_latest_version
    exit 0
fi

main "$@"