#!/bin/bash
# gcanner Universal Installer
# Usage: curl -fsSL https://get.gcanner.org | bash
# Or download and run: bash gcanner-install.sh

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

APP_NAME="gcanner"
VERSION="1.0.0"
REPO="SepJs/gcanner"
INSTALL_DIR="/opt/gcanner"
BIN_DIR="/usr/local/bin"

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
    OS=$(uname -s | tr '[:upper:]' '[:lower:]')
    ARCH=$(uname -m)
    
    case $ARCH in
        x86_64) ARCH="x86_64" ;;
        aarch64|arm64) ARCH="arm64" ;;
        *) echo -e "${RED}Unsupported architecture: $ARCH${NC}"; exit 1 ;;
    esac
    
    echo -e "${GREEN}Detected: $OS/$ARCH${NC}"
}

download_release() {
    local url="https://github.com/$REPO/releases/download/v$VERSION/gcanner-$VERSION-$OS-$ARCH.tar.gz"
    local output="/tmp/gcanner-$VERSION-$OS-$ARCH.tar.gz"
    
    echo -e "${YELLOW}Downloading gcanner $VERSION for $OS/$ARCH...${NC}"
    
    if command -v curl >/dev/null; then
        curl -fsSL -o "$output" "$url" || { echo -e "${RED}Download failed${NC}"; exit 1; }
    elif command -v wget >/dev/null; then
        wget -q -O "$output" "$url" || { echo -e "${RED}Download failed${NC}"; exit 1; }
    else
        echo -e "${RED}curl or wget required${NC}"; exit 1
    fi
    
    echo "$output"
}

install_package() {
    local archive="$1"
    
    echo -e "${YELLOW}Installing to $INSTALL_DIR...${NC}"
    
    # Create directories
    sudo mkdir -p "$INSTALL_DIR"
    sudo mkdir -p "$BIN_DIR"
    
    # Extract
    sudo tar -xzf "$archive" -C "$INSTALL_DIR" --strip-components=1
    
    # Create launcher
    cat | sudo tee "$BIN_DIR/gcanner" > /dev/null << 'LAUNCHER_EOF'
#!/bin/bash
# gcanner launcher
export LD_LIBRARY_PATH="/opt/gcanner/lib:$LD_LIBRARY_PATH"
export GCANNER_DATA_DIR="/opt/gcanner/share/gcanner"
exec /opt/gcanner/bin/gcanner "$@"
LAUNCHER_EOF
    sudo chmod +x "$BIN_DIR/gcanner"
    
    # Create GUI launcher if exists
    if [[ -f "/opt/gcanner/bin/gcanner_gui" ]]; then
        cat | sudo tee "$BIN_DIR/gcanner-gui" > /dev/null << 'LAUNCHER_EOF'
#!/bin/bash
export LD_LIBRARY_PATH="/opt/gcanner/lib:$LD_LIBRARY_PATH"
export GCANNER_DATA_DIR="/opt/gcanner/share/gcanner"
exec /opt/gcanner/bin/gcanner_gui "$@"
LAUNCHER_EOF
        sudo chmod +x "$BIN_DIR/gcanner-gui"
    fi
    
    # Desktop entry
    sudo mkdir -p /usr/share/applications
    cat | sudo tee /usr/share/applications/gcanner.desktop > /dev/null << 'DESKTOP_EOF'
[Desktop Entry]
Name=gcanner
GenericName=Game System Requirements Analyzer
Comment=Analyze game directories for system requirements
Exec=gcanner-gui
Icon=gcanner
Terminal=false
Type=Application
Categories=Utility;Development;
Keywords=game;analyzer;requirements;hardware;
DESKTOP_EOF
    
    # Icon (if available)
    if [[ -f "/opt/gcanner/share/icons/gcanner.png" ]]; then
        sudo mkdir -p /usr/share/icons/hicolor/256x256/apps
        sudo cp /opt/gcanner/share/icons/gcanner.png /usr/share/icons/hicolor/256x256/apps/
        sudo gtk-update-icon-cache -f /usr/share/icons/hicolor 2>/dev/null || true
    fi
    
    # Update desktop database
    sudo update-desktop-database /usr/share/applications 2>/dev/null || true
}

verify_install() {
    if command -v gcanner >/dev/null; then
        echo -e "${GREEN}✓ gcanner installed successfully!${NC}"
        echo -e "Run: ${BLUE}gcanner /path/to/game${NC}"
        if command -v gcanner-gui >/dev/null; then
            echo -e "GUI: ${BLUE}gcanner-gui${NC}"
        fi
    else
        echo -e "${RED}Installation verification failed${NC}"
        exit 1
    fi
}

main() {
    print_banner
    detect_os
    
    # Check if running as root for system install
    if [[ $EUID -ne 0 ]]; then
        echo -e "${YELLOW}System install requires sudo. Re-running with sudo...${NC}"
        exec sudo bash "$0" "$@"
    fi
    
    local archive=$(download_release)
    install_package "$archive"
    verify_install
    
    echo -e "\n${GREEN}Installation complete!${NC}"
    echo -e "Documentation: ${BLUE}https://github.com/$REPO${NC}"
}

main "$@"