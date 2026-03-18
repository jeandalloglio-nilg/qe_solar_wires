#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import json
import os
import random
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Tuple

import cv2


IMG_EXTS = {".jpg", ".jpeg", ".png", ".tif", ".tiff"}


@dataclass(frozen=True)
class Spot:
    x: float
    y: float


def _run_exiftool_json(image_path: Path) -> Dict[str, Any]:
    cmd = ["exiftool", "-j", "-G:1", "-s", "-n", str(image_path)]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        err = (res.stderr or "").strip()
        out = (res.stdout or "").strip()
        details = err if err else out
        if not details:
            details = f"(no output) returncode={res.returncode}"
        raise RuntimeError(f"exiftool failed for {image_path}: {details}")
    data = json.loads(res.stdout)
    return data[0] if data else {}


def _extract_spots_from_exif(exif_item: Dict[str, Any]) -> List[Spot]:
    spots: List[Spot] = []

    # FLIR spots are stored as:
    # - FLIR:Meas{n}Type == 1
    # - FLIR:Meas{n}Params == "x y" (pixel coordinates)
    n = 1
    while True:
        type_key = f"FLIR:Meas{n}Type"
        params_key = f"FLIR:Meas{n}Params"
        if type_key not in exif_item and params_key not in exif_item:
            break

        meas_type = exif_item.get(type_key, None)
        params = exif_item.get(params_key, None)
        if meas_type == 1 and params is not None:
            try:
                if isinstance(params, str):
                    parts = params.strip().split()
                    if len(parts) >= 2:
                        x = float(parts[0])
                        y = float(parts[1])
                        spots.append(Spot(x=x, y=y))
                elif isinstance(params, (list, tuple)) and len(params) >= 2:
                    x = float(params[0])
                    y = float(params[1])
                    spots.append(Spot(x=x, y=y))
            except Exception:
                pass

        n += 1

    return spots


def _maybe_rescale_spots_to_image_space(
    spots: List[Spot],
    exif_item: Dict[str, Any],
    image_w: int,
    image_h: int,
) -> List[Spot]:
    if not spots:
        return spots

    tw = exif_item.get("FLIR:RawThermalImageWidth", None)
    th = exif_item.get("FLIR:RawThermalImageHeight", None)
    try:
        tw_i = int(tw) if tw is not None else None
        th_i = int(th) if th is not None else None
    except Exception:
        tw_i, th_i = None, None

    if not tw_i or not th_i:
        return spots
    if tw_i == image_w and th_i == image_h:
        return spots
    if tw_i <= 0 or th_i <= 0 or image_w <= 0 or image_h <= 0:
        return spots

    sx = float(image_w) / float(tw_i)
    sy = float(image_h) / float(th_i)

    out: List[Spot] = []
    for s in spots:
        out.append(Spot(x=float(s.x) * sx, y=float(s.y) * sy))
    return out


def _get_image_size_wh(image_path: Path) -> Tuple[int, int]:
    img = cv2.imread(str(image_path), cv2.IMREAD_UNCHANGED)
    if img is None:
        raise FileNotFoundError(f"Could not read image: {image_path}")
    h, w = img.shape[:2]
    return int(w), int(h)


def _get_raw_thermal_wh_from_exif(exif_item: Dict[str, Any]) -> Optional[Tuple[int, int]]:
    tw = exif_item.get("FLIR:RawThermalImageWidth", None)
    th = exif_item.get("FLIR:RawThermalImageHeight", None)
    try:
        tw_i = int(tw) if tw is not None else None
        th_i = int(th) if th is not None else None
    except Exception:
        return None
    if not tw_i or not th_i:
        return None
    if tw_i <= 0 or th_i <= 0:
        return None
    return (tw_i, th_i)


def _spot_to_yolo_bbox(
    spot: Spot,
    w: int,
    h: int,
    bbox_size_px: int,
) -> Tuple[float, float, float, float]:
    half = bbox_size_px / 2.0

    x1 = max(0.0, spot.x - half)
    y1 = max(0.0, spot.y - half)
    x2 = min(float(w), spot.x + half)
    y2 = min(float(h), spot.y + half)

    bw = max(1e-6, x2 - x1)
    bh = max(1e-6, y2 - y1)
    cx = x1 + bw / 2.0
    cy = y1 + bh / 2.0

    return (cx / w, cy / h, bw / w, bh / h)


def _write_yolo_label_file(label_path: Path, boxes: Sequence[Tuple[float, float, float, float]], class_id: int = 0) -> None:
    label_path.parent.mkdir(parents=True, exist_ok=True)
    lines = [f"{class_id} {cx:.6f} {cy:.6f} {bw:.6f} {bh:.6f}" for (cx, cy, bw, bh) in boxes]
    label_path.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")


def _safe_copy(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists():
        return
    shutil.copy2(src, dst)


def _render_clean_ui(
    flir_image_path: Path,
    colormap: str = "inferno",
    clip_percent: float = 1.0,
) -> Tuple[Any, Tuple[int, int]]:
    try:
        from flirimageextractor import FlirImageExtractor
    except Exception as e:
        raise RuntimeError("flirimageextractor not available") from e

    import numpy as np

    cmap_map = {
        "jet": cv2.COLORMAP_JET,
        "turbo": cv2.COLORMAP_TURBO,
        "hot": cv2.COLORMAP_HOT,
        "magma": cv2.COLORMAP_MAGMA if hasattr(cv2, "COLORMAP_MAGMA") else cv2.COLORMAP_HOT,
        "inferno": cv2.COLORMAP_INFERNO if hasattr(cv2, "COLORMAP_INFERNO") else cv2.COLORMAP_HOT,
        "plasma": cv2.COLORMAP_PLASMA if hasattr(cv2, "COLORMAP_PLASMA") else cv2.COLORMAP_HOT,
    }

    fie = FlirImageExtractor(is_debug=False)
    fie.process_image(str(flir_image_path))
    thermal = fie.get_thermal_np()
    if thermal is None or getattr(thermal, "size", 0) == 0:
        raise RuntimeError("No radiometric thermal data available")

    th_h, th_w = thermal.shape[:2]
    th = thermal.astype(np.float32, copy=True)

    if clip_percent and clip_percent > 0:
        lo = float(np.percentile(th, clip_percent))
        hi = float(np.percentile(th, 100 - clip_percent))
    else:
        lo = float(np.nanmin(th))
        hi = float(np.nanmax(th))

    denom = max(1e-6, (hi - lo))
    th_norm = np.clip((th - lo) / denom, 0.0, 1.0)
    th_u8 = (th_norm * 255.0).astype(np.uint8)

    cmap = cmap_map.get(colormap.lower(), cv2.COLORMAP_INFERNO)
    bgr = cv2.applyColorMap(th_u8, cmap)
    return bgr, (int(th_w), int(th_h))


def _list_images(input_dir: Path) -> List[Path]:
    images: List[Path] = []
    for p in input_dir.rglob("*"):
        if p.is_file() and p.suffix.lower() in IMG_EXTS:
            images.append(p)
    images.sort()
    return images


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Prepare YOLO detect dataset from FLIR EXIF spots (MeasXParams).",
    )
    parser.add_argument("--input-images", required=True, help="Directory containing FLIR Edited images with EXIF spots")
    parser.add_argument("--output-dataset", required=True, help="Output dataset directory (will create images/ labels/ splits/)")
    parser.add_argument(
        "--render-clean-ui",
        action="store_true",
        help="If set, render a clean thermal image (no HUD) from radiometric data and use it as the training image (PNG) instead of copying the input image.",
    )
    parser.add_argument(
        "--render-colormap",
        default="inferno",
        choices=["jet", "turbo", "hot", "magma", "inferno", "plasma"],
        help="Colormap used when --render-clean-ui is set. Default=inferno",
    )
    parser.add_argument(
        "--render-clip-percent",
        type=float,
        default=1.0,
        help="Percentile clipping used when --render-clean-ui is set. Default=1.0",
    )
    parser.add_argument(
        "--skip-render-errors",
        action="store_true",
        help="If set, images that fail clean render will be skipped instead of aborting the run.",
    )
    parser.add_argument(
        "--only-thermal-320x240",
        action="store_true",
        help="If set, only keep images whose EXIF RawThermalImageWidth/Height is 320x240 (or 240x320). Others are skipped.",
    )
    parser.add_argument("--bbox-size", type=int, default=10, help="BBox size in pixels (square). Default=10")
    parser.add_argument("--val-ratio", type=float, default=0.2, help="Validation ratio. Default=0.2")
    parser.add_argument("--seed", type=int, default=42, help="Shuffle seed")
    parser.add_argument("--copy", action="store_true", help="Copy images into dataset (default).")
    parser.add_argument("--no-copy", action="store_true", help="Do not copy images; write splits pointing to original paths")
    parser.add_argument(
        "--skip-exiftool-errors",
        action="store_true",
        help="If set, images that make exiftool fail will be skipped instead of aborting the run.",
    )
    args = parser.parse_args()

    input_dir = Path(args.input_images)
    out_dir = Path(args.output_dataset)

    if not input_dir.exists():
        raise FileNotFoundError(f"Input directory not found: {input_dir}")

    # Ultralytics discovers label paths by replacing '/images/' with '/labels/'
    # so we must use this conventional folder name.
    images_dir = out_dir / "images"
    labels_dir = out_dir / "labels"
    splits_dir = out_dir / "splits"

    images = _list_images(input_dir)
    if not images:
        raise RuntimeError(f"No images found in: {input_dir}")

    use_copy = True
    if args.no_copy:
        use_copy = False

    manifest: List[Path] = []
    skipped_no_spots = 0
    skipped_exiftool_errors = 0
    skipped_render_errors = 0
    skipped_bad_thermal_size = 0

    for src_img in images:
        try:
            exif_item = _run_exiftool_json(src_img)
        except Exception as e:
            if bool(args.skip_exiftool_errors):
                skipped_exiftool_errors += 1
                continue
            raise
        spots = _extract_spots_from_exif(exif_item)
        if not spots:
            skipped_no_spots += 1
            continue

        if bool(args.only_thermal_320x240):
            thermal_wh = _get_raw_thermal_wh_from_exif(exif_item)
            if thermal_wh is None:
                skipped_bad_thermal_size += 1
                continue
            if thermal_wh not in {(320, 240), (240, 320)}:
                skipped_bad_thermal_size += 1
                continue

        dst_img: Optional[Path] = None
        if bool(args.render_clean_ui):
            dst_img = images_dir / f"{src_img.stem}_clean.png"
            if not dst_img.exists():
                try:
                    bgr, _ = _render_clean_ui(
                        flir_image_path=src_img,
                        colormap=str(args.render_colormap),
                        clip_percent=float(args.render_clip_percent),
                    )
                except Exception:
                    if bool(args.skip_render_errors):
                        skipped_render_errors += 1
                        continue
                    raise
                dst_img.parent.mkdir(parents=True, exist_ok=True)
                cv2.imwrite(str(dst_img), bgr)

            img_path_for_split = dst_img.resolve()
            label_stem = dst_img.stem
        else:
            if use_copy:
                dst_img = images_dir / src_img.name
                _safe_copy(src_img, dst_img)
                img_path_for_split = dst_img.resolve()
                label_stem = dst_img.stem
            else:
                img_path_for_split = src_img.resolve()
                label_stem = src_img.stem

        w, h = _get_image_size_wh(img_path_for_split)
        spots_img = _maybe_rescale_spots_to_image_space(spots, exif_item=exif_item, image_w=w, image_h=h)
        boxes = [_spot_to_yolo_bbox(s, w=w, h=h, bbox_size_px=int(args.bbox_size)) for s in spots_img]

        label_path = labels_dir / f"{label_stem}.txt"
        _write_yolo_label_file(label_path, boxes, class_id=0)

        manifest.append(img_path_for_split)

    if not manifest:
        raise RuntimeError("No images with EXIF spots were found; dataset would be empty.")

    # Write splits
    random.seed(int(args.seed))
    random.shuffle(manifest)

    val_ratio = float(args.val_ratio)
    val_ratio = min(max(val_ratio, 0.0), 0.95)
    n_val = max(1, int(round(len(manifest) * val_ratio))) if len(manifest) > 1 else 0

    val_list = manifest[:n_val]
    train_list = manifest[n_val:]
    if not train_list and val_list:
        train_list, val_list = val_list, []

    splits_dir.mkdir(parents=True, exist_ok=True)
    (splits_dir / "train.txt").write_text("\n".join(str(p) for p in train_list) + "\n", encoding="utf-8")
    (splits_dir / "val.txt").write_text("\n".join(str(p) for p in val_list) + ("\n" if val_list else ""), encoding="utf-8")

    # Minimal stats
    stats = {
        "input_dir": str(input_dir.resolve()),
        "output_dir": str(out_dir.resolve()),
        "total_images_found": len(images),
        "images_with_spots": len(manifest),
        "skipped_no_spots": skipped_no_spots,
        "skipped_exiftool_errors": skipped_exiftool_errors,
        "skipped_render_errors": skipped_render_errors,
        "skipped_bad_thermal_size": skipped_bad_thermal_size,
        "train_images": len(train_list),
        "val_images": len(val_list),
        "bbox_size_px": int(args.bbox_size),
        "val_ratio": float(args.val_ratio),
        "copied_images": bool(use_copy) and (not bool(args.render_clean_ui)),
        "render_clean_ui": bool(args.render_clean_ui),
        "render_colormap": str(args.render_colormap),
        "render_clip_percent": float(args.render_clip_percent),
        "only_thermal_320x240": bool(args.only_thermal_320x240),
    }
    (out_dir / "prepare_stats.json").write_text(json.dumps(stats, indent=2), encoding="utf-8")

    print(json.dumps(stats, indent=2))


if __name__ == "__main__":
    main()
