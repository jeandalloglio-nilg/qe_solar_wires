#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
WP4 → WP5 Integration (v5): RAW-Only + Dual Visualization + EXIF Injection
==========================================================================

What this script does
---------------------
1) Reads EXIF from a FLIR *RAW* or *Edited* image
2) Extracts keypoints:
   - EXIF mode: FLIR:MeasXParams (spots)
   - YOLO mode: detects points on a provided image (e.g. cleaned thermal PNG)
3) (Optional) Extracts temperatures at each keypoint using flirimageextractor
4) Maps keypoints to the chosen base image space:
   - Official: RGB image (thermal -> RGB using Real2IR + Offset)
   - Experimental: ThermalClean image (thermal -> thermal_clean scaling)
5) Calls OpenAI (vision) with structured JSON schema to produce clusters
6) Compares temperatures within each cluster to detect hotspots/dead connections
7) Saves JSON outputs + images:
   - keypoints_used.json
   - LLM input image (with IDs)
   - clusters.json
   - Visualization in BOTH thermal and RGB (separate folders):
     * thermal/: keypoints, clustered, side_by_side
     * rgb/: keypoints, clustered, side_by_side
   - combined_view.png (4 panels: thermal + RGB)
   - anomalies.json
8) (Optional) Injects detected spots into EXIF for FLIR Tools

Notes
-----
- Requires: exiftool, opencv-python, numpy, matplotlib, pillow
- OpenAI key: env var OPENAI_API_KEY (dotenv supported)
- YOLO mode requires: ultralytics (pip install ultralytics)

Usage (RAW-only mode, recommended for production)
-------------------------------------------------
python -m src.tools.wp4_wp5_integration_v4 \\
  --raw-image "Raw Images/FLIR0001.jpg" \\
  --rgb-image "rgb_images/FLIR0001_rgb.jpg" \\
  --thermal-clean-image "clean/FLIR0001_clean.png" \\
  --keypoint-source yolo \\
  --yolo-model "models/best.pt" \\
  --yolo-conf 0.15 \\
  --extract-temperatures \\
  --inject-exif \\
  --output "output/FLIR0001/"

Usage (Legacy mode with Edited image)
-------------------------------------
python -m src.tools.wp4_wp5_integration_v4 \\
  --edited-image "Edited Images/FLIR3672.jpg" \\
  --rgb-image "rgb_images/FLIR3672_rgb.jpg" \\
  --scale-to-rgb \\
  --real2ir 1.74 \\
  --offset-x -10 \\
  --offset-y -13 \\
  --clip-outside \\
  --extract-temperatures \\
  --kernel-size 3 \\
  --delta-threshold 3.0 \\
  --openai-model gpt-4o \\
  --output "wp5_test_FLIR3672_rgb/"

Author: QE Solar / Elton Gino
"""

from __future__ import annotations

import argparse
import base64
import io
import json
import math
import os
import re
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

from dotenv import load_dotenv

import cv2
import numpy as np

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches


# =============================================================================
# Utilities
# =============================================================================

IMG_EXTS = {".jpg", ".jpeg", ".png", ".tif", ".tiff"}


def _ensure_parent_dir(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)


def _read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def _write_json(path: Path, obj: Any) -> None:
    _ensure_parent_dir(path)
    path.write_text(json.dumps(obj, indent=2, ensure_ascii=False), encoding="utf-8")


def _load_image_bgr(path: Path) -> np.ndarray:
    img = cv2.imread(str(path), cv2.IMREAD_UNCHANGED)
    if img is None:
        raise FileNotFoundError(f"Could not read image: {path}")
    if img.ndim == 2:
        img = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)
    elif img.shape[2] == 4:
        img = cv2.cvtColor(img, cv2.COLOR_BGRA2BGR)
    return img


def _image_to_base64_jpeg(bgr: np.ndarray, max_size: int = 1280, quality: int = 85) -> str:
    from PIL import Image
    rgb = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
    im = Image.fromarray(rgb)
    w, h = im.size
    m = max(w, h)
    if m > max_size:
        ratio = max_size / m
        im = im.resize((int(w * ratio), int(h * ratio)), Image.LANCZOS)
    buf = io.BytesIO()
    im.save(buf, format="JPEG", quality=quality)
    return base64.b64encode(buf.getvalue()).decode("utf-8")


def _get_image_size_wh(path: Path) -> Tuple[int, int]:
    img = _load_image_bgr(path)
    h, w = img.shape[:2]
    return int(w), int(h)


def _load_env_if_available() -> None:
    try:
        from dotenv import load_dotenv
        load_dotenv()
    except Exception:
        pass


# =============================================================================
# EXIF + Keypoints
# =============================================================================

_MEAS_PARAMS_RE = re.compile(r"FLIR:Meas(\d+)Params")


def extract_exif_from_image(image_path: Path) -> Dict[str, Any]:
    cmd = ["exiftool", "-j", "-G:1", "-s", "-n", str(image_path)]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"exiftool failed: {res.stderr.strip()}")
    data = json.loads(res.stdout)
    return data[0] if data else {}


def extract_keypoints_from_exif(exif_item: Dict[str, Any]) -> List[Dict[str, Any]]:
    keypoints: List[Dict[str, Any]] = []

    for k, v in exif_item.items():
        m = _MEAS_PARAMS_RE.match(k)
        if not m:
            continue
        meas_n = m.group(1)
        meas_type = exif_item.get(f"FLIR:Meas{meas_n}Type", 0)
        if meas_type != 1:
            continue

        try:
            if isinstance(v, str):
                parts = v.strip().split()
                if len(parts) < 2:
                    continue
                x = float(parts[0]); y = float(parts[1])
            elif isinstance(v, (list, tuple)) and len(v) >= 2:
                x = float(v[0]); y = float(v[1])
            else:
                continue
        except Exception:
            continue

        label = exif_item.get(f"FLIR:Meas{meas_n}Label", f"Sp{meas_n}")
        temperature = exif_item.get(f"FLIR:Meas{meas_n}Value", None)

        kp = {
            "id": int(meas_n),
            "label": str(label),
            "x": float(x),
            "y": float(y),
            "x_thermal": float(x),
            "y_thermal": float(y),
            "temperature": temperature,
            "source": "exif",
        }
        keypoints.append(kp)

    keypoints.sort(key=lambda d: int(d["id"]))
    # normalize IDs to 1..N just in case FLIR uses sparse numbering
    for i, kp in enumerate(keypoints, start=1):
        kp["id"] = i
        kp["label"] = f"Sp{i}"
    return keypoints


def extract_keypoints_from_yolo(
    image_path: Path,
    model_path: Path,
    conf_threshold: float = 0.30,
    iou_threshold: float = 0.45,
    device: str = "auto",
    verbose: bool = False,
) -> List[Dict[str, Any]]:
    try:
        from ultralytics import YOLO
    except Exception as e:
        raise ImportError("ultralytics is required for YOLO mode. Install: pip install ultralytics") from e

    image_path = Path(image_path)
    model_path = Path(model_path)

    if not image_path.exists():
        raise FileNotFoundError(f"YOLO image not found: {image_path}")
    if not model_path.exists():
        raise FileNotFoundError(f"YOLO model not found: {model_path}")

    model = YOLO(str(model_path))
    results = model.predict(
        source=str(image_path),
        conf=float(conf_threshold),
        iou=float(iou_threshold),
        device=None if device == "auto" else str(device),
        verbose=bool(verbose),
    )

    keypoints: List[Dict[str, Any]] = []
    for r in results:
        boxes = getattr(r, "boxes", None)
        if boxes is None:
            continue

        xywh = boxes.xywh.cpu().numpy() if hasattr(boxes.xywh, "cpu") else boxes.xywh
        confs = boxes.conf.cpu().numpy() if hasattr(boxes.conf, "cpu") else boxes.conf
        clss  = boxes.cls.cpu().numpy()  if hasattr(boxes.cls,  "cpu") else boxes.cls

        for (x, y, w, h), c, cls in zip(xywh, confs, clss):
            kp = {
                "x": float(x),
                "y": float(y),
                "x_yolo": float(x),
                "y_yolo": float(y),
                "conf": float(c),
                "cls": int(cls) if cls is not None else None,
                "source": "yolo",
            }
            try:
                names = getattr(model, "names", None)
                if isinstance(names, dict) and kp["cls"] in names:
                    kp["name"] = str(names[kp["cls"]])
            except Exception:
                pass
            keypoints.append(kp)

    keypoints.sort(key=lambda k: (k["y"], k["x"]))
    for i, kp in enumerate(keypoints, start=1):
        kp["id"] = i
        kp["label"] = f"Sp{i}"
    return keypoints


def map_yolo_keypoints_to_thermal_space(
    yolo_keypoints: List[Dict[str, Any]],
    yolo_size: Tuple[int, int],
    thermal_size: Tuple[int, int],
) -> List[Dict[str, Any]]:
    yw, yh = yolo_size
    tw, th = thermal_size
    sx = (tw / float(yw)) if yw else 1.0
    sy = (th / float(yh)) if yh else 1.0

    out: List[Dict[str, Any]] = []
    for kp in yolo_keypoints:
        kp2 = dict(kp)
        x_th = float(kp2.get("x", 0.0)) * sx
        y_th = float(kp2.get("y", 0.0)) * sy
        kp2["x_thermal"] = x_th
        kp2["y_thermal"] = y_th
        # in thermal space (base)
        kp2["x"] = x_th
        kp2["y"] = y_th
        out.append(kp2)
    return out


def get_alignment_params_from_exif(exif: Dict[str, Any]) -> Dict[str, Any]:
    return {
        "real2ir": exif.get("FLIR:Real2IR", 1.76),
        "offset_x": exif.get("FLIR:OffsetX", -24.0),
        "offset_y": exif.get("FLIR:OffsetY", -24.0),
        "thermal_width": exif.get("FLIR:RawThermalImageWidth", 320),
        "thermal_height": exif.get("FLIR:RawThermalImageHeight", 240),
        "rgb_width": exif.get("FLIR:EmbeddedImageWidth", 1280),
        "rgb_height": exif.get("FLIR:EmbeddedImageHeight", 960),
    }


# =============================================================================
# Coordinate mapping
# =============================================================================

def transform_thermal_to_rgb(
    keypoints: List[Dict[str, Any]],
    thermal_size: Tuple[int, int],
    rgb_size: Tuple[int, int],
    real2ir: float,
    offset_x: float,
    offset_y: float,
    clip_outside: bool = False,
) -> List[Dict[str, Any]]:
    th_w, th_h = thermal_size
    rgb_w, rgb_h = rgb_size

    th_cx, th_cy = th_w / 2.0, th_h / 2.0
    rgb_cx, rgb_cy = rgb_w / 2.0, rgb_h / 2.0

    base_scale = rgb_w / float(th_w)
    effective_scale = base_scale / float(real2ir)

    out: List[Dict[str, Any]] = []
    for kp in keypoints:
        kp2 = dict(kp)
        x_th = float(kp2.get("x_thermal", kp2.get("x", 0.0)))
        y_th = float(kp2.get("y_thermal", kp2.get("y", 0.0)))
        kp2["x_thermal"] = x_th
        kp2["y_thermal"] = y_th

        x_off = x_th + float(offset_x)
        y_off = y_th + float(offset_y)
        rel_x = x_off - th_cx
        rel_y = y_off - th_cy

        x_new = rgb_cx + rel_x * effective_scale
        y_new = rgb_cy + rel_y * effective_scale

        if clip_outside:
            if not (0 <= x_new < rgb_w and 0 <= y_new < rgb_h):
                continue
        else:
            x_new = max(0.0, min(rgb_w - 1.0, x_new))
            y_new = max(0.0, min(rgb_h - 1.0, y_new))

        kp2["x"] = float(x_new)
        kp2["y"] = float(y_new)
        out.append(kp2)
    return out


def transform_thermal_to_target_scale(
    keypoints: List[Dict[str, Any]],
    thermal_size: Tuple[int, int],
    target_size: Tuple[int, int],
    clip_outside: bool = False,
) -> List[Dict[str, Any]]:
    th_w, th_h = thermal_size
    tg_w, tg_h = target_size
    sx = tg_w / float(th_w)
    sy = tg_h / float(th_h)

    out: List[Dict[str, Any]] = []
    for kp in keypoints:
        kp2 = dict(kp)
        x_th = float(kp2.get("x_thermal", kp2.get("x", 0.0)))
        y_th = float(kp2.get("y_thermal", kp2.get("y", 0.0)))
        kp2["x_thermal"] = x_th
        kp2["y_thermal"] = y_th

        x_new = x_th * sx
        y_new = y_th * sy
        if clip_outside:
            if not (0 <= x_new < tg_w and 0 <= y_new < tg_h):
                continue
        else:
            x_new = max(0.0, min(tg_w - 1.0, x_new))
            y_new = max(0.0, min(tg_h - 1.0, y_new))

        kp2["x"] = float(x_new)
        kp2["y"] = float(y_new)
        out.append(kp2)
    return out


# =============================================================================
# Temperature extraction (flirimageextractor preferred)
# =============================================================================

def extract_temperatures_for_keypoints(
    flir_source_image: Path,
    exif: Dict[str, Any],
    keypoints: List[Dict[str, Any]],
    kernel_size: int = 3,
) -> Tuple[List[Dict[str, Any]], Dict[str, Any]]:
    try:
        import flirimageextractor
    except ImportError:
        print("   ⚠️  flirimageextractor not installed. Run: pip install flirimageextractor")
        print("   Falling back to Planck calculation (may be inaccurate)...")
        return _extract_temperatures_planck_fallback(flir_source_image, exif, keypoints, kernel_size)

    try:
        flir = flirimageextractor.FlirImageExtractor(palettes=[])
        flir.process_image(str(flir_source_image))
        temp_c = flir.get_thermal_np()
    except Exception as e:
        print(f"   ⚠️  flirimageextractor failed: {e}")
        print("   Falling back to Planck calculation...")
        return _extract_temperatures_planck_fallback(flir_source_image, exif, keypoints, kernel_size)

    h, w = temp_c.shape
    k = max(1, int(kernel_size))
    if k % 2 == 0:
        k += 1
    r = k // 2

    temps_all: List[float] = []
    updated: List[Dict[str, Any]] = []

    for kp in keypoints:
        kp2 = dict(kp)
        x_th = kp2.get("x_thermal", None)
        y_th = kp2.get("y_thermal", None)
        if x_th is None or y_th is None:
            kp2["temperature"] = None
            kp2["temperature_std"] = None
            updated.append(kp2)
            continue

        xi = int(round(float(x_th)))
        yi = int(round(float(y_th)))

        if not (0 <= xi < w and 0 <= yi < h):
            kp2["temperature"] = None
            kp2["temperature_std"] = None
            updated.append(kp2)
            continue

        x0 = max(0, xi - r); x1 = min(w, xi + r + 1)
        y0 = max(0, yi - r); y1 = min(h, yi + r + 1)
        patch = temp_c[y0:y1, x0:x1].astype(np.float64)
        patch = patch[np.isfinite(patch)]

        if patch.size == 0:
            kp2["temperature"] = None
            kp2["temperature_std"] = None
        else:
            kp2["temperature"] = round(float(np.nanmean(patch)), 2)
            kp2["temperature_std"] = round(float(np.nanstd(patch)), 3)
            temps_all.append(float(kp2["temperature"]))

        updated.append(kp2)

    if temps_all:
        summary = {
            "min_c": round(float(np.min(temps_all)), 2),
            "max_c": round(float(np.max(temps_all)), 2),
            "delta_c": round(float(np.max(temps_all) - np.min(temps_all)), 2),
            "count": len(temps_all),
            "kernel_size": k,
            "method": "flirimageextractor",
        }
    else:
        summary = {"count": 0, "kernel_size": k, "method": "flirimageextractor"}

    return updated, summary


def _extract_temperatures_planck_fallback(
    flir_source_image: Path,
    exif: Dict[str, Any],
    keypoints: List[Dict[str, Any]],
    kernel_size: int = 3,
) -> Tuple[List[Dict[str, Any]], Dict[str, Any]]:
    @dataclass
    class PlanckParams:
        r1: float
        r2: float
        b: float
        f: float
        o: float
        emiss: float

    def _extract_raw_thermal_png(flir_image: Path) -> np.ndarray:
        cmd = ["exiftool", "-b", "-RawThermalImage", str(flir_image)]
        result = subprocess.run(cmd, capture_output=True)
        if result.returncode != 0 or not result.stdout:
            raise RuntimeError("Failed extracting RawThermalImage")
        from PIL import Image
        img = Image.open(io.BytesIO(result.stdout))
        return np.array(img)

    def _parse_planck_params(exif_: Dict[str, Any]) -> PlanckParams:
        missing = [k for k in ["FLIR:PlanckR1","FLIR:PlanckR2","FLIR:PlanckB","FLIR:PlanckF","FLIR:PlanckO","FLIR:Emissivity"] if k not in exif_]
        if missing:
            raise KeyError(f"Missing radiometric keys in EXIF: {missing}")
        return PlanckParams(
            r1=float(exif_["FLIR:PlanckR1"]),
            r2=float(exif_["FLIR:PlanckR2"]),
            b=float(exif_["FLIR:PlanckB"]),
            f=float(exif_["FLIR:PlanckF"]),
            o=float(exif_["FLIR:PlanckO"]),
            emiss=float(exif_.get("FLIR:Emissivity", 0.95)),
        )

    def _temp_k_from_raw(raw: np.ndarray, p: PlanckParams) -> np.ndarray:
        raw = raw.astype(np.float64)
        with np.errstate(divide="ignore", invalid="ignore", over="ignore"):
            denom = p.r2 * (raw + p.o)
            ratio = np.where(denom != 0, p.r1 / denom, np.nan)
            inside = ratio + p.f
            valid = inside > 0
            out = np.full_like(raw, np.nan, dtype=np.float64)
            out[valid] = p.b / np.log(inside[valid])
        return out

    try:
        p = _parse_planck_params(exif)
        raw = _extract_raw_thermal_png(flir_source_image)
        if raw.ndim == 3:
            raw = raw[:, :, 0]
        temp_k = _temp_k_from_raw(raw, p)
        temp_c = temp_k - 273.15
    except Exception as e:
        print(f"   ⚠️  Planck fallback also failed: {e}")
        return keypoints, {"count": 0, "method": "failed"}

    h, w = temp_c.shape
    k = max(1, int(kernel_size))
    if k % 2 == 0:
        k += 1
    r = k // 2

    temps_all: List[float] = []
    updated: List[Dict[str, Any]] = []

    for kp in keypoints:
        kp2 = dict(kp)
        x_th = kp2.get("x_thermal", None)
        y_th = kp2.get("y_thermal", None)

        if x_th is None or y_th is None:
            kp2["temperature"] = None
            kp2["temperature_std"] = None
            updated.append(kp2)
            continue

        xi = int(round(float(x_th)))
        yi = int(round(float(y_th)))

        if not (0 <= xi < w and 0 <= yi < h):
            kp2["temperature"] = None
            kp2["temperature_std"] = None
            updated.append(kp2)
            continue

        x0 = max(0, xi - r); x1 = min(w, xi + r + 1)
        y0 = max(0, yi - r); y1 = min(h, yi + r + 1)
        patch = temp_c[y0:y1, x0:x1].astype(np.float64)
        patch = patch[np.isfinite(patch)]

        if patch.size == 0:
            kp2["temperature"] = None
            kp2["temperature_std"] = None
        else:
            kp2["temperature"] = round(float(np.nanmean(patch)), 2)
            kp2["temperature_std"] = round(float(np.nanstd(patch)), 3)
            temps_all.append(float(kp2["temperature"]))

        updated.append(kp2)

    if temps_all:
        summary = {
            "min_c": round(float(np.min(temps_all)), 2),
            "max_c": round(float(np.max(temps_all)), 2),
            "delta_c": round(float(np.max(temps_all) - np.min(temps_all)), 2),
            "count": len(temps_all),
            "kernel_size": k,
            "method": "planck_fallback",
        }
    else:
        summary = {"count": 0, "kernel_size": k, "method": "planck_fallback"}

    return updated, summary


# =============================================================================
# OpenAI clustering (Responses API + JSON schema)
# =============================================================================

THERMAL_GROUPING_PROMPT = """Look at the electrical equipment image with circles marking keypoints.

Goal (thermal analysis):
Cluster keypoints into **thermal comparison groups**: each cluster should contain keypoints whose temperatures are meaningfully comparable for anomaly detection (they should normally run at similar temperature under the same operating conditions).

Core idea:
- Clusters are NOT strictly "one physical component instance".
- Clusters are "thermal-equivalence groups" based on function, load path, and surface/target type, so that within each cluster we can compare keypoint temperatures to flag outliers/hotspots or dead/cold connections.

How to decide if keypoints are comparable (put in same cluster):
Use visual/semantic cues to infer the role and similarity:
- Same function in the circuit: same phase path, same feeder type, same protection role, same termination role.
- Same kind of surface/contact: same material/finish (e.g., bare metal lug-to-busbar joints), same housing type (same fuse link body), same connector family (same elbow/bushing type).
- Same modular/repeated pattern: identical parts repeated in a row/column (e.g., multiple identical fuses, breaker poles, terminal blocks).
- Same expected current/load context: same conductor size and routing, same labeled positions (e.g., L1/L2/L3), same bank/rail serving parallel strings.
- Same measurement "target type": compare metal-to-metal joints with other metal-to-metal joints; compare insulation surfaces with similar insulation surfaces (avoid mixing surfaces with very different emissivity/appearance).

When NOT to group together:
- Different thermal roles or emissivity targets (e.g., shiny busbar joint vs. painted enclosure) unless explicitly analyzing as same target type.
- Different component families or clearly different ratings/sizes that imply different normal temperatures.
- If uncertain about comparability, prefer separating into different clusters.

Rules:
- Each keypoint_id must appear in exactly one cluster.
- A cluster may include multiple points on the same physical object OR points across different objects that are thermally comparable.
- Choose clustering that best supports temperature comparison within each cluster.

Return ONLY JSON per the provided schema.
"""

CLUSTER_SCHEMA: Dict[str, Any] = {
    "type": "object",
    "additionalProperties": False,
    "properties": {
        "clusters": {
            "type": "array",
            "items": {
                "type": "object",
                "additionalProperties": False,
                "properties": {
                    "cluster_id": {"type": "integer"},
                    "component_type": {"type": "string"},
                    "keypoint_ids": {"type": "array", "items": {"type": "integer"}, "minItems": 1},
                },
                "required": ["cluster_id", "component_type", "keypoint_ids"],
            },
            "minItems": 1,
        },
        "annotation_spec": {
            "type": "object",
            "additionalProperties": False,
            "properties": {
                "draw_cluster_tag_next_to_each_keypoint": {"type": "boolean"},
                "cluster_tag_format": {"type": "string"},
                "legend_box": {"type": "boolean"},
                "legend_entries": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "additionalProperties": False,
                        "properties": {
                            "cluster_id": {"type": "integer"},
                            "label": {"type": "string"},
                        },
                        "required": ["cluster_id", "label"],
                    },
                },
            },
            "required": ["draw_cluster_tag_next_to_each_keypoint", "cluster_tag_format", "legend_box", "legend_entries"],
        },
    },
    "required": ["clusters", "annotation_spec"],
}


def draw_keypoints_for_llm(image_bgr: np.ndarray, keypoints: List[Dict[str, Any]]) -> np.ndarray:
    out = image_bgr.copy()
    for kp in keypoints:
        x = int(round(float(kp["x"])))
        y = int(round(float(kp["y"])))
        kid = int(kp["id"])
        cv2.circle(out, (x, y), 14, (0, 255, 0), 2)
        cv2.circle(out, (x, y), 3, (0, 255, 0), -1)
        txt = str(kid)
        font = cv2.FONT_HERSHEY_SIMPLEX
        (tw, th), _ = cv2.getTextSize(txt, font, 0.55, 2)
        cv2.rectangle(out, (x + 10, y - th - 4), (x + 14 + tw, y + 4), (0, 0, 0), -1)
        cv2.putText(out, txt, (x + 12, y), font, 0.55, (0, 255, 0), 2)
    return out


def call_openai_clusters(
    image_bgr_with_ids: np.ndarray,
    keypoints: List[Dict[str, Any]],
    model: str = "gpt-4o",
    prompt: str = THERMAL_GROUPING_PROMPT,
    temperature: float = 0.1,
) -> Dict[str, Any]:
    try:
        from openai import OpenAI
    except Exception as e:
        raise ImportError("openai python package not available. Install/update: pip install -U openai") from e

    load_dotenv()
    client = OpenAI()
    b64 = _image_to_base64_jpeg(image_bgr_with_ids, max_size=1280)
    data_uri = f"data:image/jpeg;base64,{b64}"

    resp = client.responses.create(
        model=model,
        input=[
            {
                "role": "user",
                "content": [
                    {"type": "input_text", "text": prompt},
                    {"type": "input_image", "image_url": data_uri},
                ],
            }
        ],
        text={
            "format": {
                "type": "json_schema",
                "name": "cluster_result",
                "schema": CLUSTER_SCHEMA,
                "strict": True,
            }
        },
        temperature=float(temperature),
    )
    return json.loads(resp.output_text)


def fix_and_validate_clusters(
    clusters_obj: Dict[str, Any],
    keypoints: List[Dict[str, Any]],
) -> Dict[str, Any]:
    kp_ids = [int(k["id"]) for k in keypoints]
    kp_set = set(kp_ids)

    clusters = clusters_obj.get("clusters", [])
    if not isinstance(clusters, list) or not clusters:
        raise ValueError("No clusters returned.")

    cleaned = []
    seen = set()
    for c in clusters:
        ids = [int(x) for x in c.get("keypoint_ids", []) if int(x) in kp_set]
        ids_unique = []
        for i in ids:
            if i in seen:
                continue
            seen.add(i)
            ids_unique.append(i)
        if not ids_unique:
            continue
        cleaned.append({
            "cluster_id": int(c.get("cluster_id", 0)),
            "component_type": str(c.get("component_type", "unknown")).strip() or "unknown",
            "keypoint_ids": ids_unique,
        })

    missing = sorted(list(kp_set - seen))
    if missing:
        cleaned.append({
            "cluster_id": max([c["cluster_id"] for c in cleaned] + [-1]) + 1,
            "component_type": "unassigned",
            "keypoint_ids": missing,
        })

    cleaned_sorted = sorted(cleaned, key=lambda c: (c["cluster_id"], c["component_type"]))
    reindexed = []
    for new_id, c in enumerate(cleaned_sorted):
        reindexed.append({**c, "cluster_id": new_id})

    legend_entries = [{"cluster_id": c["cluster_id"], "label": f"C{c['cluster_id']}: {c['component_type']}"} for c in reindexed]

    return {
        "clusters": reindexed,
        "annotation_spec": {
            "draw_cluster_tag_next_to_each_keypoint": True,
            "cluster_tag_format": "C{cluster_id}",
            "legend_box": True,
            "legend_entries": legend_entries,
        },
    }


# =============================================================================
# Anomaly detection (within cluster)
# =============================================================================

def compute_anomalies_within_clusters(
    keypoints: List[Dict[str, Any]],
    clusters_fixed: Dict[str, Any],
    delta_threshold: float,
) -> Dict[str, Any]:
    kp_by_id = {int(k["id"]): k for k in keypoints}

    out_clusters = []
    anomalies = []

    for c in clusters_fixed.get("clusters", []):
        cid = int(c["cluster_id"])
        ids = [int(i) for i in c.get("keypoint_ids", [])]

        temp_kid_pairs: List[Tuple[float, int]] = []
        for kid in ids:
            kp = kp_by_id.get(kid)
            if not kp:
                continue
            t = kp.get("temperature", None)
            if t is None:
                continue
            try:
                t = float(t)
            except Exception:
                continue
            if not np.isfinite(t):
                continue
            temp_kid_pairs.append((t, kid))

        cluster_info = {
            "cluster_id": cid,
            "component_type": c.get("component_type", "unknown"),
            "keypoint_ids": ids,
            "valid_temperature_points": len(temp_kid_pairs),
            "delta_threshold_c": float(delta_threshold),
        }

        if len(temp_kid_pairs) >= 2:
            temps = [p[0] for p in temp_kid_pairs]
            arr = np.array(temps, dtype=np.float64)
            med = float(np.nanmedian(arr))

            min_idx = int(np.nanargmin(arr))
            max_idx = int(np.nanargmax(arr))
            tmin = float(temps[min_idx])
            tmax = float(temps[max_idx])
            min_kid = int(temp_kid_pairs[min_idx][1])
            max_kid = int(temp_kid_pairs[max_idx][1])

            rng = tmax - tmin
            cluster_info.update({
                "median_c": round(med, 2),
                "min_c": round(tmin, 2),
                "min_keypoint_id": min_kid,
                "max_c": round(tmax, 2),
                "max_keypoint_id": max_kid,
                "range_c": round(rng, 2),
                "range_exceeds_threshold": bool(rng >= delta_threshold),
            })

            cluster_anomaly_ids = []
            for kid in ids:
                kp = kp_by_id.get(kid)
                if not kp:
                    continue
                t = kp.get("temperature", None)
                if t is None:
                    continue
                try:
                    t = float(t)
                except Exception:
                    continue
                if not np.isfinite(t):
                    continue

                hot = (t - med) >= delta_threshold
                cold = (med - t) >= delta_threshold
                if hot or cold:
                    magnitude = abs(t - med)
                    severity = "low"
                    if magnitude >= 2 * delta_threshold:
                        severity = "high"
                    elif magnitude >= 1.5 * delta_threshold:
                        severity = "medium"

                    anomalies.append({
                        "keypoint_id": int(kid),
                        "cluster_id": cid,
                        "component_type": c.get("component_type", "unknown"),
                        "temperature_c": round(t, 2),
                        "cluster_median_c": round(med, 2),
                        "delta_to_median_c": round(t - med, 2),
                        "anomaly_type": "hotspot" if hot else "dead_connection",
                        "severity": severity,
                    })
                    cluster_anomaly_ids.append(int(kid))

            cluster_info["anomaly_count"] = len(cluster_anomaly_ids)
            cluster_info["anomaly_keypoint_ids"] = cluster_anomaly_ids

        else:
            cluster_info.update({
                "median_c": None,
                "min_c": None,
                "min_keypoint_id": None,
                "max_c": None,
                "max_keypoint_id": None,
                "range_c": None,
                "range_exceeds_threshold": False,
                "anomaly_count": 0,
                "anomaly_keypoint_ids": [],
            })

        out_clusters.append(cluster_info)

    return {
        "delta_threshold_c": float(delta_threshold),
        "clusters": out_clusters,
        "anomalies": anomalies,
    }


# =============================================================================
# Visualization
# =============================================================================

def _tab_colors(n: int) -> List[Tuple[float, float, float, float]]:
    cmap = plt.get_cmap("tab10")
    return [cmap(i % 10) for i in range(max(1, n))]


def render_cluster_overlay_with_legend(
    base_bgr: np.ndarray,
    keypoints: List[Dict[str, Any]],
    clusters_fixed: Dict[str, Any],
    out_path: Path,
) -> None:
    _ensure_parent_dir(out_path)
    img = cv2.cvtColor(base_bgr, cv2.COLOR_BGR2RGB)

    clusters = clusters_fixed["clusters"]
    colors = _tab_colors(len(clusters))

    kp_to_c = {}
    for c in clusters:
        cid = int(c["cluster_id"])
        for kid in c["keypoint_ids"]:
            kp_to_c[int(kid)] = cid

    fig = plt.figure(figsize=(12, 9))
    ax = plt.gca()
    ax.imshow(img)
    ax.axis("off")

    for kp in keypoints:
        kid = int(kp["id"])
        cid = kp_to_c.get(kid, -1)
        x = float(kp["x"]); y = float(kp["y"])
        color = colors[cid] if (0 <= cid < len(colors)) else (0.5, 0.5, 0.5, 1.0)
        ax.add_patch(plt.Circle((x, y), 15, fill=False, linewidth=2, color=color))
        label = f"{kid}:C{cid}"
        ax.text(x + 8, y + 8, label, fontsize=7, color="white",
                bbox=dict(boxstyle="round,pad=0.2", facecolor=color, alpha=0.85))

    legend_patches = []
    for c in clusters:
        cid = int(c["cluster_id"])
        label = f"C{cid}: {c.get('component_type','unknown')}"
        legend_patches.append(mpatches.Patch(color=colors[cid], label=label))
    ax.legend(handles=legend_patches, loc="upper left", fontsize=9, framealpha=0.8)

    plt.tight_layout()
    plt.savefig(str(out_path), dpi=160, bbox_inches="tight")
    plt.close(fig)


def render_side_by_side_clusters_and_anomalies(
    base_bgr: np.ndarray,
    keypoints: List[Dict[str, Any]],
    clusters_fixed: Dict[str, Any],
    anomaly_report: Dict[str, Any],
    out_path: Path,
    title_left: str = "Clustering Results",
    title_right: str = "Thermal Anomalies",
    temp_summary: Optional[Dict[str, Any]] = None,
) -> None:
    _ensure_parent_dir(out_path)
    img = cv2.cvtColor(base_bgr, cv2.COLOR_BGR2RGB)

    clusters = clusters_fixed.get("clusters", [])
    colors = _tab_colors(len(clusters))

    kp_to_c = {}
    for c in clusters:
        cid = int(c["cluster_id"])
        for kid in c["keypoint_ids"]:
            kp_to_c[int(kid)] = cid

    anomalies = anomaly_report.get("anomalies", [])
    an_by_kid = {int(a["keypoint_id"]): a for a in anomalies}
    delta_threshold = float(anomaly_report.get("delta_threshold_c", 3.0))

    fig, axes = plt.subplots(1, 2, figsize=(18, 8))
    ax0, ax1 = axes

    # LEFT: clustering
    ax0.imshow(img)
    ax0.set_title(title_left, fontsize=12, fontweight="bold")
    ax0.axis("off")
    for kp in keypoints:
        kid = int(kp["id"])
        cid = kp_to_c.get(kid, -1)
        x = float(kp["x"]); y = float(kp["y"])
        color = colors[cid] if (0 <= cid < len(colors)) else (0.5, 0.5, 0.5, 1.0)
        ax0.add_patch(plt.Circle((x, y), 15, fill=False, linewidth=2, color=color))
        ax0.text(x + 8, y + 8, f"{kid}:C{cid}", fontsize=7, color="white",
                 bbox=dict(boxstyle="round,pad=0.2", facecolor=color, alpha=0.85))

    legend_patches = [
        mpatches.Patch(color=colors[int(c["cluster_id"])],
                       label=f"C{c['cluster_id']}: {c.get('component_type','unknown')}")
        for c in clusters
    ]
    if legend_patches:
        ax0.legend(handles=legend_patches, loc="upper right", fontsize=8, framealpha=0.9)

    # RIGHT: anomalies
    ax1.imshow(img)
    ax1.set_title(title_right, fontsize=12, fontweight="bold")
    ax1.axis("off")

    sev_color = {
        "low": (1.0, 1.0, 0.0, 1.0),
        "medium": (1.0, 0.65, 0.0, 1.0),
        "high": (1.0, 0.0, 0.0, 1.0),
    }
    sev_label = {"low": "LOW", "medium": "MED", "high": "HIGH"}

    for kp in keypoints:
        x = float(kp["x"]); y = float(kp["y"])
        ax1.add_patch(plt.Circle((x, y), 12, fill=False, linewidth=1, color=(0.7, 0.7, 0.7, 0.5)))

    for kp in keypoints:
        kid = int(kp["id"])
        if kid not in an_by_kid:
            continue
        a = an_by_kid[kid]
        sev = a.get("severity", "low")
        color = sev_color.get(sev, sev_color["low"])
        x = float(kp["x"]); y = float(kp["y"])
        temp = float(a.get("temperature_c", 0.0))
        delta = float(a.get("delta_to_median_c", 0.0))
        anom_type = a.get("anomaly_type", "anomaly")

        ax1.add_patch(plt.Circle((x, y), 18, fill=False, linewidth=3, color=color))
        if anom_type == "dead_connection":
            ax1.plot([x - 10, x + 10], [y - 10, y + 10], linewidth=2, color=color)
            ax1.plot([x - 10, x + 10], [y + 10, y - 10], linewidth=2, color=color)

        delta_str = f"+{delta:.1f}" if delta >= 0 else f"{delta:.1f}"
        temp_label = f"Sp{kid} {temp:.1f}°C ({delta_str})"
        ax1.text(x + 12, y - 5, temp_label, fontsize=8, color="white", fontweight="bold",
                 bbox=dict(boxstyle="round,pad=0.3", facecolor=color, alpha=0.9,
                           edgecolor="white", linewidth=0.5))

    # Info box
    if temp_summary is None:
        temps = [kp.get("temperature") for kp in keypoints if kp.get("temperature") is not None]
        temps = [float(t) for t in temps if t is not None and np.isfinite(float(t))]
        temp_summary = {
            "count": len(temps),
            "min_c": min(temps) if temps else 0.0,
            "max_c": max(temps) if temps else 0.0,
            "delta_c": (max(temps) - min(temps)) if temps else 0.0,
        }

    cluster_stats = anomaly_report.get("clusters", [])

    info_lines = []
    info_lines.append("─" * 32)
    info_lines.append("📊 THERMAL SUMMARY")
    info_lines.append("─" * 32)
    info_lines.append(
        f"  Points: {temp_summary.get('count', len(keypoints))} | "
        f"Range: {float(temp_summary.get('min_c', 0.0)):.1f}-{float(temp_summary.get('max_c', 0.0)):.1f}°C"
    )
    info_lines.append(f"  Threshold: Δ ≥ {delta_threshold:.1f}°C | Anomalies: {len(anomalies)}")

    if cluster_stats:
        info_lines.append("")
        info_lines.append("─" * 32)
        info_lines.append("📋 CLUSTERS")
        info_lines.append("─" * 32)
        for cs in cluster_stats:
            cid = cs.get("cluster_id", "?")
            ctype = str(cs.get("component_type", "unknown"))
            if len(ctype) > 16:
                ctype = ctype[:14] + ".."
            n_pts = cs.get("valid_temperature_points", 0)
            info_lines.append(f"C{cid} {ctype} ({n_pts} pts)")
            if cs.get("median_c") is not None:
                med = float(cs.get("median_c", 0.0))
                tmin = float(cs.get("min_c", 0.0))
                tmax = float(cs.get("max_c", 0.0))
                min_kid = cs.get("min_keypoint_id", "?")
                max_kid = cs.get("max_keypoint_id", "?")
                anomaly_ids = cs.get("anomaly_keypoint_ids", [])
                info_lines.append(f"  Med: {med:.1f}°C | {tmin:.1f}-{tmax:.1f}°C")
                min_marker = "⚠" if min_kid in anomaly_ids else ""
                max_marker = "⚠" if max_kid in anomaly_ids else ""
                info_lines.append(f"  ↓Sp{min_kid}{min_marker}  ↑Sp{max_kid}{max_marker}")
            else:
                info_lines.append("  (insufficient data)")

    if anomalies:
        info_lines.append("")
        info_lines.append("─" * 32)
        info_lines.append(f"🔥 ANOMALIES ({len(anomalies)})")
        info_lines.append("─" * 32)
        sev_order = {"high": 0, "medium": 1, "low": 2}
        sorted_anomalies = sorted(
            anomalies,
            key=lambda a: (sev_order.get(a.get("severity", "low"), 3), -abs(float(a.get("delta_to_median_c", 0.0)))),
        )
        for a in sorted_anomalies:
            kid = a.get("keypoint_id", "?")
            temp = float(a.get("temperature_c", 0.0))
            delta = float(a.get("delta_to_median_c", 0.0))
            sev = sev_label.get(a.get("severity", "low"), "?")
            anom_type = a.get("anomaly_type", "unknown")
            type_short = "hot" if anom_type == "hotspot" else "dead"
            delta_str = f"+{delta:.1f}" if delta >= 0 else f"{delta:.1f}"
            info_lines.append(f"Sp{kid:<3} {temp:>5.1f}°C {delta_str:>5} {sev:<4} {type_short}")
    else:
        info_lines.append("")
        info_lines.append("─" * 32)
        info_lines.append("✅ NO ANOMALIES DETECTED")
        info_lines.append("─" * 32)

    info_text = "\n".join(info_lines)
    ax1.text(
        0.02, 0.02, info_text, transform=ax1.transAxes,
        fontsize=8, fontfamily="monospace", verticalalignment="bottom",
        bbox=dict(boxstyle="round,pad=0.5", facecolor="black", alpha=0.85,
                  edgecolor="white", linewidth=1),
        color="white"
    )

    severity_patches = [
        mpatches.Patch(color=sev_color["low"], label="Low (Δ ≥ 3°C)"),
        mpatches.Patch(color=sev_color["medium"], label="Medium (Δ ≥ 4.5°C)"),
        mpatches.Patch(color=sev_color["high"], label="High (Δ ≥ 6°C)"),
    ]
    ax1.legend(handles=severity_patches, loc="upper right", fontsize=7, framealpha=0.9)

    plt.tight_layout()
    plt.savefig(str(out_path), dpi=160, bbox_inches="tight")
    plt.close(fig)


# =============================================================================
# NEW: Dual Visualization (Thermal + RGB) Functions
# =============================================================================

def draw_keypoints_simple(
    img_bgr: np.ndarray,
    keypoints: List[Dict],
    coord_key_x: str = "x",
    coord_key_y: str = "y",
    color: Tuple[int, int, int] = (0, 255, 255),
    radius: int = 8,
    thickness: int = 2,
) -> np.ndarray:
    """Desenha keypoints com IDs em uma imagem."""
    out = img_bgr.copy()
    for kp in keypoints:
        x = kp.get(coord_key_x)
        y = kp.get(coord_key_y)
        if x is None or y is None:
            continue
        x, y = int(float(x)), int(float(y))
        kid = kp.get("id", "?")
        cv2.circle(out, (x, y), radius, color, thickness)
        label = f"Sp{kid}"
        cv2.putText(out, label, (x + radius + 2, y + 4),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.4, color, 1, cv2.LINE_AA)
    return out


def draw_clusters_simple(
    img_bgr: np.ndarray,
    keypoints: List[Dict],
    clusters: List[Dict],
    coord_key_x: str = "x",
    coord_key_y: str = "y",
    radius: int = 10,
    thickness: int = 2,
) -> np.ndarray:
    """Desenha keypoints coloridos por cluster."""
    out = img_bgr.copy()
    cmap = plt.cm.get_cmap("tab10")
    colors = [tuple(int(c * 255) for c in cmap(i)[:3][::-1]) for i in range(10)]
    
    kp_to_cluster = {}
    for c in clusters:
        cid = c.get("cluster_id", 0)
        for kid in c.get("keypoint_ids", []):
            kp_to_cluster[int(kid)] = int(cid)
    
    for kp in keypoints:
        x = kp.get(coord_key_x)
        y = kp.get(coord_key_y)
        if x is None or y is None:
            continue
        x, y = int(float(x)), int(float(y))
        kid = int(kp.get("id", 0))
        cid = kp_to_cluster.get(kid, 0)
        color = colors[cid % len(colors)]
        cv2.circle(out, (x, y), radius, color, thickness)
        label = f"{kid}:C{cid}"
        cv2.putText(out, label, (x + radius + 2, y + 4),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.35, color, 1, cv2.LINE_AA)
    return out


def draw_anomalies_simple(
    img_bgr: np.ndarray,
    keypoints: List[Dict],
    anomalies: List[Dict],
    coord_key_x: str = "x",
    coord_key_y: str = "y",
    radius: int = 12,
    thickness: int = 3,
) -> np.ndarray:
    """Desenha anomalias com cores por severidade."""
    out = img_bgr.copy()
    sev_colors = {
        "low": (0, 255, 255),      # Amarelo
        "medium": (0, 165, 255),   # Laranja
        "high": (0, 0, 255),       # Vermelho
    }
    an_by_kid = {int(a.get("keypoint_id", 0)): a for a in anomalies}
    
    # Todos os keypoints em cinza primeiro
    for kp in keypoints:
        x = kp.get(coord_key_x)
        y = kp.get(coord_key_y)
        if x is None or y is None:
            continue
        x, y = int(float(x)), int(float(y))
        cv2.circle(out, (x, y), radius - 4, (128, 128, 128), 1)
    
    # Anomalias destacadas
    for kp in keypoints:
        kid = int(kp.get("id", 0))
        if kid not in an_by_kid:
            continue
        x = kp.get(coord_key_x)
        y = kp.get(coord_key_y)
        if x is None or y is None:
            continue
        x, y = int(float(x)), int(float(y))
        a = an_by_kid[kid]
        sev = a.get("severity", "low")
        color = sev_colors.get(sev, sev_colors["low"])
        temp = float(a.get("temperature_c", 0))
        delta = float(a.get("delta_to_median_c", 0))
        cv2.circle(out, (x, y), radius, color, thickness)
        delta_str = f"+{delta:.1f}" if delta >= 0 else f"{delta:.1f}"
        label = f"Sp{kid}: {temp:.1f}C ({delta_str})"
        cv2.putText(out, label, (x + radius + 2, y + 4),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.35, color, 1, cv2.LINE_AA)
    return out


def create_simple_side_by_side(img1: np.ndarray, img2: np.ndarray, img3: np.ndarray,
                                title1: str = "", title2: str = "", title3: str = "") -> np.ndarray:
    """Cria imagem side-by-side com 3 painéis."""
    target_h = max(img1.shape[0], img2.shape[0], img3.shape[0])
    
    def resize_h(img, th):
        if img.shape[0] == th:
            return img
        scale = th / img.shape[0]
        return cv2.resize(img, (int(img.shape[1] * scale), th))
    
    img1 = resize_h(img1, target_h)
    img2 = resize_h(img2, target_h)
    img3 = resize_h(img3, target_h)
    combined = np.hstack([img1, img2, img3])
    
    font = cv2.FONT_HERSHEY_SIMPLEX
    w1, w2 = img1.shape[1], img2.shape[1]
    if title1:
        cv2.putText(combined, title1, (10, 25), font, 0.6, (255, 255, 255), 2)
    if title2:
        cv2.putText(combined, title2, (w1 + 10, 25), font, 0.6, (255, 255, 255), 2)
    if title3:
        cv2.putText(combined, title3, (w1 + w2 + 10, 25), font, 0.6, (255, 255, 255), 2)
    return combined


def create_combined_4panel(
    thermal_orig: np.ndarray, thermal_clust: np.ndarray,
    rgb_orig: np.ndarray, rgb_clust: np.ndarray,
    title: str = "",
) -> np.ndarray:
    """Cria view com 4 painéis (2x2): thermal/rgb × original/clustered."""
    target_h = 480
    
    def resize_h(img, th):
        scale = th / img.shape[0]
        return cv2.resize(img, (int(img.shape[1] * scale), th))
    
    t_orig = resize_h(thermal_orig, target_h)
    t_clust = resize_h(thermal_clust, target_h)
    r_orig = resize_h(rgb_orig, target_h)
    r_clust = resize_h(rgb_clust, target_h)
    
    w1 = max(t_orig.shape[1], r_orig.shape[1])
    w2 = max(t_clust.shape[1], r_clust.shape[1])
    
    def pad_w(img, tw):
        if img.shape[1] >= tw:
            return img
        return cv2.copyMakeBorder(img, 0, 0, 0, tw - img.shape[1], cv2.BORDER_CONSTANT, value=(0, 0, 0))
    
    row1 = np.hstack([pad_w(t_orig, w1), pad_w(t_clust, w2)])
    row2 = np.hstack([pad_w(r_orig, w1), pad_w(r_clust, w2)])
    combined = np.vstack([row1, row2])
    
    font = cv2.FONT_HERSHEY_SIMPLEX
    cv2.putText(combined, "Thermal Original", (10, 22), font, 0.5, (255, 255, 255), 1)
    cv2.putText(combined, "Thermal Clustered", (w1 + 10, 22), font, 0.5, (255, 255, 255), 1)
    cv2.putText(combined, "RGB Original", (10, target_h + 22), font, 0.5, (255, 255, 255), 1)
    cv2.putText(combined, "RGB Clustered", (w1 + 10, target_h + 22), font, 0.5, (255, 255, 255), 1)
    if title:
        cv2.putText(combined, title, (combined.shape[1] // 2 - 100, combined.shape[0] - 8),
                   font, 0.4, (180, 180, 180), 1)
    return combined


def generate_dual_visualizations(
    thermal_clean_bgr: np.ndarray,
    rgb_bgr: np.ndarray,
    keypoints: List[Dict],
    clusters: List[Dict],
    anomalies: List[Dict],
    output_dir: Path,
    prefix: str,
) -> Dict[str, Path]:
    """
    Gera visualizações em AMBAS as imagens (thermal + RGB) em pastas separadas.
    """
    thermal_dir = output_dir / "thermal"
    rgb_dir = output_dir / "rgb"
    thermal_dir.mkdir(parents=True, exist_ok=True)
    rgb_dir.mkdir(parents=True, exist_ok=True)
    outputs = {}
    
    # === THERMAL ===
    thermal_kp = draw_keypoints_simple(thermal_clean_bgr, keypoints,
                                        "x_thermal", "y_thermal", (0, 255, 255), 6, 2)
    path_t_kp = thermal_dir / f"{prefix}_thermal_keypoints.png"
    cv2.imwrite(str(path_t_kp), thermal_kp)
    outputs["thermal_keypoints"] = path_t_kp
    
    thermal_cl = draw_clusters_simple(thermal_clean_bgr, keypoints, clusters,
                                       "x_thermal", "y_thermal", 8, 2)
    path_t_cl = thermal_dir / f"{prefix}_thermal_clustered.png"
    cv2.imwrite(str(path_t_cl), thermal_cl)
    outputs["thermal_clustered"] = path_t_cl
    
    thermal_an = draw_anomalies_simple(thermal_clean_bgr, keypoints, anomalies,
                                        "x_thermal", "y_thermal", 10, 2)
    path_t_an = thermal_dir / f"{prefix}_thermal_anomalies.png"
    cv2.imwrite(str(path_t_an), thermal_an)
    outputs["thermal_anomalies"] = path_t_an
    
    thermal_sbs = create_simple_side_by_side(thermal_clean_bgr, thermal_cl, thermal_an,
                                              "Original", "Clusters", "Anomalies")
    path_t_sbs = thermal_dir / f"{prefix}_thermal_side_by_side.png"
    cv2.imwrite(str(path_t_sbs), thermal_sbs)
    outputs["thermal_side_by_side"] = path_t_sbs
    
    # === RGB ===
    rgb_kp = draw_keypoints_simple(rgb_bgr, keypoints, "x", "y", (0, 255, 255), 10, 2)
    path_r_kp = rgb_dir / f"{prefix}_rgb_keypoints.png"
    cv2.imwrite(str(path_r_kp), rgb_kp)
    outputs["rgb_keypoints"] = path_r_kp
    
    rgb_cl = draw_clusters_simple(rgb_bgr, keypoints, clusters, "x", "y", 12, 2)
    path_r_cl = rgb_dir / f"{prefix}_rgb_clustered.png"
    cv2.imwrite(str(path_r_cl), rgb_cl)
    outputs["rgb_clustered"] = path_r_cl
    
    rgb_an = draw_anomalies_simple(rgb_bgr, keypoints, anomalies, "x", "y", 15, 3)
    path_r_an = rgb_dir / f"{prefix}_rgb_anomalies.png"
    cv2.imwrite(str(path_r_an), rgb_an)
    outputs["rgb_anomalies"] = path_r_an
    
    rgb_sbs = create_simple_side_by_side(rgb_bgr, rgb_cl, rgb_an,
                                          "Original", "Clusters", "Anomalies")
    path_r_sbs = rgb_dir / f"{prefix}_rgb_side_by_side.png"
    cv2.imwrite(str(path_r_sbs), rgb_sbs)
    outputs["rgb_side_by_side"] = path_r_sbs
    
    # === COMBINED ===
    combined = create_combined_4panel(thermal_clean_bgr, thermal_cl, rgb_bgr, rgb_cl, prefix)
    path_comb = output_dir / f"{prefix}_combined_view.png"
    cv2.imwrite(str(path_comb), combined)
    outputs["combined_view"] = path_comb
    
    return outputs


def inject_spots_to_exif(
    source_raw: Path, 
    output_path: Path, 
    keypoints: List[Dict],
    atlas_include: str = "",
    atlas_libdir: str = "",
    atlas_libs: str = "atlas_c_sdk",
) -> bool:
    """
    Injeta spots detectados na imagem FLIR usando Atlas C SDK.
    
    O Atlas SDK é a única forma de criar spots de medição em imagens RAW
    que são reconhecidos pelo FLIR Tools. exiftool não consegue escrever
    as tags proprietárias FLIR:MeasNParams.
    
    Args:
        source_raw: Imagem FLIR de entrada
        output_path: Caminho de saída
        keypoints: Lista de keypoints com x_thermal/y_thermal
        atlas_include: Caminho para includes do Atlas SDK
        atlas_libdir: Caminho para libs do Atlas SDK
        atlas_libs: Nome das bibliotecas do Atlas
    
    Returns:
        True se sucesso
    """
    import shutil

    if not atlas_include or not atlas_libdir:
        repo_root = Path(__file__).resolve().parents[2]
        atlas_root = repo_root / "flir_atlas_c"
        if not atlas_include:
            atlas_include = str(atlas_root / "include")
        if not atlas_libdir:
            atlas_libdir = str(atlas_root / "lib")
    
    # Extrair coordenadas térmicas
    coords = []
    for kp in keypoints:
        x = kp.get("x_thermal", kp.get("x_yolo", kp.get("x", 0)))
        y = kp.get("y_thermal", kp.get("y_yolo", kp.get("y", 0)))
        x_int = int(round(float(x)))
        y_int = int(round(float(y)))
        coords.append((x_int, y_int))
    
    if not coords:
        print("⚠️  Nenhuma coordenada para injetar")
        shutil.copy2(source_raw, output_path)
        return True
    
    # Garantir executável Atlas compilado
    exe_path = _ensure_atlas_tool(atlas_include, atlas_libdir, atlas_libs)
    if exe_path is None:
        print("⚠️  Não foi possível compilar o executável Atlas")
        return False
    
    # Montar comando: atlas_write_spots <in> <out> <N> x1 y1 x2 y2 ...
    cmd = [str(exe_path), str(source_raw), str(output_path), str(len(coords))]
    for x, y in coords:
        cmd.extend([str(x), str(y)])
    
    try:
        env = dict(os.environ)
        existing = env.get("LD_LIBRARY_PATH", "")
        env["LD_LIBRARY_PATH"] = f"{atlas_libdir}:{existing}" if existing else str(atlas_libdir)
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60, env=env)
        if result.returncode != 0:
            print(f"⚠️  Atlas SDK error: {result.stderr or result.stdout}")
            return False
        
        if output_path.exists():
            return True
        else:
            print("⚠️  Arquivo de saída não foi criado")
            return False
    except subprocess.TimeoutExpired:
        print("⚠️  Timeout ao injetar spots")
        return False
    except Exception as e:
        print(f"⚠️  Erro ao injetar spots: {e}")
        return False


# Atlas C SDK code (embedded)
_ATLAS_WRITE_SPOTS_C = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <acs/enum.h>
#include <acs/measurements.h>
#include <acs/measurement_spot.h>
#include <acs/thermal_image.h>

int main(int argc, char** argv) {
    if (argc < 4) { fprintf(stderr, "uso: %s <in> <out> <N> [x1 y1 ...]\n", argv[0]); return 2; }
    const char* in_path = argv[1];
    const char* out_path = argv[2];
    int n = atoi(argv[3]);
    if (n < 0 || argc != 4 + 2*n) { fprintf(stderr, "args invalidos\n"); return 2; }

    ACS_ThermalImage* image = ACS_ThermalImage_alloc();
    if (!image) { fprintf(stderr, "alloc falhou\n"); return 3; }
    ACS_ThermalImage_openFromFile(image, (const ACS_NativePathChar*)in_path);
    ACS_Measurements* ms = ACS_ThermalImage_getMeasurements(image);

    for (int i = 0; i < n; ++i) {
        int x = atoi(argv[4 + 2*i]);
        int y = atoi(argv[4 + 2*i + 1]);
        ACS_MeasurementSpot* spot = ACS_Measurements_addSpot(ms, x, y);
        if (!spot) fprintf(stderr, "addSpot(%d,%d) falhou\n", x, y);
    }
    ACS_ThermalImage_saveAs(image, (const ACS_NativePathChar*)out_path, ACS_FileFormat_jpeg);
    ACS_ThermalImage_free(image);
    return 0;
}
'''


def _ensure_atlas_tool(
    atlas_include: str,
    atlas_libdir: str,
    atlas_libs: str = "atlas_c_sdk",
) -> Optional[Path]:
    """Compila o executável atlas_write_spots se necessário."""
    import shutil
    
    build_dir = Path.home() / ".atlas_build"
    build_dir.mkdir(parents=True, exist_ok=True)
    
    exe_path = build_dir / "atlas_write_spots"
    src_path = build_dir / "atlas_write_spots.c"
    meta_path = build_dir / "atlas_write_spots.meta.json"
    
    # Se já existe e a config é a mesma, retornar
    if exe_path.exists() and meta_path.exists():
        try:
            meta = json.loads(meta_path.read_text(encoding="utf-8"))
            if (
                meta.get("atlas_include") == str(atlas_include)
                and meta.get("atlas_libdir") == str(atlas_libdir)
                and meta.get("atlas_libs") == str(atlas_libs)
            ):
                return exe_path
        except Exception:
            pass
    
    # Escrever código fonte
    src_path.write_text(_ATLAS_WRITE_SPOTS_C)
    
    # Compilar
    cc = shutil.which("gcc") or shutil.which("cc")
    if not cc:
        print("⚠️  Compilador C não encontrado")
        return None
    
    cmd = [cc, "-o", str(exe_path), str(src_path)]
    cmd.extend(["-I", atlas_include])
    cmd.extend(["-L", atlas_libdir])
    cmd.extend([f"-Wl,-rpath,{atlas_libdir}"])
    for lib in atlas_libs.split(","):
        cmd.append(f"-l{lib.strip()}")
    
    print(f"   Compilando Atlas tool: {' '.join(cmd[:5])}...")
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
    
    if result.returncode != 0:
        print(f"⚠️  Compilação falhou: {result.stderr}")
        return None

    if exe_path.exists():
        try:
            exe_path.chmod(exe_path.stat().st_mode | 0o111)
        except Exception:
            pass
        try:
            meta_path.write_text(
                json.dumps(
                    {"atlas_include": str(atlas_include), "atlas_libdir": str(atlas_libdir), "atlas_libs": str(atlas_libs)},
                    indent=2,
                    ensure_ascii=False,
                ),
                encoding="utf-8",
            )
        except Exception:
            pass
        print(f"   ✅ Compilado: {exe_path}")
        return exe_path
    
    return None


# =============================================================================
# Main
# =============================================================================

def main(argv: Optional[List[str]] = None) -> None:
    parser = argparse.ArgumentParser(
        description="WP4→WP5 integration v4: RAW-only + Dual Viz + EXIF injection",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    # Input images - RAW ou Edited
    parser.add_argument("--raw-image", help="FLIR RAW image (preferred for RAW-only mode)")
    parser.add_argument("--edited-image", help="FLIR Edited image (legacy mode)")
    parser.add_argument("--output", "-o", default="wp5_results", help="Output directory")
    parser.add_argument("--output-prefix", default=None, help="Prefix for output files")

    # Base image selection
    parser.add_argument("--rgb-image", help="RGB image to cluster on (official mode)")
    parser.add_argument("--thermal-clean-image", help="Thermal clean PNG to cluster on (experimental)")
    parser.add_argument("--thermal-source-image", help="FLIR source image to extract RawThermalImage (optional override)")

    # Alignment mapping
    parser.add_argument("--scale-to-rgb", action="store_true", help="Map thermal keypoints to RGB space (recommended with --rgb-image)")
    parser.add_argument("--real2ir", type=float, default=None, help="Override Real2IR")
    parser.add_argument("--offset-x", type=float, default=None, help="Override OffsetX")
    parser.add_argument("--offset-y", type=float, default=None, help="Override OffsetY")
    parser.add_argument("--clip-outside", action="store_true", help="Drop points that map outside target image")

    # Keypoint source
    parser.add_argument("--keypoint-source", choices=["yolo", "exif"], default="yolo",
                        help="Source of keypoints: exif or yolo")
    parser.add_argument("--yolo-model", default=None, help="YOLO model path (.pt). Required if --keypoint-source yolo.")
    parser.add_argument("--yolo-conf", type=float, default=0.30, help="YOLO confidence threshold")
    parser.add_argument("--yolo-iou", type=float, default=0.45, help="YOLO IoU threshold")
    parser.add_argument("--yolo-device", default="auto", help="YOLO device (auto, cpu, 0, 1, ...)")
    parser.add_argument("--yolo-image", default=None,
                        help="Image path to run YOLO on (e.g., cleaned thermal PNG). If omitted, falls back to --thermal-clean-image if provided.")

    # Temperature + anomalies
    parser.add_argument("--extract-temperatures", action="store_true", help="Compute per-keypoint temperatures via flirimageextractor")
    parser.add_argument("--kernel-size", type=int, default=3, help="Kernel size around keypoint in thermal pixels")
    parser.add_argument("--delta-threshold", type=float, default=3.0, help="Delta threshold (°C) for anomalies")

    # OpenAI
    parser.add_argument("--clustering-method", default="vllm", choices=["vllm"],
                        help="Kept for compatibility; uses OpenAI.")
    parser.add_argument("--openai-model", default="gpt-4o", help="OpenAI vision model for clustering")
    parser.add_argument("--llm-temperature", type=float, default=0.1, help="Sampling temperature for clustering")

    # NEW: EXIF injection and dual visualization
    parser.add_argument("--inject-exif", action="store_true", help="Inject detected spots into EXIF for FLIR Tools (uses Atlas SDK)")
    parser.add_argument("--dual-viz", action="store_true", default=True, help="Generate visualizations in both thermal and RGB")
    
    # Atlas SDK configuration
    parser.add_argument("--atlas-include", default="",
                        help="Atlas SDK include directory")
    parser.add_argument("--atlas-libdir", default="",
                        help="Atlas SDK library directory")
    parser.add_argument("--atlas-libs", default="atlas_c_sdk",
                        help="Atlas SDK library name(s), comma-separated")

    args = parser.parse_args(argv)

    _load_env_if_available()

    # Resolve Atlas SDK paths (repo-local default)
    repo_root = Path(__file__).resolve().parents[2]
    atlas_root = repo_root / "flir_atlas_c"
    if (not args.atlas_include) or (not Path(str(args.atlas_include)).is_dir()):
        args.atlas_include = str(atlas_root / "include")
    if (not args.atlas_libdir) or (not Path(str(args.atlas_libdir)).is_dir()):
        args.atlas_libdir = str(atlas_root / "lib")

    # Determine source image (RAW or Edited)
    source_image: Optional[Path] = None
    raw_mode = False
    if args.raw_image:
        source_image = Path(args.raw_image)
        raw_mode = True
    elif args.edited_image:
        source_image = Path(args.edited_image)
        raw_mode = False
    else:
        print("❌ You must provide either --raw-image or --edited-image")
        sys.exit(2)
    
    if not source_image.exists():
        print(f"❌ Source image not found: {source_image}")
        sys.exit(2)

    out_dir = Path(args.output)
    out_dir.mkdir(parents=True, exist_ok=True)
    
    # Output prefix
    prefix = args.output_prefix if args.output_prefix else source_image.stem

    print("=" * 70)
    print("WP4 → WP5 Integration (v5): RAW-Only + Dual Viz + EXIF Injection")
    print("=" * 70)
    print(f"\n📸 Source: {source_image} ({'RAW' if raw_mode else 'Edited'})")

    exif = extract_exif_from_image(source_image)
    params = get_alignment_params_from_exif(exif)
    th_size = (int(params["thermal_width"]), int(params["thermal_height"]))

    # Mostrar parâmetros de alinhamento EXIF
    print(f"\n📐 Alignment Parameters (from EXIF):")
    print(f"   Real2IR:    {params['real2ir']:.4f}")
    print(f"   OffsetX:    {params['offset_x']:.1f}")
    print(f"   OffsetY:    {params['offset_y']:.1f}")
    print(f"   Thermal:    {params['thermal_width']}x{params['thermal_height']}")
    print(f"   RGB:        {params['rgb_width']}x{params['rgb_height']}")
    
    # Indicar se há overrides via CLI
    cli_overrides = []
    if args.real2ir is not None:
        cli_overrides.append(f"Real2IR={args.real2ir}")
    if args.offset_x is not None:
        cli_overrides.append(f"OffsetX={args.offset_x}")
    if args.offset_y is not None:
        cli_overrides.append(f"OffsetY={args.offset_y}")
    
    if cli_overrides:
        print(f"   ⚠️  CLI Overrides: {', '.join(cli_overrides)}")

    # Decide base image mode
    mode = None
    base_image_path: Optional[Path] = None
    if args.rgb_image:
        mode = "rgb"
        base_image_path = Path(args.rgb_image)
    elif args.thermal_clean_image:
        mode = "thermal"
        base_image_path = Path(args.thermal_clean_image)
    else:
        print("❌ You must provide either --rgb-image (official) or --thermal-clean-image (experimental).")
        sys.exit(2)

    base_bgr = _load_image_bgr(base_image_path)
    target_size = (int(base_bgr.shape[1]), int(base_bgr.shape[0]))

    # Keypoints
    print(f"\n🎯 Keypoint source: {args.keypoint_source.upper()}")
    if args.keypoint_source == "yolo":
        if not args.yolo_model:
            print("❌ --keypoint-source yolo requires --yolo-model")
            sys.exit(2)

        yolo_img = Path(args.yolo_image) if args.yolo_image else None
        if (yolo_img is None) or (not yolo_img.exists()):
            if args.thermal_clean_image and Path(args.thermal_clean_image).exists():
                yolo_img = Path(args.thermal_clean_image)

        if (yolo_img is None) or (not yolo_img.exists()):
            print("❌ YOLO requires --yolo-image (or a valid --thermal-clean-image).")
            sys.exit(2)

        yolo_size = _get_image_size_wh(yolo_img)
        yolo_kps = extract_keypoints_from_yolo(
            image_path=yolo_img,
            model_path=Path(args.yolo_model),
            conf_threshold=float(args.yolo_conf),
            iou_threshold=float(args.yolo_iou),
            device=str(args.yolo_device),
        )
        if not yolo_kps:
            print("❌ No keypoints found by YOLO.")
            sys.exit(2)

        keypoints = map_yolo_keypoints_to_thermal_space(yolo_kps, yolo_size=yolo_size, thermal_size=th_size)
        print(f"   ✅ YOLO points: {len(keypoints)} | yolo {yolo_size[0]}x{yolo_size[1]} -> thermal {th_size[0]}x{th_size[1]}")
    else:
        keypoints = extract_keypoints_from_exif(exif)
        if not keypoints:
            print("❌ No keypoints found in EXIF.")
            sys.exit(2)
        print(f"   ✅ EXIF points: {len(keypoints)}")

    # Temperatures (thermal space)
    temp_summary: Dict[str, Any] = {}
    if args.extract_temperatures:
        source_for_raw = Path(args.thermal_source_image) if args.thermal_source_image else source_image
        print("\n🌡️  Extracting temperatures (flirimageextractor)...")
        keypoints, temp_summary = extract_temperatures_for_keypoints(
            flir_source_image=source_for_raw,
            exif=exif,
            keypoints=keypoints,
            kernel_size=int(args.kernel_size),
        )
        if temp_summary.get("count", 0) > 0:
            print(
                f"   ✅ Temperatures: {temp_summary['min_c']:.2f}°C .. {temp_summary['max_c']:.2f}°C "
                f"(Δ={temp_summary['delta_c']:.2f}°C) [{temp_summary.get('method','unknown')}]"
            )
        else:
            print("   ⚠️  No temperatures computed (missing/invalid radiometric data).")

    # Mapping to base image
    if mode == "rgb":
        if args.scale_to_rgb:
            real2ir = float(args.real2ir) if args.real2ir is not None else float(params["real2ir"])
            offx = float(args.offset_x) if args.offset_x is not None else float(params["offset_x"])
            offy = float(args.offset_y) if args.offset_y is not None else float(params["offset_y"])
            # Indicar fonte de cada parâmetro
            r2i_src = "[CLI]" if args.real2ir is not None else "[EXIF]"
            offx_src = "[CLI]" if args.offset_x is not None else "[EXIF]"
            offy_src = "[CLI]" if args.offset_y is not None else "[EXIF]"
            
            print("\n📐 Mapping keypoints (Thermal → RGB):")
            print(f"   Thermal:  {th_size[0]}x{th_size[1]}")
            print(f"   RGB:      {target_size[0]}x{target_size[1]}")
            print(f"   Real2IR:  {real2ir:.4f} {r2i_src}")
            print(f"   OffsetX:  {offx:.1f} {offx_src}")
            print(f"   OffsetY:  {offy:.1f} {offy_src}")
            keypoints = transform_thermal_to_rgb(
                keypoints,
                thermal_size=th_size,
                rgb_size=target_size,
                real2ir=real2ir,
                offset_x=offx,
                offset_y=offy,
                clip_outside=bool(args.clip_outside),
            )
        else:
            print("\n⚠️  --scale-to-rgb not set: keypoints remain in thermal coords and will NOT overlay correctly on RGB.")
            # ensure x_thermal/y_thermal are set
            for kp in keypoints:
                kp["x_thermal"] = float(kp.get("x_thermal", kp["x"]))
                kp["y_thermal"] = float(kp.get("y_thermal", kp["y"]))
    else:
        print("\n📐 Mapping keypoints (Thermal → ThermalClean):")
        print(f"   Thermal: {th_size[0]}x{th_size[1]}")
        print(f"   Target:  {target_size[0]}x{target_size[1]}  ({base_image_path.name})")
        keypoints = transform_thermal_to_target_scale(
            keypoints,
            thermal_size=th_size,
            target_size=target_size,
            clip_outside=bool(args.clip_outside),
        )

    # Save keypoints
    kp_out = out_dir / "keypoints_used.json"
    _write_json(kp_out, keypoints)
    _write_json(out_dir / f"{base_image_path.stem}_keypoints_used.json", keypoints)

    # Prepare LLM image
    llm_img = draw_keypoints_for_llm(base_bgr, keypoints)
    llm_img_out = out_dir / f"{base_image_path.stem}_llm_keypoints.png"
    cv2.imwrite(str(llm_img_out), llm_img)

    # OpenAI clustering
    print(f"\n🤖 Clustering with OpenAI ({args.openai_model}) ...")
    clusters_raw = call_openai_clusters(
        image_bgr_with_ids=llm_img,
        keypoints=keypoints,
        model=str(args.openai_model),
        prompt=THERMAL_GROUPING_PROMPT,
        temperature=float(args.llm_temperature),
    )
    clusters_fixed = fix_and_validate_clusters(clusters_raw, keypoints)

    clusters_out = out_dir / f"{base_image_path.stem}_clusters.json"
    _write_json(clusters_out, clusters_fixed)

    overlay_out = out_dir / f"{base_image_path.stem}_clustered_overlay.png"
    render_cluster_overlay_with_legend(base_bgr, keypoints, clusters_fixed, overlay_out)

    # Anomalies
    anomaly_report = compute_anomalies_within_clusters(
        keypoints=keypoints,
        clusters_fixed=clusters_fixed,
        delta_threshold=float(args.delta_threshold),
    )
    anomalies_out = out_dir / "anomalies.json"
    _write_json(anomalies_out, anomaly_report)

    side_by_side_out = out_dir / f"{base_image_path.stem}_analysis_side_by_side.png"
    render_side_by_side_clusters_and_anomalies(
        base_bgr=base_bgr,
        keypoints=keypoints,
        clusters_fixed=clusters_fixed,
        anomaly_report=anomaly_report,
        out_path=side_by_side_out,
        title_left=f"Clustering Results ({len(clusters_fixed['clusters'])} clusters)\nModel: {args.openai_model}",
        title_right=f"Thermal Anomalies (Δ ≥ {float(args.delta_threshold):.1f}°C)",
        temp_summary=temp_summary if temp_summary else None,
    )

    # NEW: Dual visualizations (thermal + RGB)
    dual_viz_outputs = {}
    if args.dual_viz and args.thermal_clean_image and args.rgb_image:
        print("\n🎨 Generating dual visualizations (thermal + RGB)...")
        try:
            thermal_clean_bgr = cv2.imread(str(Path(args.thermal_clean_image)), cv2.IMREAD_COLOR)
            rgb_bgr_viz = cv2.imread(str(Path(args.rgb_image)), cv2.IMREAD_COLOR)
            
            if thermal_clean_bgr is not None and rgb_bgr_viz is not None:
                # Escalar thermal_clean se necessário
                if thermal_clean_bgr.shape[:2] != (th_size[1], th_size[0]):
                    thermal_clean_bgr = cv2.resize(thermal_clean_bgr, th_size)
                
                dual_viz_outputs = generate_dual_visualizations(
                    thermal_clean_bgr=thermal_clean_bgr,
                    rgb_bgr=rgb_bgr_viz,
                    keypoints=keypoints,
                    clusters=clusters_fixed.get("clusters", []),
                    anomalies=anomaly_report.get("anomalies", []),
                    output_dir=out_dir,
                    prefix=prefix,
                )
                for name, path in dual_viz_outputs.items():
                    print(f"   ✅ {name}: {path.name}")
            else:
                print("   ⚠️  Could not load images for dual visualization")
        except Exception as e:
            print(f"   ⚠️  Dual visualization error: {e}")

    # NEW: EXIF injection (via Atlas SDK)
    exif_output_path = None
    if args.inject_exif and raw_mode:
        print("\n💉 Injecting spots via Atlas SDK...")
        # Criar pasta edited/ centralizada no nível do site (diretório pai de out_dir)
        edited_dir = out_dir.parent / "edited"
        edited_dir.mkdir(parents=True, exist_ok=True)
        exif_output_path = edited_dir / f"{prefix}_with_spots.jpg"
        if inject_spots_to_exif(
            source_image, 
            exif_output_path, 
            keypoints,
            atlas_include=args.atlas_include,
            atlas_libdir=args.atlas_libdir,
            atlas_libs=args.atlas_libs,
        ):
            print(f"   ✅ ../edited/{exif_output_path.name}")
        else:
            print("   ⚠️  Atlas SDK injection failed")
            exif_output_path = None

    run_meta = {
        "mode": mode,
        "raw_mode": raw_mode,
        "source_image": str(source_image),
        "base_image": str(base_image_path),
        "keypoint_source": str(args.keypoint_source),
        "yolo_model": str(args.yolo_model) if args.yolo_model else None,
        "yolo_image": str(args.yolo_image) if args.yolo_image else None,
        "temperature_extraction": bool(args.extract_temperatures),
        "temperature_summary": temp_summary,
        "delta_threshold_c": float(args.delta_threshold),
        "openai_model": str(args.openai_model),
        "output_dir": str(out_dir),
        "output_prefix": prefix,
        "dual_viz": bool(dual_viz_outputs),
        "exif_injected": exif_output_path is not None,
        "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
        "version": "v5",
    }
    _write_json(out_dir / "run_metadata.json", run_meta)

    print("\n" + "=" * 70)
    print("✅ Done.")
    print("=" * 70)
    print(f" - Keypoints used:  {kp_out}")
    print(f" - LLM input:       {llm_img_out}")
    print(f" - Clusters JSON:   {clusters_out}")
    print(f" - Cluster overlay: {overlay_out}")
    print(f" - Anomalies JSON:  {anomalies_out}")
    print(f" - Side-by-side:    {side_by_side_out}")
    if dual_viz_outputs:
        print(f" - Thermal folder:  {out_dir / 'thermal'}")
        print(f" - RGB folder:      {out_dir / 'rgb'}")
        print(f" - Combined view:   {dual_viz_outputs.get('combined_view', 'N/A')}")
    if exif_output_path:
        print(f" - Edited folder:   {out_dir.parent / 'edited'}")
        print(f" - EXIF output:     ../edited/{exif_output_path.name}")
    print(f" - Metadata:        {out_dir / 'run_metadata.json'}")


if __name__ == "__main__":
    main()