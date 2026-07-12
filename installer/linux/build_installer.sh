#!/bin/bash
# gcanner Linux Installer Build Script
# Creates AppImage, DEB, and RPM packages

set -e

APP_NAME="gcanner"
APP_VERSION="1.0.0"
APP_BINARY="gcanner_gui"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}Building gcanner Linux installers...${NC}"

# Build first
cd ..
cmake --build build --config Release --target gcanner_gui

# Create staging directory
STAGING=$(mktemp -d)
echo "Staging: $STAGING"

# AppDir structure for AppImage
APPDIR="$STAGING/$APP_NAME.AppDir"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/lib"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$APPDIR/usr/share/$APP_NAME"

# Copy binary
cp "build/src/gui/$APP_BINARY" "$APPDIR/usr/bin/"

# Copy Qt libraries (using linuxdeployqt or manual)
if command -v linuxdeployqt &> /dev/null; then
    linuxdeployqt "$APPDIR/usr/bin/$APP_BINARY" -appimage -qmldir=src/gui
else
    # Manual copy of Qt libs
    QT_LIBS=$(ldd "build/src/gui/$APP_BINARY" | grep -oP '/usr/lib.*?\.so\.\d+' | sort -u)
    for lib in $QT_LIBS; do
        cp "$lib" "$APPDIR/usr/lib/"
    done
fi

# Desktop file
cat > "$APPDIR/usr/share/applications/$APP_NAME.desktop" << EOF
[Desktop Entry]
Type=Application
Name=gcanner
GenericName=Game System Requirements Analyzer
Comment=Scan game directories and generate system requirement reports
Exec=$APP_BINARY
Icon=$APP_NAME
Terminal=false
Categories=Utility;Development;
MimeType=application/json;
StartupNotify=true
EOF

# Icon
if [[ -f "src/gui/icons/app_icon.svg" ]]; then
    # Convert SVG to PNG if needed
    if command -v convert &> /dev/null; then
        convert "src/gui/icons/app_icon.svg" -resize 256x256 "$APPDIR/usr/share/icons/hicolor/256x256/apps/$APP_NAME.png"
    else
        cp "src/gui/icons/app_icon.svg" "$APPDIR/usr/share/icons/hicolor/scalable/apps/$APP_NAME.svg"
    fi
fi

# AppRun script
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$HERE/usr/lib/qt6/plugins:$QT_PLUGIN_PATH"
export QML2_IMPORT_PATH="$HERE/usr/lib/qt6/qml:$QML2_IMPORT_PATH"
exec "$HERE/usr/bin/gcanner_gui" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# Copy data files
cp -r data "$APPDIR/usr/share/$APP_NAME/" 2>/dev/null || true

# Build AppImage
if command -v appimagetool &> /dev/null; then
    echo -e "${YELLOW}Building AppImage...${NC}"
    export ARCH=x86_64
    appimagetool "$APPDIR" "$APP_NAME-$APP_VERSION-x86_64.AppImage"
    echo -e "${GREEN}AppImage created!${NC}"
else
    echo -e "${YELLOW}appimagetool not found, skipping AppImage${NC}"
fi

# Build DEB package
echo -e "${YELLOW}Building DEB package...${NC}"
DEB_DIR="$STAGING/deb"
mkdir -p "$DEB_DIR/DEBIAN"
mkdir -p "$DEB_DIR/usr/bin"
mkdir -p "$DEB_DIR/usr/share/applications"
mkdir -p "$DEB_DIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$DEB_DIR/usr/share/$APP_NAME"

cp "build/src/gui/$APP_BINARY" "$DEB_DIR/usr/bin/"
cp "$APPDIR/usr/share/applications/$APP_NAME.desktop" "$DEB_DIR/usr/share/applications/"
if [[ -f "$APPDIR/usr/share/icons/hicolor/256x256/apps/$APP_NAME.png" ]]; then
    cp "$APPDIR/usr/share/icons/hicolor/256x256/apps/$APP_NAME.png" "$DEB_DIR/usr/share/icons/hicolor/256x256/apps/"
fi
cp -r data "$DEB_DIR/usr/share/$APP_NAME/" 2>/dev/null || true

# Calculate installed size
INSTALLED_SIZE=$(du -sk "$DEB_DIR/usr" | cut -f1)

cat > "$DEB_DIR/DEBIAN/control" << EOF
Package: $APP_NAME
Version: $APP_VERSION
Section: utils
Priority: optional
Architecture: amd64
Installed-Size: $INSTALLED_SIZE
Maintainer: gcanner contributors <gcanner@gcanner.org>
Homepage: https://github.com/SepJs/gcanner
Description: Game System Requirements Analyzer
 gcanner scans game directories and analyzes assets to generate
 detailed system requirement reports including minimum, recommended,
 high, ultra, and ray-tracing tiers.
Depends: libc6 (>= 2.31), libstdc++6 (>= 10), libqt6core6 (>= 6.2), libqt6gui6 (>= 6.2), libqt6widgets6 (>= 6.2), libqt6charts6 (>= 6.2), libqt6svg6 (>= 6.2)
EOF

cat > "$DEB_DIR/DEBIAN/postinst" << 'EOF'
#!/bin/bash
update-desktop-database /usr/share/applications 2>/dev/null || true
gtk-update-icon-cache /usr/share/icons/hicolor 2>/dev/null || true
EOF
chmod +x "$DEB_DIR/DEBIAN/postinst"

dpkg-deb --build "$DEB_DIR" "$APP_NAME-${APP_VERSION}_amd64.deb"
echo -e "${GREEN}DEB package created!${NC}"

# Build RPM package
if command -v rpmbuild &> /dev/null; then
    echo -e "${YELLOW}Building RPM package...${NC}"
    RPM_DIR="$STAGING/rpm"
    mkdir -p "$RPM_DIR/BUILD"
    mkdir -p "$RPM_DIR/RPMS"
    mkdir -p "$RPM_DIR/SOURCES"
    mkdir -p "$RPM_DIR/SPECS"
    mkdir -p "$RPM_DIR/BUILDROOT"

    # Create source tarball
    tar -czf "$RPM_DIR/SOURCES/$APP_NAME-$APP_VERSION.tar.gz" -C "$APPDIR" .

    cat > "$RPM_DIR/SPECS/$APP_NAME.spec" << EOF
Name: $APP_NAME
Version: $APP_VERSION
Release: 1%{?dist}
Summary: Game System Requirements Analyzer
License: MIT
URL: https://github.com/SepJs/gcanner
Source0: %{name}-%{version}.tar.gz
BuildArch: x86_64
Requires: qt6-qtbase, qt6-qtcharts, qt6-qtsvg
BuildRequires: qt6-qtbase-devel, qt6-qtcharts-devel, qt6-qtsvg-devel

%description
gcanner scans game directories and analyzes assets to generate detailed
system requirement reports including minimum, recommended, high, ultra,
and ray-tracing tiers.

%prep
%autosetup

%build
# Already built

%install
mkdir -p %{buildroot}/usr/bin
mkdir -p %{buildroot}/usr/share/applications
mkdir -p %{buildroot}/usr/share/icons/hicolor/256x256/apps
mkdir -p %{buildroot}/usr/share/%{name}

cp -r * %{buildroot}/usr/share/%{name}/
install -m 755 usr/bin/gcanner_gui %{buildroot}/usr/bin/gcanner_gui
install -m 644 usr/share/applications/gcanner.desktop %{buildroot}/usr/share/applications/
install -m 644 usr/share/icons/hicolor/256x256/apps/gcanner.png %{buildroot}/usr/share/icons/hicolor/256x256/apps/

%files
/usr/bin/gcanner_gui
/usr/share/applications/gcanner.desktop
/usr/share/icons/hicolor/256x256/apps/gcanner.png
/usr/share/gcanner/

%post
update-desktop-database /usr/share/applications &>/dev/null || true
gtk-update-icon-cache /usr/share/icons/hicolor &>/dev/null || true

%postun
update-desktop-database /usr/share/applications &>/dev/null || true
gtk-update-icon-cache /usr/share/icons/hicolor &>/dev/null || true

%changelog
* $(date +"%a %b %d %Y") gcanner contributors <gcanner@gcanner.org> - $APP_VERSION-1
- Initial release
EOF

    rpmbuild --define "_topdir $RPM_DIR" -ba "$RPM_DIR/SPECS/$APP_NAME.spec"
    cp "$RPM_DIR/RPMS/x86_64/${APP_NAME}-${APP_VERSION}-1.x86_64.rpm" .
    echo -e "${GREEN}RPM package created!${NC}"
else
    echo -e "${YELLOW}rpmbuild not found, skipping RPM${NC}"
fi

# Create universal installer script
cat > "${APP_NAME}-${APP_VERSION}-linux-installer.sh" << 'EOF'
#!/bin/bash
# gcanner Linux Universal Installer
set -e

APP_NAME="gcanner"
INSTALL_DIR="/opt/gcanner"
BIN_DIR="/usr/local/bin"
DESKTOP_DIR="/usr/share/applications"
ICON_DIR="/usr/share/icons/hicolor/256x256/apps"

echo "Installing gcanner..."

# Check root
if [[ $EUID -ne 0 ]]; then
    echo "This installer requires root privileges. Please run with sudo."
    exit 1
fi

# Create directories
mkdir -p "$INSTALL_DIR"
mkdir -p "$BIN_DIR"
mkdir -p "$DESKTOP_DIR"
mkdir -p "$ICON_DIR"

# Copy files
cp gcanner_gui "$INSTALL_DIR/"
cp gcanner.desktop "$DESKTOP_DIR/"
cp gcanner.png "$ICON_DIR/" 2>/dev/null || true
cp -r data "$INSTALL_DIR/" 2>/dev/null || true

# Create launcher script
cat > "$BIN_DIR/gcanner" << 'LAUNCHER_EOF'
#!/bin/bash
exec /opt/gcanner/gcanner_gui "$@"
LAUNCHER_EOF
chmod +x "$BIN_DIR/gcanner"

# Update desktop database
update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
gtk-update-icon-cache "$ICON_DIR" 2>/dev/null || true

echo "gcanner installed successfully!"
echo "Run 'gcanner' from terminal or find it in your applications menu."
EOF

chmod +x "${APP_NAME}-${APP_VERSION}-linux-installer.sh"

# Cleanup
rm -rf "$STAGING"

echo -e "${GREEN}Done! Created packages:${NC}"
ls -la *.AppImage *.deb *.rpm *.sh 2>/dev/null | grep -v "^d" || true