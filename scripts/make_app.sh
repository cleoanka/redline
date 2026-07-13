#!/usr/bin/env bash
# Assemble redline.app (a double-clickable macOS bundle) from a compiled engine-sim-app.
#
# Usage: scripts/make_app.sh [build_dir] [output_dir]
#   build_dir  defaults to ./build
#   output_dir defaults to ./dist
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${1:-$ROOT/build}"
OUT="${2:-$ROOT/dist}"
APP="$OUT/redline.app"
BIN="$BUILD/engine-sim-app"

if [ ! -x "$BIN" ]; then
    echo "error: $BIN not found — build engine-sim-app first:" >&2
    echo "  cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DDISCORD_ENABLED=OFF" >&2
    echo "  cmake --build build --target engine-sim-app -j" >&2
    exit 1
fi

echo "Assembling $APP"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources/engines/basic"

cp "$BIN" "$APP/Contents/MacOS/redline"
cp -R "$ROOT/assets" "$APP/Contents/Resources/assets"
# es/ is the engine-script library (engine_sim.mr + its imports); main.mr pulls it in via
# the compiler search path "../Resources/es/".
cp -R "$ROOT/es" "$APP/Contents/Resources/es"
cp -R "$ROOT/dependencies/submodules/delta-studio/engines/basic/fonts"   "$APP/Contents/Resources/engines/basic/fonts"
cp -R "$ROOT/dependencies/submodules/delta-studio/engines/basic/shaders" "$APP/Contents/Resources/engines/basic/shaders"

# delta.conf sits next to the executable. The app chdir's to its own directory at startup,
# so GetModulePath() (== CWD) is Contents/MacOS/; these paths redirect into Resources/.
#   line 1: engine base path (fonts + shaders)
#   line 2: asset path (main.mr, sound-library, themes, ...)
printf '../Resources/engines/basic\n../Resources/assets\n' > "$APP/Contents/MacOS/delta.conf"

cat > "$APP/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key><string>redline</string>
  <key>CFBundleDisplayName</key><string>redline</string>
  <key>CFBundleIdentifier</key><string>com.cleoanka.redline</string>
  <key>CFBundleVersion</key><string>0.1.0</string>
  <key>CFBundleShortVersionString</key><string>0.1.0</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleExecutable</key><string>redline</string>
  <key>CFBundleIconFile</key><string>AppIcon</string>
  <key>NSHighResolutionCapable</key><true/>
  <key>LSMinimumSystemVersion</key><string>13.0</string>
  <key>NSHumanReadableCopyright</key><string>MIT. Fork of engine-sim (c) 2022 Ange Yaghi.</string>
</dict>
</plist>
PLIST

# Optional icon.
if [ -f "$ROOT/packaging/AppIcon.icns" ]; then
    cp "$ROOT/packaging/AppIcon.icns" "$APP/Contents/Resources/AppIcon.icns"
fi

# Ad-hoc codesign so Gatekeeper allows local launch (no Developer ID needed).
if ! codesign --force --deep --sign - "$APP" >/dev/null 2>&1; then
    echo "warning: ad-hoc codesign failed (the app may still run locally)" >&2
fi

echo "Built $APP"
echo "Run it:  open \"$APP\"   (or double-click in Finder)"
