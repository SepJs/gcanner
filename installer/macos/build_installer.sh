#!/bin/bash
# macOS package builder for gcanner
# Creates .dmg and .pkg installers

set -e

APP_NAME="gcanner"
APP_VERSION="1.0.0"
BUNDLE_ID="org.gcanner.gcanner"

echo "Building gcanner macOS installer..."

# Build the app bundle
echo "Building application bundle..."
cmake --build build --config Release --target gcanner_gui

# Create .app bundle structure
APP_BUNDLE="build/${APP_NAME}.app"
mkdir -p "${APP_BUNDLE}/Contents/MacOS"
mkdir -p "${APP_BUNDLE}/Contents/Resources"
mkdir -p "${APP_BUNDLE}/Contents/Frameworks"
mkdir -p "${APP_BUNDLE}/Contents/PlugIns"
mkdir -p "${APP_BUNDLE}/Contents/Resources/gcanner"

# Copy executable
cp build/src/gui/gcanner_gui "${APP_BUNDLE}/Contents/MacOS/"

# Copy Qt frameworks
echo "Copying Qt frameworks..."
QT_PATH=$(qmake -query QT_INSTALL_LIBS 2>/dev/null || echo "/opt/homebrew/lib")
for fw in QtCore QtGui QtWidgets QtCharts QtSvg; do
    if [[ -d "${QT_PATH}/${fw}.framework" ]]; then
        cp -R "${QT_PATH}/${fw}.framework" "${APP_BUNDLE}/Contents/Frameworks/"
    fi
done

# Copy plugins
QT_PLUGINS=$(qmake -query QT_INSTALL_PLUGINS 2>/dev/null || echo "/opt/homebrew/plugins")
cp -R "${QT_PLUGINS}" "${APP_BUNDLE}/Contents/PlugIns/"

# Create Info.plist
cat > "${APP_BUNDLE}/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>gcanner_gui</string>
    <key>CFBundleIdentifier</key>
    <string>${BUNDLE_ID}</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>gcanner</string>
    <key>CFBundleDisplayName</key>
    <string>gcanner</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>${APP_VERSION}</string>
    <key>CFBundleVersion</key>
    <string>${APP_VERSION}</string>
    <key>CFBundleIconFile</key>
    <string>AppIcon</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
    <key>CFBundleDocumentTypes</key>
    <array>
        <dict>
            <key>CFBundleTypeName</key>
            <string>gcanner Report</string>
            <key>CFBundleTypeRole</key>
            <string>Viewer</string>
            <key>LSHandlerRank</key>
            <string>Owner</string>
            <key>LSItemContentTypes</key>
            <array>
                <string>public.json</string>
                <string>org.gcanner.report</string>
            </array>
        </dict>
    </array>
    <key>UTExportedTypeDeclarations</key>
    <array>
        <dict>
            <key>UTTypeIdentifier</key>
            <string>org.gcanner.report</string>
            <key>UTTypeDescription</key>
            <string>gcanner System Requirements Report</string>
            <key>UTTypeConformsTo</key>
            <array>
                <string>public.json</string>
            </array>
            <key>UTTypeTagSpecification</key>
            <dict>
                <key>public.filename-extension</key>
                <string>gcr</string>
                <key>public.mime-type</key>
                <string>application/x-gcanner-report</string>
            </dict>
        </dict>
    </array>
</dict>
</plist>
EOF

# Copy icon
if [[ -f "src/gui/icons/app_icon.svg" ]]; then
    # Convert to icns
    mkdir -p "${APP_BUNDLE}/Contents/Resources/AppIcon.iconset"
    for size in 16 32 64 128 256 512 1024; do
        rsvg-convert -w $size -h $size src/gui/icons/app_icon.svg -o "${APP_BUNDLE}/Contents/Resources/AppIcon.iconset/icon_${size}x${size}.png" 2>/dev/null || \
        convert src/gui/icons/app_icon.svg -resize ${size}x${size} "${APP_BUNDLE}/Contents/Resources/AppIcon.iconset/icon_${size}x${size}.png" 2>/dev/null || true
    done
    iconutil -c icns "${APP_BUNDLE}/Contents/Resources/AppIcon.iconset" -o "${APP_BUNDLE}/Contents/Resources/AppIcon.icns" 2>/dev/null || true
fi

# Copy data files
cp -r data/* "${APP_BUNDLE}/Contents/Resources/gcanner/" 2>/dev/null || true

# Fix library paths with install_name_tool
echo "Fixing library paths..."
for lib in "${APP_BUNDLE}/Contents/Frameworks"/*.framework/Versions/*/*; do
    if [[ -f "$lib" && ! -L "$lib" ]]; then
        install_name_tool -id "@rpath/$(basename $(dirname $(dirname $lib)))/$(basename $lib)" "$lib" 2>/dev/null || true
    fi
done

# Fix executable rpaths
install_name_tool -add_rpath "@executable_path/../Frameworks" "${APP_BUNDLE}/Contents/MacOS/gcanner_gui" 2>/dev/null || true
install_name_tool -add_rpath "@loader_path/../Frameworks" "${APP_BUNDLE}/Contents/MacOS/gcanner_gui" 2>/dev/null || true

# Sign the app (requires Apple Developer ID)
# codesign --deep --force --verify --verbose --sign "Developer ID Application: Your Name" "${APP_BUNDLE}"

# Create DMG
echo "Creating DMG..."
DMG_NAME="${APP_NAME}-${APP_VERSION}-macos.dmg"
DMG_TEMP=$(mktemp -d)
mkdir -p "${DMG_TEMP}"
cp -R "${APP_BUNDLE}" "${DMG_TEMP}/"
ln -s /Applications "${DMG_TEMP}/Applications"

# Create background image for DMG (optional)
# hdiutil create -volname "gcanner ${APP_VERSION}" -srcfolder "${DMG_TEMP}" -ov -format UDZO "${DMG_NAME}"

# Use create-dmg if available
if command -v create-dmg &> /dev/null; then
    create-dmg \
        --volname "gcanner ${APP_VERSION}" \
        --volicon "src/gui/icons/app_icon.icns" \
        --window-pos 200 120 \
        --window-size 600 400 \
        --icon-size 100 \
        --icon "gcanner.app" 150 190 \
        --hide-extension "gcanner.app" \
        --app-drop-link 400 185 \
        "${DMG_NAME}" \
        "${DMG_TEMP}"
else
    hdiutil create -volname "gcanner ${APP_VERSION}" -srcfolder "${DMG_TEMP}" -ov -format UDZO "${DMG_NAME}"
fi

# Create PKG installer
echo "Creating PKG installer..."
PKG_DIR=$(mktemp -d)
mkdir -p "${PKG_DIR}/scripts"
mkdir -p "${PKG_DIR}/root/Applications"
cp -R "${APP_BUNDLE}" "${PKG_DIR}/root/Applications/"

# Preinstall script
cat > "${PKG_DIR}/scripts/preinstall" << 'EOF'
#!/bin/bash
# Remove old version if exists
if [[ -d "/Applications/gcanner.app" ]]; then
    rm -rf "/Applications/gcanner.app"
fi
EOF
chmod +x "${PKG_DIR}/scripts/preinstall"

# Postinstall script
cat > "${PKG_DIR}/scripts/postinstall" << 'EOF'
#!/bin/bash
# Update Launch Services database
/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister -f "/Applications/gcanner.app" 2>/dev/null || true
EOF
chmod +x "${PKG_DIR}/scripts/postinstall"

# Build pkg
pkgbuild --root "${PKG_DIR}/root" \
    --scripts "${PKG_DIR}/scripts" \
    --identifier "${BUNDLE_ID}" \
    --version "${APP_VERSION}" \
    --install-location "/" \
    "${APP_NAME}-${APP_VERSION}.pkg"

# Create product archive for notarization
productbuild --package "${APP_NAME}-${APP_VERSION}.pkg" \
    --distribution Distribution.xml \
    --resources Resources \
    "${APP_NAME}-${APP_VERSION}-installer.pkg"

# Distribution.xml
cat > Distribution.xml << EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>gcanner ${APP_VERSION}</title>
    <organization>gcanner</organization>
    <domains enable_anywhere="true" enable_currentUserHome="true" enable_localSystem="true"/>
    <options customize="never" require-scripts="false" rootVolumeOnly="false"/>
    <welcome file="welcome.html" mime-type="text/html"/>
    <license file="license.html" mime-type="text/html"/>
    <conclusion file="conclusion.html" mime-type="text/html"/>
    <pkg-ref id="${BUNDLE_ID}"/>
    <choices-outline>
        <line choice="default">
            <line choice="${BUNDLE_ID}"/>
        </line>
    </choices-outline>
    <choice id="default"/>
    <choice id="${BUNDLE_ID}" visible="false">
        <pkg-ref id="${BUNDLE_ID}"/>
    </choice>
    <pkg-ref id="${BUNDLE_ID}" version="${APP_VERSION}" onConclusion="none">${APP_NAME}-${APP_VERSION}.pkg</pkg-ref>
</installer-gui-script>
EOF

# Welcome, License, Conclusion HTML
cat > welcome.html << 'EOF'
<!DOCTYPE html>
<html><body style="font-family: -apple-system; padding: 20px;">
<h1>Welcome to gcanner Installer</h1>
<p>This will install gcanner ${APP_VERSION} on your Mac.</p>
<p>gcanner is a premium game system requirements analyzer that scans game directories and generates detailed hardware requirement reports.</p>
</body></html>
EOF

cat > license.html << 'EOF'
<!DOCTYPE html>
<html><body style="font-family: -apple-system; padding: 20px;">
<h1>License Agreement</h1>
<p>MIT License</p>
<p>Copyright (c) 2024-2026 gcanner contributors</p>
<p>Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:</p>
<p>The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.</p>
<p>THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.</p>
</body></html>
EOF

cat > conclusion.html << 'EOF'
<!DOCTYPE html>
<html><body style="font-family: -apple-system; padding: 20px;">
<h1>Installation Complete</h1>
<p>gcanner has been successfully installed!</p>
<p>You can find gcanner in your Applications folder.</p>
</body></html>
EOF

# Cleanup
rm -rf "${DMG_TEMP}" "${PKG_DIR}" Distribution.xml welcome.html license.html conclusion.html

echo "Done! Created:"
ls -la *.dmg *.pkg 2>/dev/null || true