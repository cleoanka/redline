# Changelog

All notable changes to redline. Format loosely follows Keep a Changelog.

## [Unreleased]

### Added
- Project docs: `docs/DESIGN.md`, `NOTICE`, this changelog; redline README.
- CMake `DELTA_BUILD_DEMOS` option (default OFF) so delta-studio's large, unrelated
  demo projects and assets are excluded from the engine-sim build.

### Fixed (toolchain revival — builds on macOS 26 Tahoe / CMake 4.2 / Apple clang 21 / Boost 1.90)
- **Boost 1.90**: `piranha` used `boost::filesystem::path::is_complete()`, removed in
  modern Boost → replaced with `is_absolute()`.
- **flex 2.6.4**: the vendored `FlexLexer.h` declared `LexerInput/LexerOutput` with
  `size_t` signatures while the installed flex generates `int` ones, breaking the
  generated scanner → vendored header re-aligned to the toolchain's flex.
- **libc++ 21**: `yds_opengl_device.cpp` used `assert` without `<cassert>` (no longer
  transitively included) → added the include.

### Removed
- Windows-only prebuilt libraries (Vulkan `shaderc`/`SPIRV-Tools`, D3DX, DXGI,
  DirectSound) and delta-studio demo assets — unused by the macOS build; trims the repo
  from ~1.1 GB to ~35 MB.

## [0.1.0] — bring-up
- Forked from boxofbox/engine-sim-APPLE_ARM; established the lean, self-contained
  macOS tree and got `engine-sim-app` building and launching on Apple Silicon.
