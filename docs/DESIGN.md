# redline — design & status

**Started:** 2026-07-13
**Goal:** A maintained, native **Metal** build of AngeTheGreat's `engine-sim` for
current Apple-Silicon macOS — building cleanly on a modern toolchain, with the
long-standing rendering bugs fixed, packaged as a real `.app`, and developed further.

## Lineage (how the code got here)

`engine-sim` renders through Ange Yaghi's own engine, `delta-studio`, which was
DirectX-only. Getting it onto Apple hardware took a chain of open-source work:

1. **ange-yaghi/engine-sim** — original simulator (Windows / DirectX 11).
2. **phire/delta-studio @ clang_linux** — DirectX→OpenGL, MSVC→clang.
3. **bobsayshilol/engine-sim @ gcc-fixes** — gcc/clang portability.
4. **boxofbox/engine-sim-APPLE_ARM** — Apple-Silicon port that added a **native Metal
   backend to `delta-studio`** (`yds_metal_*`, using Apple's `metal-cpp` bindings),
   SDL2 windowing/audio, `sse2neon`. Built on macOS Ventura 13 in Jan 2023, then went
   dormant.

`redline` continues from (4). The Metal renderer already existed — but it did not build
on a 2026 toolchain, and it shipped with three known rendering bugs. redline's job is to
**revive, fix, and extend** it.

## Architecture

```
.mr script → piranha → Engine model → Simulator ─┬─► audio samples ─► SDL2 audio out
  (bison/flex parser)   (portable C++)            └─► 2D geometry / UI ─► delta-studio
                                                        ─► Metal (yds_metal_*) ─► CAMetalLayer ─► screen
SDL2 events (keyboard / mouse / window) ─────────► application
```

- **Simulation + audio synthesis** — portable C++ (`src/*.cpp`), untouched.
- **Scripting** — `piranha` (`.mr` engine definitions), needs bison + flex.
- **Renderer** — `delta-studio`'s `yds_metal_*` backend via **metal-cpp**; swapchain is
  the `CAMetalLayer` from SDL2's renderer (`SDL_RenderGetMetalLayer`).
- **Windowing / input / audio** — SDL2 (`sdl2-compat`).

## What redline changes

### Toolchain revival (done — builds on macOS 26 / CMake 4.2 / Apple clang 21 / Boost 1.90)
- `boost::filesystem::path::is_complete()` → `is_absolute()` (removed in Boost 1.90).
- Vendored `FlexLexer.h` re-aligned to the installed flex 2.6.4 (`int` vs `size_t`
  `LexerInput`/`LexerOutput` signature clash).
- `#include <cassert>` in `yds_opengl_device.cpp` (libc++ 21 no longer transitively
  provides it).
- CMake configured with `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` for the pre-3.5
  sub-projects; demos gated behind `DELTA_BUILD_DEMOS` (off by default) and their large
  unrelated assets dropped from the tree.

### Rendering-bug fixes (in progress)
The Apple port's README listed three bugs, all rooted in HiDPI point-vs-pixel handling:
1. **Engine view is huge** — projection/viewport uses one coordinate space, the drawable
   another (Retina 2× backing scale).
2. **Window runs off the edge** — a fixed 1920×1080 *point* window exceeds a 14″ MBP's
   1512×982-point display.
3. **Valves / cam lobes drawn wrong** — geometry/primitive artifact in the engine
   cross-section.

### Packaging + features (planned)
Double-clickable `.app` (icon, bundled assets, Retina, ad-hoc codesign), then new
features specced separately.

## Verification

Build clean → launch → engine audible → render matches a correct reference (screenshot
diff). Renderer C++/Metal changes are cross-checked with a second model (codex).

## License

MIT, inherited from `engine-sim`. Upstream copyright preserved; full attribution in the
README and `NOTICE`.
