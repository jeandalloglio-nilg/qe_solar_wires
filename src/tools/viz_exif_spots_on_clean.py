#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import json
import os
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Tuple

import cv2


@dataclass(frozen=True)
class Spot:
    x: float
    y: float


@dataclass(frozen=True)
class YoloBox:
    cls: int
    cx: float
    cy: float
    w: float
    h: float


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
                        spots.append(Spot(x=float(parts[0]), y=float(parts[1])))
                elif isinstance(params, (list, tuple)) and len(params) >= 2:
                    spots.append(Spot(x=float(params[0]), y=float(params[1])))
            except Exception:
                pass

        n += 1

    return spots


def _get_image_size_wh(image_path: Path) -> Tuple[int, int]:
    img = cv2.imread(str(image_path), cv2.IMREAD_UNCHANGED)
    if img is None:
        raise FileNotFoundError(f"Could not read image: {image_path}")
    h, w = img.shape[:2]
    return int(w), int(h)


def _read_yolo_labels_txt(label_path: Path) -> List[YoloBox]:
    if not label_path.exists():
        raise FileNotFoundError(f"Label file not found: {label_path}")
    out: List[YoloBox] = []
    for line in label_path.read_text(encoding="utf-8").splitlines():
        s = line.strip()
        if not s:
            continue
        parts = s.split()
        if len(parts) < 5:
            continue
        try:
            cls = int(float(parts[0]))
            cx = float(parts[1])
            cy = float(parts[2])
            w = float(parts[3])
            h = float(parts[4])
        except Exception:
            continue
        out.append(YoloBox(cls=cls, cx=cx, cy=cy, w=w, h=h))
    return out


def _maybe_rescale_spots_to_target_image(
    spots: List[Spot],
    exif_item: Dict[str, Any],
    target_w: int,
    target_h: int,
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
        raise RuntimeError("Missing FLIR:RawThermalImageWidth/Height in EXIF")

    if tw_i <= 0 or th_i <= 0:
        raise RuntimeError(f"Invalid thermal size in EXIF: {tw_i}x{th_i}")

    sx = float(target_w) / float(tw_i)
    sy = float(target_h) / float(th_i)

    out: List[Spot] = []
    for s in spots:
        out.append(Spot(x=float(s.x) * sx, y=float(s.y) * sy))
    return out


def _try_render_clean_from_radiometric(
    flir_image_path: Path,
    colormap: str = "inferno",
    clip_percent: float = 1.0,
) -> Tuple[cv2.typing.MatLike, Tuple[int, int]]:
    """Attempt to render a clean thermal visualization (no HUD/UI) from radiometric data.

    Returns:
        (bgr_image, thermal_wh)

    Raises:
        RuntimeError if radiometric extraction isn't available.
    """

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


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Draw FLIR EXIF spots (MeasXParams) as bboxes on top of a *_clean.png image.",
    )
    parser.add_argument(
        "--mode",
        default="exif",
        choices=["exif", "yolo"],
        help="Visualization mode. 'exif' draws FLIR EXIF spots, 'yolo' draws YOLO labels. Default=exif",
    )
    parser.add_argument("--clean-image", default=None, help="Path to *_clean.png (thermal_clean). If omitted, use --image.")
    parser.add_argument("--image", default=None, help="Alias for --clean-image")
    parser.add_argument(
        "--edited-image",
        default=None,
        help="Path to original Edited JPG for side-by-side comparison when using --show.",
    )
    parser.add_argument(
        "--exif-source",
        required=False,
        help="Path to RAW/Edited image that contains FLIR:MeasXParams in EXIF (spots)",
    )
    parser.add_argument(
        "--yolo-label",
        default=None,
        help="Path to YOLO label .txt file (class cx cy w h normalized). Used when --mode yolo.",
    )
    parser.add_argument("--bbox-size", type=int, default=10, help="BBox size in pixels (square). Default=10")
    parser.add_argument("--output", default=None, help="Output PNG path. Default: next to clean image")
    parser.add_argument("--draw-ids", action="store_true", help="Draw spot indices next to each bbox")
    parser.add_argument("--show", action="store_true", help="Show the visualization in an OpenCV window")
    parser.add_argument("--no-save", action="store_true", help="Do not write output image to disk")
    parser.add_argument(
        "--tmp-output",
        default=None,
        help="When running headless with --show, write a temporary PNG here (or under /tmp).",
    )
    parser.add_argument(
        "--render-clean-ui",
        action="store_true",
        help="If set, ignore --clean-image and render a clean thermal image in-memory from --exif-source radiometric data.",
    )
    parser.add_argument(
        "--colormap",
        default="inferno",
        choices=["jet", "turbo", "hot", "magma", "inferno", "plasma"],
        help="Colormap used when --render-clean-ui is set. Default=inferno",
    )
    parser.add_argument(
        "--clip-percent",
        type=float,
        default=1.0,
        help="Percentile clipping for clean render when --render-clean-ui is set. Default=1.0",
    )
    args = parser.parse_args()

    clean_image_arg = args.clean_image or args.image
    exif_source = Path(args.exif_source) if args.exif_source else None
    if args.mode == "exif":
        if exif_source is None:
            raise SystemExit("--exif-source is required when --mode exif")
        if not exif_source.exists():
            raise FileNotFoundError(f"EXIF source not found: {exif_source}")

    clean_image = None
    img = None
    clean_wh = None
    if bool(args.render_clean_ui):
        if args.mode != "exif":
            raise SystemExit("--render-clean-ui is only supported with --mode exif")
        try:
            img, clean_wh = _try_render_clean_from_radiometric(
                flir_image_path=exif_source,
                colormap=str(args.colormap),
                clip_percent=float(args.clip_percent),
            )
            clean_image = exif_source
        except Exception as e:
            print(f"⚠️  Could not render clean UI from radiometric data: {e}")
            print("   Fallback: drawing on top of the input image pixels")

    if img is None:
        if not clean_image_arg:
            raise SystemExit("You must provide --clean-image/--image, or set --render-clean-ui")
        clean_image = Path(clean_image_arg)
        if not clean_image.exists():
            raise FileNotFoundError(f"Image not found: {clean_image}")
        img = cv2.imread(str(clean_image), cv2.IMREAD_COLOR)
        if img is None:
            raise RuntimeError(f"Could not read image: {clean_image}")
        h, w = img.shape[:2]
        clean_wh = (int(w), int(h))

    out_path = Path(args.output) if args.output else Path(clean_image).with_name(
        f"{Path(clean_image).stem}_exif_spots_bbox{int(args.bbox_size)}.png"
    )

    w, h = clean_wh

    if args.mode == "exif":
        exif = _run_exiftool_json(exif_source)
        spots = _extract_spots_from_exif(exif)
        spots_img = _maybe_rescale_spots_to_target_image(spots, exif_item=exif, target_w=w, target_h=h)
        half = float(args.bbox_size) / 2.0

        for idx, s in enumerate(spots_img, start=1):
            x = float(s.x)
            y = float(s.y)
            x1 = int(max(0.0, x - half))
            y1 = int(max(0.0, y - half))
            x2 = int(min(float(w - 1), x + half))
            y2 = int(min(float(h - 1), y + half))

            cv2.rectangle(img, (x1, y1), (x2, y2), (0, 255, 255), 2)
            cv2.circle(img, (int(round(x)), int(round(y))), 2, (0, 0, 255), -1)
            if bool(args.draw_ids):
                cv2.putText(
                    img,
                    str(idx),
                    (min(w - 1, x2 + 2), min(h - 1, y2 + 2)),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.5,
                    (255, 255, 255),
                    1,
                    cv2.LINE_AA,
                )

    if args.mode == "yolo":
        if not args.yolo_label:
            raise SystemExit("--yolo-label is required when --mode yolo")
        label_path = Path(args.yolo_label)
        boxes = _read_yolo_labels_txt(label_path)

        for idx, b in enumerate(boxes, start=1):
            x1 = int(max(0.0, (b.cx - b.w / 2.0) * float(w)))
            y1 = int(max(0.0, (b.cy - b.h / 2.0) * float(h)))
            x2 = int(min(float(w - 1), (b.cx + b.w / 2.0) * float(w)))
            y2 = int(min(float(h - 1), (b.cy + b.h / 2.0) * float(h)))

            cx_px = int(round(b.cx * float(w)))
            cy_px = int(round(b.cy * float(h)))
            cv2.rectangle(img, (x1, y1), (x2, y2), (0, 255, 255), 2)
            cv2.circle(img, (cx_px, cy_px), 2, (0, 0, 255), -1)
            if bool(args.draw_ids):
                cv2.putText(
                    img,
                    f"{idx}:{b.cls}",
                    (min(w - 1, x2 + 2), min(h - 1, y2 + 2)),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.5,
                    (255, 255, 255),
                    1,
                    cv2.LINE_AA,
                )

    if bool(args.show):
        has_display = bool(os.environ.get("DISPLAY"))
        edited = None
        if args.edited_image:
            edited_path = Path(args.edited_image)
            if not edited_path.exists():
                raise FileNotFoundError(f"Edited image not found: {edited_path}")
            edited = cv2.imread(str(edited_path), cv2.IMREAD_COLOR)
            if edited is None:
                raise RuntimeError(f"Could not read edited image: {edited_path}")

        if edited is not None:
            eh, ew = edited.shape[:2]
            ch, cw = img.shape[:2]
            if eh != ch:
                scale = float(ch) / float(max(1, eh))
                new_w = int(round(float(ew) * scale))
                edited = cv2.resize(edited, (max(1, new_w), int(ch)), interpolation=cv2.INTER_AREA)
            combined = cv2.hconcat([edited, img])
        else:
            combined = img

        if has_display:
            title = "viz"
            cv2.imshow(title, combined)
            cv2.waitKey(0)
            cv2.destroyAllWindows()
        else:
            tmp_path = Path(args.tmp_output) if args.tmp_output else Path("/tmp") / f"viz_compare_{os.getpid()}.png"
            tmp_path.parent.mkdir(parents=True, exist_ok=True)
            cv2.imwrite(str(tmp_path), combined)
            print(f"headless_show_written: {tmp_path}")

    if not bool(args.no_save):
        out_path.parent.mkdir(parents=True, exist_ok=True)
        cv2.imwrite(str(out_path), img)

    print(f"image_used_for_overlay: {clean_image}")
    print(f"mode: {args.mode}")
    print(f"clean_wh: {w}x{h}")
    if args.mode == "exif":
        tw = exif.get("FLIR:RawThermalImageWidth")
        th = exif.get("FLIR:RawThermalImageHeight")
        print(f"exif_source: {exif_source}")
        print(f"thermal_wh (EXIF): {tw}x{th}")
        print(f"spots_found: {len(spots)}")
    if args.mode == "yolo":
        print(f"yolo_label: {args.yolo_label}")
        print(f"boxes_found: {len(boxes)}")
    if not bool(args.no_save):
        print(f"wrote: {out_path}")


if __name__ == "__main__":
    main()
