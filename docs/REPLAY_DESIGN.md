# MCAP replay: design and work split

Goal (roadmap M6 + parts of M7): `Pendragon <recording.mcap>` opens a Foxglove-recorded
MCAP file and replays it: camera videos, 3D objects (scene entities), point clouds
(lidar, radar), transforms, with a timeline (play/pause/seek/speed). Reference recording:
Foxglove's `NuScenes-v1.0-mini-scene-0061` conversion (`foxglove/nuscenes2mcap`), whose
topics and schemas are listed at the end. Everything must also build on Windows (T11).

Layering stays as in `docs/ROADMAP.md`: Avalon knows nothing about MCAP; Camelot owns
data ingestion, playback and the scene; Pendragon parses arguments.

## Status

| Module | Status |
|---|---|
| data (`Camelot/src/data/`) | merged into `feature/mcap-replay` |
| lines (`avalon::LineDrawable`) | merged into `feature/mcap-replay` |
| ci (Windows job, portability) | in progress on `feature/mcap-replay-ci` |
| replay (`Camelot/src/replay/`, `MainModel`, Pendragon arguments) | implemented on `feature/mcap-replay-replay`: `Recording`, `IRecordingSink`, `SceneUpdater`, topic tree, timeline, camera windows, `Pendragon [--help] [recording.mcap]`; texts, grids, models and triangle lists are counted but not drawn |

## Decisions

- **MCAP reading**: `mcap` from Conan (header-only, `mcap/2.1.3`) with `lz4` and `zstd`
  (the recording is zstd-chunked).
- **Protobuf**: no `protobuf` dependency. Foxglove messages are decoded with a small
  wire-format reader (`camelot::ProtoReader`, ~300 lines) plus one decoder function per
  message type. The vendored `.proto` files under `Camelot/src/data/schemas/` are the
  field-number reference (documentation, not compiled). This keeps MSVC/CI free of
  protobuf+abseil and is enough for the eight message types we render.
- **JPEG**: `libjpeg-turbo` from Conan (`camelot::JpegDecoder` -> RGBA8).
- **Lines** (lanes, wireframes, image annotations): new `avalon::LineDrawable`
  (LINE_LIST), reusing `PointVertex` and the camera UBO; works in scene views and as a
  `VideoTexture` overlay (pixel space).
- **Not rendered in this iteration** (skipped with one `spdlog::warn` per topic):
  `Grid` (map images), `ModelPrimitive`, `TriangleListPrimitive`, `TextPrimitive` in 3D,
  JSON channels (`/imu`, `/odom`, `/diagnostics`), `LocationFix`. They are listed in the
  topic tree as "unsupported".

## Modules and owners

| Path | Owner | Contents |
|---|---|---|
| `Camelot/src/data/` | agent **data** | `ProtoReader`, `FoxgloveMessages.h` (plain structs), `FoxgloveDecoder`, `McapSource`, `TransformTree`, `Playback`, `JpegDecoder`, `schemas/*.proto` (vendored), CMake, `conanfile.py` additions |
| `Tests/Camelot/data/`, `Tests/fixtures/` | agent **data** | one `*Test.cpp` per class; `Tests/fixtures/make_fixture.py` + the generated `nuscenes-mini.mcap` (< 400 KB) |
| `Avalon/src/renderer/LineDrawable.*`, `Avalon/shaders/line.*`, `Tests/Avalon/src/renderer/LineDrawableTest.cpp` | agent **lines** | line pipeline + drawable |
| `.github/`, `scripts/windows/`, `profiles/Camelot-Win`, `Tests/test_main.cpp`, `Tests/CMakeLists.txt` (ICD path only), `README.md` (Windows/CI sections) | agent **ci** | Windows job, Linux job fixes, portability |
| `Camelot/src/replay/`, `Camelot/src/main/MainModel.*`, `Pendragon/src/main.cpp`, `Tests/Camelot/replay/`, `Tests/Camelot/MainModelTest.cpp` | agent **replay** (after data + lines) | `Recording`, `SceneUpdater`, camera windows, timeline UI, argument parsing |

Owners touch only their paths (plus the CMakeLists that list their files). No two agents
edit the same file.

## APIs (namespace `camelot` unless noted)

### ProtoReader (`Camelot/src/data/ProtoReader.h`)
```cpp
enum class WireType : uint8_t { kVarint = 0, kFixed64 = 1, kLengthDelimited = 2, kFixed32 = 5 };
struct ProtoField { uint32_t number; WireType type; uint64_t varint; std::span<const std::byte> bytes; /* fixed32/64 read from bytes */ };
class ProtoReader {  // iterates the fields of one message; nested messages are new readers over field.bytes
 public:
  explicit ProtoReader(std::span<const std::byte> message);
  bool next(ProtoField& field);        // false at end; throws std::runtime_error on malformed input
  static double toDouble(const ProtoField&);  static float toFloat(const ProtoField&);
  static uint32_t toFixed32(const ProtoField&); static uint64_t toFixed64(const ProtoField&);
  static int64_t toSint(const ProtoField&);  static std::string_view toString(const ProtoField&);
  static void packedDoubles(const ProtoField&, std::vector<double>&); static void packedUint32(const ProtoField&, std::vector<uint32_t>&);
};
```
Skips unknown fields. `google.protobuf.Timestamp` -> `Time` (nanoseconds since epoch, `uint64_t`).

### FoxgloveMessages.h — plain structs mirroring the schemas
`Time`, `Vec3`, `Quat`, `Pose`, `Color` (floats 0..1), `PackedField{name, offset, type}`,
`PointCloud`, `CompressedImage`, `FrameTransform`, `CubePrimitive`, `SpherePrimitive`,
`LinePrimitive` (type: LINE_STRIP/LINE_LOOP/LINE_LIST, thickness, scale_invariant, points, color, colors, indices),
`ArrowPrimitive`, `TextPrimitive`, `SceneEntity` (id, frame_id, timestamp, lifetime, frame_locked, cubes, spheres, lines, arrows, texts, unsupported counts), `SceneEntityDeletion`, `SceneUpdate`,
`CircleAnnotation`, `PointsAnnotation`, `TextAnnotation`, `ImageAnnotations`, `CameraCalibration` (width, height, K[9], P[12], distortion_model, D).
Decoders: `decodePointCloud(span) -> PointCloud` etc., all `noexcept(false)` (throw `std::runtime_error` with the schema name on malformed input).
`PointCloud` gets a helper `std::vector<glm::vec4> positionsAndIntensity()` resolving x/y/z(/intensity) by field name and type (FLOAT32/FLOAT64/INT*/UINT*), so the renderer never touches `data` directly.

### McapSource (`Camelot/src/data/McapSource.h`)
```cpp
struct TopicInfo { std::string topic, schemaName, encoding; uint64_t messageCount; };
struct RawMessage { std::string_view topic; std::string_view schemaName; Time logTime; std::span<const std::byte> data; };
class McapSource {
 public:
  void open(const std::filesystem::path&);          // throws std::runtime_error with the mcap status text
  [[nodiscard]] const std::vector<TopicInfo>& topics() const;
  [[nodiscard]] Time startTime() const; [[nodiscard]] Time endTime() const;
  // Every message with t0 <= logTime < t1 on the given topics (empty = all), in log-time order.
  void forEach(Time t0, Time t1, std::span<const std::string> topics, const std::function<void(const RawMessage&)>&);
  // The last message at or before t on a topic (nullopt when none). Used after a seek.
  std::optional<std::vector<std::byte>> latestBefore(std::string_view topic, Time t);
};
```
`latestBefore` may read backwards through the chunk/message indexes or scan; correctness
first, but it must not read the whole file for every call (cache per-topic message time lists from the summary).

### TransformTree (`Camelot/src/data/TransformTree.h`)
`add(const FrameTransform&)`; `std::optional<glm::mat4> lookup(std::string_view target, std::string_view source, Time t) const`
(pose of `source` expressed in `target`, walking the parent chain both ways, nearest-time transform per edge, static edges (single sample) always match); `frames()`; `clear()`.
Right-handed, column-major GLM, quaternion (x,y,z,w).

### Playback (`Camelot/src/data/Playback.h`) — no Vulkan, no I/O
`setRange(start, end)`, `play()`, `pause()`, `toggle()`, `setSpeed(double)`, `seek(Time)` (clamped),
`Range advance(double wallSeconds)` -> `{from, to}` of the interval to apply this tick (empty when paused), `setLoop(bool)`,
`current()`, `progress()` in 0..1, `isPlaying()`, `atEnd()`.

### JpegDecoder (`Camelot/src/data/JpegDecoder.h`)
`struct DecodedImage { uint32_t width, height; std::vector<std::byte> rgba; };`
`DecodedImage decodeJpeg(std::span<const std::byte>)`; `std::vector<std::byte> encodeJpegForTests(...)` (libjpeg-turbo has both, so the test round-trips a pattern; no image fixture needed).
`bool looksLikeJpeg(span)`; PNG is out of scope (throw with a clear message).

### LineDrawable (`Avalon/src/renderer/LineDrawable.h`, namespace `avalon`)
Mirrors `PointCloudDrawable`: ctor `(device, allocator, std::span<const PointVertex> = {})`, `setLines(std::span<const PointVertex>)` (pairs -> LINE_LIST), `setStrip(std::span<const PointVertex>, bool closed)` (expanded to pairs on the CPU), `setTransform`, `setTint`, `setVisible`, `setLineWidth(float)` (clamped to the device's `lineWidthRange`, 1.0 when `wideLines` is unavailable), `vertexCount()`, `record(const FrameContext&)`, `static GraphicsPipeline buildPipeline(...)`, `kPipeline = "lines"`.
Shaders `Avalon/shaders/line.vert` / `line.frag` (add to `camelot_add_shaders` in `Avalon/CMakeLists.txt`); push constants: `mat4 model; vec4 tint;` (same as `MeshPushConstants`).
Must work both in the main render pass / `SceneViewWidget` targets and as a `VideoTexture` overlay (the overlay pass has its own camera UBO in pixel space; nothing special needed if you follow `MeshDrawable`).

### Replay (agent replay; details are its call, but keep these seams)
- `camelot::Recording`: owns `McapSource`, `TransformTree`, `Playback`, per-topic state; `open(path)`, `tick(dt)` -> applies messages of the advanced interval, `seek(t)` -> reloads latest-before for every topic; exposes `topics()` with a `visible` flag each.
- `camelot::SceneUpdater`: turns `SceneUpdate` entities into Avalon drawables (cubes -> `MeshDrawable` with `shapes::box`, spheres -> `shapes::sphere`, arrows -> cylinder + a short thicker cylinder as head, lines -> `LineDrawable`), keyed by (topic, entity id), honouring deletions and `lifetime`; poses resolved through the `TransformTree` into a fixed render frame (`map` if present, else the first root frame); point clouds -> one `PointCloudDrawable` per topic (intensity -> colour ramp); camera topics -> one `VideoTexture` per camera (JPEG decoded on the tick; `ImageAnnotations` of the same namespace drawn as `LineDrawable`/`PointCloudDrawable` overlays in pixel space).
- `MainModel`: `openRecording(path)`; a "Timeline" window (ImGui: play/pause, speed combo, slider seek, current/end time); the "Scene" tree gets a "Topics" branch (checkbox visibility per topic, unsupported topics greyed with the schema name); "Windows" menu gets "New camera view" per camera topic; camera windows are `VideoTexture` + `ImGui::Image` fitted like `VideoWidget::draw()` (playback is global, so no per-video controls).
- `Pendragon`: `Pendragon [--help] [recording.mcap]`; a bad path is an error message and exit code 2, no window.

## Build and test (Linux, this machine)

conan and cmake are uv tools in `~/.local/bin`. Three X libraries GLFW never uses are not
installed here, so `conan install` needs stub `.pc` files:
```sh
export PATH="$HOME/.local/bin:$PATH"
export PKG_CONFIG_PATH="/home/jfilip/.cache/camelot-tools/pkgconfig:${PKG_CONFIG_PATH:-}"
conan install . --profile:host=Camelot-local --profile:build=Camelot-local -s build_type=Release \
    --build=missing -c tools.system.package_manager:mode=report
cmake -S . -B build/Release -G Ninja -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTING=ON -DCAMELOT_TESTS_USE_MOCK_ICD=ON
cmake --build build/Release -j4            # -j4: several agents build at once on this machine
build/Release/bin/Test_Main --gtest_filter='ProtoReader*'   # no xvfb here; the live session works
```
Style: `docs/CODING_STYLE.md` (Google style, `m_` members, `k` constants, license header
from `LICENSES/code_header`, project includes by path from the repo root, one class per
file, `[[nodiscard]]` getters). clang-tidy (`.clang-tidy`, warnings are errors) and cpplint
run in CI: keep functions small, no magic-number lint traps in headers, `noexcept` moves.
Run `pre-commit run --files <your files>` before you finish (pre-commit is installed).

## Reference recording (nuscenes2mcap output)

| Topic | Schema | Notes |
|---|---|---|
| `/CAM_{FRONT,FRONT_LEFT,FRONT_RIGHT,BACK,BACK_LEFT,BACK_RIGHT}/image_rect_compressed` | `foxglove.CompressedImage` (jpeg, 1600x900, ~12 Hz) | video windows |
| `/CAM_*/camera_info` | `foxglove.CameraCalibration` | intrinsics (unused for now) |
| `/CAM_*/annotations` | `foxglove.ImageAnnotations` | 2D boxes of the annotations on the image |
| `/LIDAR_TOP` | `foxglove.PointCloud` (x,y,z,intensity float32, ~20 Hz, ~35k points) | point cloud |
| `/RADAR_{FRONT,FRONT_LEFT,FRONT_RIGHT,BACK_LEFT,BACK_RIGHT}` | `foxglove.PointCloud` | small clouds |
| `/markers/annotations` | `foxglove.SceneUpdate` (cubes + texts, 2 Hz keyframes) | objects |
| `/markers/car` | `foxglove.SceneUpdate` (model or cube) | ego car |
| `/semantic_map` | `foxglove.SceneUpdate` (lines: lane centerlines) | lanes |
| `/tf` | `foxglove.FrameTransform` (map->base_link, base_link->sensors) | transforms |
| `/map`, `/drivable_area` | `foxglove.Grid` | unsupported for now |
| `/gps` | `foxglove.LocationFix`; `/imu`, `/odom`, `/diagnostics` json | unsupported |
