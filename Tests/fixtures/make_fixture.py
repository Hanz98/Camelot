#!/usr/bin/env python3
"""Writes Tests/fixtures/nuscenes-mini.mcap, a tiny stand-in for Foxglove's
nuscenes2mcap output (NuScenes-v1.0-mini-scene-0061) with the same topic
layout, schemas and encodings, small enough to commit (< 400 KB).

McapSourceTest asserts the exact counts and times below, so keep them in
sync. Run with the Foxglove Python SDK (packages foxglove, numpy, PIL):

    python Tests/fixtures/make_fixture.py
"""

import io
import json
import math
import os
import sys

import numpy as np
from PIL import Image

import foxglove
from foxglove import Channel
from foxglove.channels import (
    CameraCalibrationChannel,
    CompressedImageChannel,
    FrameTransformChannel,
    ImageAnnotationsChannel,
    LocationFixChannel,
    PointCloudChannel,
    SceneUpdateChannel,
)
from foxglove.mcap import MCAPCompression, MCAPWriteOptions
from foxglove.messages import (
    CameraCalibration,
    Color,
    CompressedImage,
    CubePrimitive,
    Duration,
    FrameTransform,
    ImageAnnotations,
    LinePrimitive,
    LinePrimitiveLineType,
    LocationFix,
    PackedElementField,
    PackedElementFieldNumericType,
    Point2,
    Point3,
    PointCloud,
    PointsAnnotation,
    PointsAnnotationType,
    Pose,
    Quaternion,
    SceneEntity,
    SceneUpdate,
    TextAnnotation,
    TextPrimitive,
    Timestamp,
    Vector3,
)

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "nuscenes-mini.mcap")

NS = 1_000_000_000
T0 = 1_600_000_000 * NS  # log time of the first message
MS = 1_000_000

CAMERAS = ["CAM_FRONT", "CAM_BACK"]
CAM_FRAMES = 8  # per camera, every 250 ms
IMAGE_SIZE = (64, 36)
LIDAR_MESSAGES = 10  # every 200 ms
LIDAR_POINTS = 300
RADAR_MESSAGES = 10  # every 200 ms, 50 ms after the lidar
RADAR_POINTS = 24
MARKER_MESSAGES = 4  # every 500 ms
MARKER_OBJECTS = 3
LANES = 3
LANE_POINTS = 10
TF_DYNAMIC = 20  # every 100 ms
GPS_MESSAGES = 3  # every second
IMU_MESSAGES = 5  # every 400 ms, json

CHUNK_SIZE = 16 * 1024  # small chunks so seeks cross chunk boundaries


def stamp(t_ns):
    return Timestamp(sec=t_ns // NS, nsec=t_ns % NS)


def identity():
    return Quaternion(x=0.0, y=0.0, z=0.0, w=1.0)


def jpeg_frame(k, camera):
    """A 64x36 gradient with a bar that moves with k (and camera)."""
    w, h = IMAGE_SIZE
    x = np.arange(w, dtype=np.uint8)
    y = np.arange(h, dtype=np.uint8)
    img = np.zeros((h, w, 3), dtype=np.uint8)
    img[..., 0] = (x * 4)[None, :]
    img[..., 1] = (y * 7)[:, None]
    img[..., 2] = 96 if camera == "CAM_FRONT" else 200
    bar = (k * 8) % w
    img[:, bar : bar + 4, :] = 255
    buf = io.BytesIO()
    Image.fromarray(img, "RGB").save(buf, format="JPEG", quality=80)
    return buf.getvalue()


def lidar_cloud(k):
    i = np.arange(LIDAR_POINTS, dtype=np.float32)
    angle = i * (2.0 * math.pi / LIDAR_POINTS) + 0.1 * k
    radius = 5.0 + (i % 7)
    points = np.empty((LIDAR_POINTS, 4), dtype=np.float32)
    points[:, 0] = np.cos(angle) * radius
    points[:, 1] = np.sin(angle) * radius
    points[:, 2] = i * 0.01 - 1.0
    points[:, 3] = i % 256  # intensity
    return points


def radar_cloud(k):
    i = np.arange(RADAR_POINTS, dtype=np.float32)
    points = np.empty((RADAR_POINTS, 4), dtype=np.float32)
    points[:, 0] = 10.0 + i * 2.0
    points[:, 1] = (i % 5) - 2.0
    points[:, 2] = 0.5
    points[:, 3] = 3.0 + 0.1 * k  # vx
    return points


def float32_fields(names):
    return [
        PackedElementField(name=n, offset=4 * j, type=PackedElementFieldNumericType.Float32)
        for j, n in enumerate(names)
    ]


def point_cloud(t, frame_id, names, points):
    return PointCloud(
        timestamp=stamp(t),
        frame_id=frame_id,
        pose=Pose(position=Vector3(x=0.0, y=0.0, z=0.0), orientation=identity()),
        point_stride=4 * len(names),
        fields=float32_fields(names),
        data=points.astype("<f4").tobytes(),
    )


def annotations(t, k):
    x0 = 8.0 + 2.0 * k
    box = [Point2(x=x0, y=8.0), Point2(x=x0 + 20.0, y=8.0),
           Point2(x=x0 + 20.0, y=24.0), Point2(x=x0, y=24.0)]
    return ImageAnnotations(
        timestamp=stamp(t),
        points=[
            PointsAnnotation(
                timestamp=stamp(t),
                type=PointsAnnotationType.LineLoop,
                points=box,
                outline_color=Color(r=0.0, g=1.0, b=0.0, a=1.0),
                thickness=2.0,
            )
        ],
        texts=[
            TextAnnotation(
                timestamp=stamp(t),
                position=Point2(x=x0, y=6.0),
                text="car",
                font_size=6.0,
                text_color=Color(r=1.0, g=1.0, b=1.0, a=1.0),
                background_color=Color(r=0.0, g=0.0, b=0.0, a=0.5),
            )
        ],
    )


def calibration(t, camera):
    w, h = IMAGE_SIZE
    return CameraCalibration(
        timestamp=stamp(t),
        frame_id=camera,
        width=w,
        height=h,
        distortion_model="plumb_bob",
        D=[0.0, 0.0, 0.0, 0.0, 0.0],
        K=[50.0, 0.0, 32.0, 0.0, 50.0, 18.0, 0.0, 0.0, 1.0],
        R=[1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0],
        P=[50.0, 0.0, 32.0, 0.0, 0.0, 50.0, 18.0, 0.0, 0.0, 0.0, 1.0, 0.0],
    )


def markers(t, k):
    entities = []
    for i in range(MARKER_OBJECTS):
        x = 10.0 + 6.0 * i + 0.5 * k
        pose = Pose(position=Vector3(x=x, y=3.0 * i - 3.0, z=0.75), orientation=identity())
        entities.append(
            SceneEntity(
                timestamp=stamp(t),
                frame_id="map",
                id=f"obj-{i}",
                lifetime=Duration(sec=0, nsec=500 * MS),
                frame_locked=False,
                cubes=[
                    CubePrimitive(
                        pose=pose,
                        size=Vector3(x=4.0, y=2.0, z=1.5),
                        color=Color(r=1.0, g=0.2 * i, b=0.0, a=0.6),
                    )
                ],
                texts=[
                    TextPrimitive(
                        pose=Pose(
                            position=Vector3(x=x, y=3.0 * i - 3.0, z=2.0),
                            orientation=identity(),
                        ),
                        billboard=True,
                        font_size=0.5,
                        scale_invariant=False,
                        color=Color(r=1.0, g=1.0, b=1.0, a=1.0),
                        text=f"vehicle.car {i}",
                    )
                ],
            )
        )
    return SceneUpdate(entities=entities)


def semantic_map(t):
    lines = []
    for lane in range(LANES):
        points = [
            Point3(x=2.0 * j, y=3.5 * lane - 3.5 + 0.2 * math.sin(j), z=0.0)
            for j in range(LANE_POINTS)
        ]
        lines.append(
            LinePrimitive(
                type=LinePrimitiveLineType.LineStrip,
                pose=Pose(position=Vector3(x=0.0, y=0.0, z=0.0), orientation=identity()),
                thickness=0.2,
                scale_invariant=False,
                points=points,
                color=Color(r=0.2, g=0.8, b=0.2, a=1.0),
            )
        )
    return SceneUpdate(
        entities=[
            SceneEntity(
                timestamp=stamp(t),
                frame_id="map",
                id="lane-0",
                lifetime=Duration(sec=0, nsec=0),
                frame_locked=False,
                lines=lines,
            )
        ]
    )


def tf(t, parent, child, translation, rotation=None):
    return FrameTransform(
        timestamp=stamp(t),
        parent_frame_id=parent,
        child_frame_id=child,
        translation=Vector3(x=translation[0], y=translation[1], z=translation[2]),
        rotation=rotation or identity(),
    )


def main():
    if os.path.exists(OUT):
        os.remove(OUT)
    options = MCAPWriteOptions(compression=MCAPCompression.Zstd, chunk_size=CHUNK_SIZE)
    writer = foxglove.open_mcap(OUT, writer_options=options)

    cam_images = {c: CompressedImageChannel(f"/{c}/image_rect_compressed") for c in CAMERAS}
    cam_annotations = {c: ImageAnnotationsChannel(f"/{c}/annotations") for c in CAMERAS}
    cam_info = {c: CameraCalibrationChannel(f"/{c}/camera_info") for c in CAMERAS}
    lidar = PointCloudChannel("/LIDAR_TOP")
    radar = PointCloudChannel("/RADAR_FRONT")
    marker_channel = SceneUpdateChannel("/markers/annotations")
    semantic_channel = SceneUpdateChannel("/semantic_map")
    tf_channel = FrameTransformChannel("/tf")
    gps = LocationFixChannel("/gps")
    imu = Channel(
        "/imu",
        message_encoding="json",
        schema={
            "type": "object",
            "properties": {
                "ax": {"type": "number"},
                "ay": {"type": "number"},
                "az": {"type": "number"},
            },
        },
    )

    # Static transforms and the map first, all at T0.
    tf_channel.log(tf(T0, "base_link", "LIDAR_TOP", (0.9, 0.0, 1.8)), log_time=T0)
    optical = Quaternion(x=-0.5, y=0.5, z=-0.5, w=0.5)
    tf_channel.log(tf(T0, "base_link", "CAM_FRONT", (1.7, 0.0, 1.5), optical), log_time=T0)
    tf_channel.log(tf(T0, "base_link", "CAM_BACK", (-0.5, 0.0, 1.5), optical), log_time=T0)
    tf_channel.log(tf(T0, "base_link", "RADAR_FRONT", (3.4, 0.0, 0.5)), log_time=T0)
    semantic_channel.log(semantic_map(T0), log_time=T0)

    for k in range(TF_DYNAMIC):
        t = T0 + k * 100 * MS
        tf_channel.log(tf(t, "map", "base_link", (0.5 * k, 0.0, 0.0)), log_time=t)

    for k in range(CAM_FRAMES):
        t = T0 + k * 250 * MS
        for camera in CAMERAS:
            cam_images[camera].log(
                CompressedImage(
                    timestamp=stamp(t), frame_id=camera, data=jpeg_frame(k, camera), format="jpeg"
                ),
                log_time=t,
            )
            cam_annotations[camera].log(annotations(t, k), log_time=t)
            cam_info[camera].log(calibration(t, camera), log_time=t)

    for k in range(LIDAR_MESSAGES):
        t = T0 + k * 200 * MS
        lidar.log(
            point_cloud(t, "LIDAR_TOP", ["x", "y", "z", "intensity"], lidar_cloud(k)),
            log_time=t,
        )
    for k in range(RADAR_MESSAGES):
        t = T0 + k * 200 * MS + 50 * MS
        radar.log(point_cloud(t, "RADAR_FRONT", ["x", "y", "z", "vx"], radar_cloud(k)), log_time=t)

    for k in range(MARKER_MESSAGES):
        t = T0 + k * 500 * MS
        marker_channel.log(markers(t, k), log_time=t)

    for k in range(GPS_MESSAGES):
        t = T0 + k * NS
        gps.log(
            LocationFix(
                timestamp=stamp(t),
                frame_id="base_link",
                latitude=42.336 + 0.0001 * k,
                longitude=-71.059,
                altitude=12.0,
            ),
            log_time=t,
        )

    for k in range(IMU_MESSAGES):
        t = T0 + k * 400 * MS
        imu.log(json.dumps({"ax": 0.1 * k, "ay": 0.0, "az": 9.81}).encode(), log_time=t)

    writer.close()
    size = os.path.getsize(OUT)
    print(f"wrote {OUT}: {size} bytes")
    if size > 400 * 1024:
        print("fixture is larger than 400 KB", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
