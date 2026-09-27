# Camelot roadmap

Goal: a desktop viewer that opens MCAP recordings and renders their contents in
3D on a custom Vulkan renderer. First-class data: point clouds, line sets
(lanes, paths), and simple markers (boxes, spheres, text labels). Think of a
small Foxglove/RViz-style 3D panel, without the web stack.

Layering stays as it is today:

- **Avalon** – Vulkan engine. Knows nothing about MCAP or scene semantics.
- **Camelot** – application model: scene, playback, data ingestion. Talks to
  Avalon through a small renderer API.
- **Pendragon** – the executable (window loop, argument parsing).

## Milestones

| # | Milestone | Outcome |
|---|-----------|---------|
| M1 | First frame | Window stays open, clears to a colour every frame, survives resize. |
| M2 | Triangle | Shader toolchain, render pass, pipeline manager, one hard-coded triangle. |
| M3 | Camera | Orbit camera with mouse input, MVP uniform buffer, descriptor sets. |
| M4 | Points & lines | Point-cloud and line-list pipelines fed from GPU vertex buffers. |
| M5 | Scene model | Renderer draws from a scene of primitives owned by Camelot, not from test data. |
| M6 | MCAP | Reader turns Foxglove-schema messages into scene primitives; timeline playback. |
| M7 | UI | ImGui overlay: topic list, timeline scrubber, camera reset. |

## Tickets

Each ticket is meant to be a single PR against `develop`. "AC" = acceptance
criteria. Existing GitHub issue numbers are noted where one already exists.

### T1 – Render loop, sync and clear-screen presentation (M1)

Why: nothing draws yet; every later ticket needs a frame loop.

Scope:
- `Window`: `shouldClose()`, `pollEvents()`, `getFramebufferSize()`, resize flag via GLFW callback.
- `SwapchainModel`: expose image views, extent, format; build with the real framebuffer size; own framebuffers for a given render pass; `recreate()` on resize.
- `RenderPass`: MSAA colour + depth attachment, resolve to the swapchain image.
- `CommandPool` (graphics queue family) and per-frame command buffers.
- `FrameSync`: image-available / render-finished semaphores and in-flight fence, two frames in flight.
- `Renderer::drawFrame()`: acquire → record (begin pass with clear values, end pass) → submit → present; handles `VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR` by recreating.
- `Avalon::drawFrame()` / `MainModel::run()` / Pendragon loop until the window closes.

AC: Pendragon shows a window that stays open with a solid clear colour, resizing does not crash or trigger validation errors, closing exits 0. A GoogleTest draws three frames on the mock ICD.

### T2 – Shader build pipeline (M2)

Why: pipelines need SPIR-V; hand-compiled binaries in git rot.

Scope: add `glslang` (or `shaderc`) via Conan; CMake function `camelot_add_shaders()` that compiles `*.vert`/`*.frag` under `Avalon/shaders/` to SPIR-V into the build tree and embeds or installs them; `ShaderModule` RAII wrapper that loads a SPIR-V file.

AC: touching a GLSL file rebuilds only the affected SPIR-V; missing shader is a configure-time error; unit test loads a compiled module.

### T3 – PipelineManager and graphics pipeline base; triangle (M2) — issues #27, #28

Scope: `PipelineLayout` and `GraphicsPipeline` wrappers with a builder for vertex input, topology, rasterization, MSAA, depth state; `PipelineManager` that caches pipelines by name; a `triangle` pipeline with vertices generated in the vertex shader; `Renderer` records `vkCmdDraw(3)`.

AC: a triangle is visible; pipeline creation failures throw with the pipeline name; test builds the pipeline on the mock ICD.

### T4 – Camera, uniform buffers, descriptor sets (M3)

Scope: `Camera` (orbit: yaw/pitch/distance/target, perspective projection, GLM via Conan); `UniformBuffer` on top of `Buffer` (host-visible, per frame in flight); `DescriptorPool`/`DescriptorSetLayout` wrappers; GLFW mouse/scroll input routed from `Window` to the camera; the triangle now moves with the camera.

AC: dragging orbits, scrolling zooms, window resize keeps aspect ratio correct; test round-trips a view-projection matrix through the uniform buffer.

### T5 – Point-cloud pipeline (M4) — issue #29

Scope: `Buffer` grows: `createVertexBuffer(size, usage)`, staging upload through a one-shot command buffer, `upload(const void*, size)`; `PointCloudPipeline` (topology POINT_LIST, per-vertex position + RGBA colour, point size from a push constant); `Renderer` draws a `PointCloudDrawable`.

AC: 1M random points render at interactive frame rate; test uploads a buffer on the mock ICD.

### T6 – Line pipeline (M4) — issue #30

Scope: `LinePipeline` (topology LINE_LIST, wide lines via `wideLines` feature when available, otherwise 1px); `LineSetDrawable`; a grid + axes helper drawn every frame.

AC: ground grid and XYZ axes visible; lanes-style polylines render from a vertex list.

### T7 – Scene model in Camelot (M5)

Scope: `Scene` with `PointCloud`, `LineSet`, `Marker` (box, sphere, arrow, text placeholder) primitives, each with a frame id, timestamp, transform, colour; `SceneRenderer` in Avalon that consumes a read-only view of the scene and keeps GPU buffers in sync (dirty tracking); `MainModel` owns the scene and feeds it synthetic data until T8 lands.

AC: adding/removing primitives at runtime updates the frame without re-uploading unchanged buffers; unit tests for scene mutation and dirty flags.

### T8 – MCAP reader (M6) — issues #22, #49

Scope: `mcap` C++ library via Conan (header-only reader); `McapSource` that indexes channels/schemas; decoders for Foxglove schemas `PointCloud`, `SceneUpdate` (lines, cubes, spheres, arrows, text) and `FrameTransform`, encoded as protobuf (Foxglove `.proto` files vendored, `protobuf` via Conan); channel filter by topic.

AC: opening a Foxglove-recorded `.mcap` lists topics and loads the first message of each supported schema into the scene; unsupported schemas are skipped with a warning, not an error; tests use a small fixture file checked in under `Tests/fixtures`.

Open question for the owner: is Foxglove/protobuf the right schema family, or are the recordings ROS 2 (CDR)? The decoder layer is pluggable either way, but only one will be implemented first.

### T9 – Timeline playback (M6)

Scope: `Playback` (play/pause, seek, speed) driven by message log time; `McapSource::messagesBetween(t0, t1)`; scene updates applied per tick; transforms resolved per frame id.

AC: playing a recording animates the point cloud; seeking is bounded by the file's time range; unit tests for seek/clamp logic without Vulkan.

### T10 – ImGui overlay (M7)

Scope: Dear ImGui via Conan with the GLFW + Vulkan backends; a UI pass in the same render pass; panels: topics (toggle visibility), timeline (scrub, play/pause, speed), camera (reset, FOV).

AC: UI usable with mouse; disabling a topic removes its primitives; no validation errors.

### T11 – Windows CI job (cross-cutting) — issues #15, #45

Scope: matrix entry on `windows-latest` using `profiles/Camelot-Win`, MSVC + Ninja, tests on the mock ICD (no xvfb needed on Windows).

AC: both jobs green on a PR.

Status: **done**. Implemented as a separate `build-windows` job in
`.github/workflows/build.yaml` (not a matrix entry, so a Windows failure is
isolated from the Linux jobs): Conan-provided Vulkan loader/headers and glslang,
MSVC 19.4x + Ninja, `Test_Main.exe` on the mock ICD. `scripts/windows/setup.ps1`
mirrors it for developers (`USE_MOCK_ICD=1`).

### T12 – Namespaces and public headers (cross-cutting) — issue #42

Scope: `avalon::` and `camelot::` namespaces; move consumer-facing headers of Avalon into `Avalon/include/avalon/` so Camelot stops including `Avalon/src/...`; keep the change mechanical, no behaviour change.

AC: `Camelot/` and `Pendragon/` include nothing from `Avalon/src`; build and tests unchanged.

## Suggested order

T1 → T2 → T3 → T4 → T5 → T6 → T7 → T8 → T9 → T10, with T11 and T12 slotted
in whenever a quiet moment appears (T12 is best done right after T3, before
the surface area grows).
