#!/bin/sh
# M6-03: wrap the macOS binary into a drag-and-drop application bundle.
#   make vesperis && sh packaging/make_app.sh            -> dist/Vesperis.app
set -e
cd "$(dirname "$0")/.."
BIN=${1:-vesperis}
APP=dist/Vesperis.app
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
cp "$BIN" "$APP/Contents/MacOS/vesperis"
# the game writes its saves, guide and shots into the working directory: start it from Documents/vesperis
cat > "$APP/Contents/MacOS/launch" <<'LAUNCH'
#!/bin/sh
cd "$HOME/Documents" 2>/dev/null || cd "$HOME"
mkdir -p vesperis && cd vesperis
exec "$(dirname "$0")/vesperis" "$@"
LAUNCH
chmod +x "$APP/Contents/MacOS/launch" "$APP/Contents/MacOS/vesperis"
cat > "$APP/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleName</key><string>Vesperis</string>
  <key>CFBundleDisplayName</key><string>Vesperis</string>
  <key>CFBundleIdentifier</key><string>org.vesperis.game</string>
  <key>CFBundleVersion</key><string>1.0.0</string>
  <key>CFBundleShortVersionString</key><string>1.0</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleExecutable</key><string>launch</string>
  <key>NSHighResolutionCapable</key><true/>
  <key>LSMinimumSystemVersion</key><string>11.0</string>
</dict></plist>
PLIST
# bundle the Homebrew raylib dylib when the binary links it dynamically
LIB=$(otool -L "$BIN" 2>/dev/null | grep -o '/[^ ]*libraylib[^ ]*dylib' | head -1 || true)
if [ -n "$LIB" ] && [ -f "$LIB" ]; then
  cp "$LIB" "$APP/Contents/MacOS/"
  install_name_tool -change "$LIB" "@executable_path/$(basename "$LIB")" "$APP/Contents/MacOS/vesperis"
fi
cp README.md "$APP/Contents/Resources/"
echo "built $APP"
