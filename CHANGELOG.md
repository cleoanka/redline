# Changelog

All notable changes to redline. Format loosely follows Keep a Changelog.

## [Unreleased]

### Added
- **Xbox / SDL2 game-controller support** (`src/gamepad_input.*`): analog throttle (RT)
  and clutch (LT), A=starter, B=ignition, X=dyno, Y=reload, RB/LB=next/prev engine,
  D-pad=gears, sticks=volume/time-warp, Start=pause. Poll-based (immune to the shared
  SDL event queue), hot-plug via periodic re-scan, radial stick dead-zone; keyboard and
  pad compose.

### Fixed (adversarial audit hardening pass — a multi-agent + AddressSanitizer sweep)
- **Crash on minimize / alt-tab / Mission Control / resize:** `CAMetalLayer.nextDrawable()`
  returns null when the window is occluded; the Metal backend then dereferenced it. Now
  skips the frame cleanly (null-safe encoder, guarded draw path) and retries next frame,
  with a corrected frame-pacing semaphore that stays balanced across a skipped frame.
- **Shader/pipeline compile failure aborted (or null-deref'd in release)** via `assert(false)`
  → now returns an error and the draw is skipped.
- **Physics sign bug** in `gas_system.cpp` `pressureEquilibriumMaxFlow` (missing negation
  in the below-ambient branch; fixed upstream) → corrected.
- **Undefined behavior / heap issues:** scalar-`new` freed with `delete[]` in
  `ysErrorSystem::Destroy`; uninitialized `ysMetalGPUBuffer::m_buffer[3]`; `SetRenderTarget`
  out-of-bounds write before its slot check; `GetStrideOfFormat` fell off the end (+ wrong
  UInt sizes); per-glyph staging-buffer leak in `CreateAlphaTexture`.
- **Audio thread-safety:** the SDL audio-callback thread iterated the source list while the
  sim thread could reallocate it (use-after-free on engine start / gear change / switch) →
  `SDL_LockAudioDevice` around source add + data-buffer assignment. `AddToBuffer` could read
  past the source ring when the mix buffer was larger → rewritten with modulo indexing and a
  zero-size guard.
- **Fullscreen (F) and live resize were no-ops:** `F` now actually toggles SDL fullscreen,
  and the window reports its live drawable size so the projection tracks resizes instead of
  distorting.
- **Keymap:** `[`/`]` (engine switch) and top-row `1`–`5` (time-warp) were unmapped on
  laptops (keypad-only) → added. Division-by-zero guards on the engine-view bounds.

- **Random crash after a few seconds (heap corruption).** The Metal `ResizeRenderTarget`
  (called every frame from the engine-view camera path) ran `YDS_ERROR_DECLARE` — which
  pushes onto delta-studio's error call-stack — but returned without `YDS_ERROR_RETURN`,
  which pops. So `ysErrorSystem::m_stackLevel` leaked ~1 level/frame and overflowed
  `m_callStack` within seconds, corrupting the heap (surfacing as bad-access / malloc /
  SF-Symbol crashes, e.g. on window focus changes). Fixed the imbalance and hardened
  `StackRaise` with a bounds check so no future raise/descend imbalance can corrupt the
  heap. Verified with AddressSanitizer (30 s clean; was <5 s to overflow) and a 45 s
  idle+focus-change stress test.

### Added
- **In-app engine switcher** — `[` and `]` cycle through 16 built-in engines at runtime
  (Chevy 454 V8, Hayabusa I4, VTEC, EJ25, 2JZ, LS/Ferrari V8s, LFA V10, Merlin V12,
  radials, V-twins, …). Rewrites `main.mr` with the chosen preset and hot-reloads it,
  bracketed by audio stop/start so the audio thread never touches an engine being rebuilt.
  Env-gated `REDLINE_CYCLE=<frames>` auto-cycles (for testing the reload path).
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
