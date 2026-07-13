# Changelog

All notable changes to redline. Format loosely follows Keep a Changelog.

## [Unreleased]

### Added
- Project docs: `docs/DESIGN.md`, `NOTICE`, this changelog; redline README.
- CMake `DELTA_BUILD_DEMOS` option (default OFF) so delta-studio's large, unrelated
  demo projects and assets are excluded from the engine-sim build.
- Env-var-gated Metal frame capture (`REDLINE_CAPTURE` / `REDLINE_CAPTURE_FRAME` /
  `REDLINE_CAPTURE_SEQ=start,stride,count`): the app reads back its own drawable and dumps
  raw BGRA8 frame(s), so the render can be recorded/verified headlessly without macOS
  Screen-Recording permission.
- Env-gated demo auto-run (`REDLINE_AUTORUN=1`): cranks and revs the engine unattended —
  used to record the animated GIFs.
- Visual media (`media/`): animated hero + engine-closeup GIFs, before/after bug
  comparison, architecture diagram, app icon; a fully rewritten, illustrated README.
- **Double-clickable `redline.app` bundle** via `scripts/make_app.sh` (binary +
  assets + `es/` script library + engine fonts/shaders + `Info.plist` + ad-hoc
  codesign). The app now `chdir`s to its own directory at startup and resolves assets
  through a bundled `delta.conf`, so it launches from Finder as well as the CLI
  (the Apple port was command-line-only). Includes a tachometer app icon.

### Fixed (all three of the Apple port's known rendering bugs)
- **Off-screen window**: a fixed 1920x1080 window opened partly off-screen on smaller
  Retina displays. Now clamped (aspect-preserving) to the usable display bounds and
  centred.
- **Oversized engine view**: the app used its 1920x1080 *request* as the render
  resolution instead of the real Retina drawable. Now reports the actual
  `SDL_GetRendererOutputSize` (pixels) so projection/UI match what Metal draws.
- **Scrambled engine cross-section (valves/cam "look weird")**: the engine view's
  sub-render-target viewport was never applied — `CreateSubRenderTarget`,
  `ResizeRenderTarget`, and `SetRenderTarget` were stubs, and `SetRenderTarget`'s
  signature didn't even match the base the render loop calls. Implemented them to set
  a Metal viewport + scissor for the target's region; the engine now renders as a
  correctly-sized V8 confined to its centre region.

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
