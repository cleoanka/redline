# The Metal backend in delta-studio

engine-sim renders through Ange Yaghi's engine, **delta-studio**, which abstracts the GPU
behind a `ysDevice` interface with one implementation per API (`yds_d3d11_*`, `yds_opengl_*`,
`yds_vulkan_*`). The Apple-Silicon port ([boxofbox](https://github.com/boxofbox/engine-sim-APPLE_ARM))
added a **Metal** family, `yds_metal_*`, which `redline` continues. This documents how it works
and what `redline` changed.

## Binding: metal-cpp, not Objective-C

The backend uses Apple's official **[metal-cpp](https://developer.apple.com/metal/cpp/)** C++
bindings (`MTL::`, `NS::`, `CA::`), so the files are plain `.cpp` — no `.mm`. One translation
unit (`yds_metal_device.cpp`) defines `CA_/MTL_/NS_PRIVATE_IMPLEMENTATION` to emit the wrapper
symbols. metal-cpp objects *are* their underlying Obj-C `id`, which lets us reach the few
unbound selectors via the Obj-C runtime when needed (e.g. `setFramebufferOnly:` on the layer).

## File map (`yds_metal_*`)

| File | Wraps |
| :-- | :-- |
| `yds_metal_device` | `MTL::Device`, `MTL::CommandQueue`, the per-frame `RenderCommandEncoder`, depth-stencil state — implements the whole `ysDevice` interface |
| `yds_metal_context` | thin — the swapchain lives on the device |
| `yds_metal_gpu_buffer` | vertex/index buffers (`MTL::Buffer`) + inline constant data |
| `yds_metal_input_layout` | `MTL::VertexDescriptor` from a `ysRenderGeometryFormat` |
| `yds_metal_shader` / `_shader_program` | `MTL::Library` + `MTL::Function`, pipeline state |
| `yds_metal_render_target` | on-screen (drawable) + subdivision (viewport) targets |
| `yds_metal_texture` | `MTL::Texture` + sampler |

Registration is via delta-studio's `ysSubclassRegistry<DeviceAPI, ysDevice>`; the device
registers for a `DeviceAPI::Metal` enum value and is created by
`ysDevice::CreateDevice(&dev, DeviceAPI::Metal)`.

## Frame lifecycle

1. **`CreateRenderingContext`** grabs the `CA::MetalLayer` from SDL2
   (`SDL_RenderGetMetalLayer`), builds the command queue, the depth texture, and the first
   `RenderPassDescriptor` + `RenderCommandEncoder`.
2. Each frame the app records draws into the live encoder.
3. **`Present`** ends the encoder, presents the drawable, commits, then acquires the next
   drawable and starts a fresh command buffer + encoder for the following frame.

Constant buffers are pushed inline with `setVertexBytes` / `setFragmentBytes` at bind time
(so the app's edit-then-bind-per-draw pattern gives each draw its own uniforms). Shaders are
**MSL**, loaded from `engines/basic/shaders/msl/` and compiled at runtime with
`newLibrary(source)` — no build-time `.metallib` step, and therefore no full-Xcode dependency.

## What redline changed

### Sub-render-target viewports (the engine-view fix)
engine-sim draws the engine into a **subdivision** render target — a sub-region of the screen —
while the UI is drawn to the full screen. In the Metal backend `CreateSubRenderTarget`,
`ResizeRenderTarget`, and `SetRenderTarget` were all stubs, and `SetRenderTarget(target)` didn't
match the base `SetRenderTarget(target, slot)` the render loop calls — so it was dead code and
no viewport was ever set. The engine, projected for its little box, was rasterised across the
whole drawable, on top of the gauges.

`redline` implements them: a subdivision target stores its region + parent, and
`SetRenderTarget` sets a `MTL::Viewport` (+ a clamped scissor) to that region. Metal shares
DirectX's top-left origin, and the app already stores `posY` in top-left space (it pre-flips in
`RepositionRenderTarget`), so no Y-flip is needed — unlike the OpenGL backend.

### HiDPI sizing
The window is clamped (aspect-preserving) to the display's usable bounds and centred, and the
**actual** Metal drawable size (`SDL_GetRendererOutputSize`, in pixels) is reported as the screen
resolution — so the app's projection matches what Metal renders instead of assuming 1920×1080.

### Headless frame capture
`Present` can read back its own drawable (blit → shared `MTL::Texture` → `getBytes`) and dump raw
BGRA8, gated by `REDLINE_CAPTURE*`. This is how the render is verified and the demo GIFs are
recorded without macOS Screen-Recording permission. The drawable is made readable with
`setFramebufferOnly:NO` sent through the Obj-C runtime (metal-cpp doesn't bind it).

## Known simplifications

- Off-screen render targets (`CreateOffScreenRenderTarget`) remain unimplemented — engine-sim's
  macOS path doesn't need them (it uses on-screen + subdivision targets only).
- Depth state is a single always-on less/write configuration; the UI happens to layer correctly
  under it. `SetDepthTestEnabled` is a no-op.
- The OpenGL backend is still compiled into `delta-core` on macOS (behind
  `GL_SILENCE_DEPRECATION`); only the Metal path is used at runtime.
