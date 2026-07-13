# redline

**A maintained, native-Metal build of AngeTheGreat's combustion-engine simulator for
Apple-Silicon macOS.**

`redline` is a fork of [`engine-sim`](https://github.com/ange-yaghi/engine-sim) — a
real-time internal-combustion-engine simulation that generates realistic engine audio
from a physical model of gas dynamics, combustion, and the valvetrain. The original is
Windows-only (DirectX 11). redline runs it on Apple Silicon through a **native Metal
renderer** (Apple's `metal-cpp` bindings inside `delta-studio`), with SDL2 for
windowing, input, and audio.

> It's a toy for making engine sounds and exploring engine response — not a scientific
> engineering tool.

## Why this fork

A Metal port already existed ([boxofbox/engine-sim-APPLE_ARM](https://github.com/boxofbox/engine-sim-APPLE_ARM))
but went dormant in early 2023: it no longer builds on a modern macOS toolchain, and it
shipped with three known rendering bugs. redline **revives it for macOS 26 (Tahoe) /
CMake 4 / Apple clang 21 / Boost 1.90**, fixes those bugs, and develops it further. See
[`docs/DESIGN.md`](docs/DESIGN.md) and [`CHANGELOG.md`](CHANGELOG.md).

## Status

- ✅ Builds & runs on Apple Silicon, macOS 26 (Tahoe), CMake 4.2, Apple clang 21.
- ✅ All three inherited rendering bugs fixed — off-screen window, oversized engine
  view, and the scrambled engine cross-section (see `docs/screenshot_current.png`).
- ✅ Double-clickable `redline.app` bundle with an app icon (`scripts/make_app.sh`).

## Build (Apple Silicon, macOS)

```sh
# arm64 Homebrew (/opt/homebrew)
brew install cmake boost bison sse2neon sdl2 sdl2_image flex

git clone https://github.com/cleoanka/redline.git
cd redline
export PATH="/opt/homebrew/opt/bison/bin:/opt/homebrew/opt/flex/bin:/opt/homebrew/bin:$PATH"
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DDISCORD_ENABLED=OFF
cmake --build build --target engine-sim-app -j

cd build && ./engine-sim-app     # run from build/ so it finds ../assets
```

### Package as a double-clickable app

```sh
scripts/make_app.sh              # -> dist/redline.app
open dist/redline.app            # or double-click it in Finder
```

## Controls

Minimal, keyboard-driven (unchanged from upstream):

| Key | Action | Key | Action |
| :--: | :-- | :--: | :-- |
| A | Toggle ignition | S | Hold for starter |
| D | Enable dyno | H | RPM hold (needs dyno) |
| Q W E R | Throttle presets | Space | Fine throttle (+scroll) |
| ↑ / ↓ | Shift gear up/down | Shift | Clutch |
| Z/X/C/V/B + scroll | Audio mix | Tab | Change screen |
| M / , | View layer up/down | Enter | Reload engine script |
| F | Fullscreen | Esc | Quit |

## Credits & lineage

redline stands on a chain of open-source work — thank you to everyone in it:

- **[ange-yaghi/engine-sim](https://github.com/ange-yaghi/engine-sim)** — the original
  simulator and the `delta-studio` engine. © 2022 Ange Yaghi (MIT).
- **[phire/delta-studio @ clang_linux](https://github.com/phire/delta-studio/tree/clang_linux)**
  — DirectX→OpenGL, MSVC→clang.
- **[bobsayshilol/engine-sim @ gcc-fixes](https://github.com/bobsayshilol/engine-sim/tree/gcc-fixes)**
  — gcc/clang portability.
- **[boxofbox/engine-sim-APPLE_ARM](https://github.com/boxofbox/engine-sim-APPLE_ARM)**
  — the Apple-Silicon port with the native `metal-cpp` Metal backend redline continues.

## License

MIT — see [`LICENSE`](LICENSE) and [`NOTICE`](NOTICE). Upstream copyright preserved.
