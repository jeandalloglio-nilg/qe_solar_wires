#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
WP2 — Group-wise scale tuning (Linux + Atlas C SDK)

Pipeline:
1) Group by EXIF ImageDescription -> extract equipment TYPE
2) Copy originals preserving metadata/timestamps to the output folder
3) Extract ALL temperatures (°C) from radiometric images in the group and compute P5–P95
4) Apply scale [P5, P95] and palette via atlas_sdk/build/atlas_scale_set (radiometric files only)
5) Organize output under out/<group_id>/ with FLAT structure (no subfolders) and generate reports

IMPORTANT:
- Grouping is done by EQUIPMENT TYPE (e.g., CB / INV / XMFR), not by individual equipment instance.
- The grouping key is inferred from ImageDescription, then normalized + aliased to match WP1 naming.
- Non-radiometric images are copied/moved but NOT tuned (they are listed in reports; strict mode can fail).
- Output structure is FLAT: all images go directly into their group folder (no nested subfolders).
- Empty directories are cleaned up at the end.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import re
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple, Any, Set

import numpy as np
from PIL import Image, ExifTags
from tqdm import tqdm

# --- optional dependencies ---
try:
    import piexif  # EXIF-only fallback if exiftool is unavailable
except Exception:
    piexif = None

try:
    from flirimageextractor import FlirImageExtractor
except Exception:
    FlirImageExtractor = None


SUPPORTED_EXT = {".jpg", ".jpeg", ".png", ".tif", ".tiff"}
EXIF_TAGS = {v: k for k, v in ExifTags.TAGS.items()}
INVALID_FS = r'<>:"/\|?*'


@dataclass
class ArgsNS:
    input: Path
    output: Path
    atlas_sdk_dir: Path
    palette: str
    p_low: float
    p_high: float
    hist_clip: float
    strict: bool


def safe_dirname(name: str) -> str:
    s = (name or "UNKNOWN").strip()
    for ch in INVALID_FS:
        s = s.replace(ch, "_")
    return s or "UNKNOWN"


def has_exiftool() -> bool:
    return shutil.which("exiftool") is not None


def run(cmd: List[str], check: bool = True) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, check=check, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)


def discover_images(root: Path) -> List[Path]:
    return [p for p in root.rglob("*") if p.is_file() and p.suffix.lower() in SUPPORTED_EXT]


def get_imagedescription(p: Path) -> str:
    try:
        with Image.open(p) as im:
            exif = im.getexif() or {}
            tag = EXIF_TAGS.get("ImageDescription", -1)
            v = exif.get(tag)
            if isinstance(v, bytes):
                v = v.decode("utf-8", "ignore")
            if isinstance(v, str) and v.strip():
                return v.strip().replace("/", "_").replace("\\", "_")
    except Exception:
        pass
    return "UNKNOWN"


def normalize_group_id(desc: str) -> str:
    """
    Extract an equipment TYPE from ImageDescription.

    1) Grab initial alphabetic prefix at start of string.
       Examples: "CB-02" -> "CB", "INVERTER-3" -> "INVERTER", "XMFR01" -> "XMFR"
    2) Apply aliases to match WP1 naming (INVERTER -> INV, TRANSFORMER -> XMFR)
    3) Return "UNKNOWN" if no alphabetic prefix found.
    """
    if not desc:
        return "UNKNOWN"

    s = desc.strip()
    m = re.match(r"^([A-Za-z]+)", s)
    if not m:
        return "UNKNOWN"

    prefix = (m.group(1) or "").upper().strip()
    if not prefix:
        return "UNKNOWN"

    aliases = {
        "INVERTER": "INV",
        "INV": "INV",
        "TRANSFORMER": "XMFR",
        "XFMR": "XMFR",
        "XMFR": "XMFR",
        "CB": "CB",
        "BREAKER": "CB",
    }
    return aliases.get(prefix, prefix)


def copy_preserve(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def copy_all_metadata(src: Path, dst: Path) -> None:
    if not has_exiftool():
        raise RuntimeError("exiftool not found. Install it, e.g.: sudo apt-get install exiftool")

    cmd = [
        "exiftool",
        "-overwrite_original",
        "-P",
        "-TagsFromFile", str(src),
        "-all:all",
        str(dst),
    ]
    cp = run(cmd, check=False)
    if cp.returncode != 0:
        raise RuntimeError(f"exiftool failed copying metadata:\n{cp.stdout}")


def copy_basic_exif_python(src: Path, dst: Path) -> None:
    if piexif is None:
        print("[warn] piexif not installed and exiftool unavailable; metadata may be lost.", file=sys.stderr)
        return
    try:
        src_exif = piexif.load(str(src))
        piexif.insert(piexif.dump(src_exif), str(dst))
    except Exception as e:
        print(f"[warn] fallback EXIF-only failed ({e}); metadata may differ.", file=sys.stderr)


def thermal_celsius(p: Path) -> np.ndarray:
    if FlirImageExtractor is None:
        raise RuntimeError("flirimageextractor not installed. Install with: pip install flirimageextractor")
    try:
        fie = FlirImageExtractor(is_debug=False)
    except TypeError:
        fie = FlirImageExtractor()
    fie.process_image(str(p))
    arr = fie.get_thermal_np()
    if arr is None:
        raise RuntimeError(f"not radiometric: {p.name}")
    return np.asarray(arr, dtype=np.float32)


def robust_window_group(files: List[Path], p_low: float, p_high: float, hist_clip: float) -> Tuple[float, float, Set[Path]]:
    flat_vals: List[np.ndarray] = []
    radiometric_files: Set[Path] = set()

    for fp in files:
        try:
            a = thermal_celsius(fp).reshape(-1)
            a = a[np.isfinite(a)]
            if a.size:
                flat_vals.append(a)
                radiometric_files.add(fp)
        except Exception:
            continue

    if not flat_vals:
        raise RuntimeError("No valid radiometric image in group (cannot compute window).")

    big = np.concatenate(flat_vals, axis=0)

    if hist_clip > 0:
        lo = np.percentile(big, hist_clip)
        hi = np.percentile(big, 100.0 - hist_clip)
        big = big[(big >= lo) & (big <= hi)]

    tmin = float(np.percentile(big, p_low))
    tmax = float(np.percentile(big, p_high))

    if not np.isfinite(tmin) or not np.isfinite(tmax) or tmax <= tmin or (tmax - tmin) < 2.0:
        med = float(np.median(big))
        tmin, tmax = med - 1.0, med + 1.0

    return tmin, tmax, radiometric_files


def atlas_cli_path(atlas_sdk_dir: Path) -> Path:
    cli = atlas_sdk_dir / "build" / "atlas_scale_set"
    if not cli.exists():
        raise RuntimeError(
            f"atlas_scale_set not found at: {cli}\n"
            f"Build the atlas_sdk project (example): make -C {atlas_sdk_dir} ..."
        )
    return cli


def apply_scale_with_cli(cli: Path, img: Path, tmin: float, tmax: float, palette: str) -> None:
    tmp_path = img.with_suffix(img.suffix + ".atlas_tmp")
    if tmp_path.exists():
        tmp_path.unlink()

    cp = run([str(cli), str(img), f"{tmin}", f"{tmax}", palette, str(tmp_path)], check=False)
    if cp.returncode != 0:
        raise RuntimeError(f"atlas_scale_set failed for {img.name} (rc={cp.returncode}):\n{(cp.stdout or '').strip()}")

    if has_exiftool():
        copy_all_metadata(img, tmp_path)
    else:
        copy_basic_exif_python(img, tmp_path)

    orig_stat = img.stat()
    tmp_path.replace(img)
    os.utime(img, (orig_stat.st_atime, orig_stat.st_mtime))


def unique_dest(folder: Path, filename: str) -> Path:
    """Generate a unique destination path, handling name collisions."""
    base = Path(filename).stem
    ext = Path(filename).suffix
    cand = folder / (base + ext)
    if not cand.exists():
        return cand
    for i in range(1, 10000):
        cand = folder / f"{base}_{i:03d}{ext}"
        if not cand.exists():
            return cand
    raise RuntimeError(f"Too many name collisions for {filename} in {folder}")


def cleanup_empty_dirs(root: Path) -> int:
    """Remove empty directories recursively. Returns count of removed dirs."""
    removed = 0
    for d in sorted([p for p in root.rglob("*") if p.is_dir()], reverse=True):
        try:
            if d.exists() and not any(d.iterdir()):
                d.rmdir()
                removed += 1
        except Exception:
            pass
    return removed


def main() -> None:
    ap = argparse.ArgumentParser(description="Group-wise P5–P95 scale tuning (Atlas C SDK).")
    ap.add_argument("--input", required=True, type=Path, help="Directory with images.")
    ap.add_argument("--output", required=True, type=Path, help="Output directory.")
    ap.add_argument("--atlas-sdk-dir", required=True, type=Path, help="C project directory (atlas_sdk).")
    ap.add_argument("--palette", default="iron", help="Palette (iron, rainbow, bw, whitehot, blackhot...).")
    ap.add_argument("--p-low", type=float, default=5.0, help="Lower percentile for group (°C). Default: 5.")
    ap.add_argument("--p-high", type=float, default=95.0, help="Upper percentile for group (°C). Default: 95.")
    ap.add_argument("--hist-clip", type=float, default=0.0,
                    help="Symmetric histogram tail clip (%%) before computing percentiles. 0 disables (default).")
    ap.add_argument("--strict", action="store_true",
                    help="Fail (non-zero exit) if any group is skipped or any image fails (or is non-radiometric).")
    ns = ArgsNS(**vars(ap.parse_args()))

    if not ns.input.is_dir():
        print(f"Error: --input does not exist: {ns.input}", file=sys.stderr)
        sys.exit(1)
    ns.output.mkdir(parents=True, exist_ok=True)

    cli = atlas_cli_path(ns.atlas_sdk_dir)

    files_in = discover_images(ns.input)
    if not files_in:
        print("No supported images found.", file=sys.stderr)
        sys.exit(1)

    # Copy originals preserving relative structure temporarily
    files_out: List[Path] = []
    for src in tqdm(files_in, desc="Copying originals"):
        rel = src.relative_to(ns.input)
        dst = ns.output / rel
        copy_preserve(src, dst)
        files_out.append(dst)

    # Group by equipment TYPE
    groups: Dict[str, List[Path]] = {}
    original_descriptions: Dict[Path, str] = {}
    for p in files_out:
        raw_gid = get_imagedescription(p)
        original_descriptions[p] = raw_gid
        gid = normalize_group_id(raw_gid)
        groups.setdefault(gid, []).append(p)

    print(f"Found {len(groups)} equipment-type groups: {sorted(groups.keys())}")

    group_report: Dict[str, Dict[str, Any]] = {}
    per_image_rows: List[Dict[str, Any]] = []
    failed_images: List[Dict[str, Any]] = []
    skipped_groups: List[Dict[str, Any]] = []
    non_radiometric_images: List[Dict[str, Any]] = []

    for gid, gfiles in groups.items():
        print(f"\n[Group] {gid} — {len(gfiles)} files")

        try:
            tmin_c, tmax_c, radiometric_set = robust_window_group(gfiles, ns.p_low, ns.p_high, ns.hist_clip)
        except Exception as e:
            msg = str(e)
            print(f"[ERROR] Could not compute group window '{gid}': {msg}", file=sys.stderr)
            skipped_groups.append({"group_id": gid, "count": len(gfiles), "reason": msg})
            continue

        print(f"  Window (°C): Tmin={tmin_c:.3f}  Tmax={tmax_c:.3f}  (radiometric={len(radiometric_set)}/{len(gfiles)})")

        # Apply scale/palette to radiometric images
        for img in tqdm(gfiles, desc=f"Applying scale/palette: {gid}"):
            is_radiometric = img in radiometric_set

            if not is_radiometric:
                non_radiometric_images.append({
                    "group_id": gid,
                    "original_description": original_descriptions.get(img, "UNKNOWN"),
                    "file": str(img),
                    "reason": "non-radiometric (skipped tuning)",
                })
            else:
                try:
                    apply_scale_with_cli(cli, img, tmin_c, tmax_c, ns.palette)
                except Exception as e:
                    msg = str(e)
                    print(f"[atlas] error on {img.name}: {msg}", file=sys.stderr)
                    failed_images.append({
                        "group_id": gid,
                        "original_description": original_descriptions.get(img, "UNKNOWN"),
                        "file": str(img),
                        "reason": msg,
                    })

            per_image_rows.append({
                "group_id": gid,
                "original_description": original_descriptions.get(img, "UNKNOWN"),
                "file": str(img),
                "tmin_c": tmin_c,
                "tmax_c": tmax_c,
                "palette": ns.palette,
                "tuned": bool(is_radiometric),
            })

        # Organize: FLAT per group folder (no subfolders)
        # Output: out/<GROUP>/<filename>.jpg
        target_root = ns.output / safe_dirname(gid)
        target_root.mkdir(parents=True, exist_ok=True)

        # Move images into the group folder (flat)
        moved_map: Dict[str, str] = {}  # original path -> new path
        for img in list(gfiles):
            dest = unique_dest(target_root, img.name)
            try:
                if img.resolve() != dest.resolve():
                    shutil.move(str(img), str(dest))
                moved_map[str(img)] = str(dest)
            except Exception as e:
                print(f"[warn] could not move {img.name} -> {dest}: {e}", file=sys.stderr)
                moved_map[str(img)] = str(img)  # keep original path in report

        # Build report file list from moved_map
        fixed_files: List[str] = []
        for p in gfiles:
            fixed_files.append(moved_map.get(str(p), str(target_root / p.name)))

        group_report[gid] = {
            "count": len(gfiles),
            "radiometric_count": len(radiometric_set),
            "p_low": ns.p_low,
            "p_high": ns.p_high,
            "hist_clip": ns.hist_clip,
            "tmin_c": tmin_c,
            "tmax_c": tmax_c,
            "palette": ns.palette,
            "files": fixed_files,
        }

        # Update per-image rows paths
        for row in per_image_rows:
            if row["group_id"] == gid and row["file"] in moved_map:
                row["file"] = moved_map[row["file"]]

    # Cleanup empty directories left behind after moving files
    removed_dirs = cleanup_empty_dirs(ns.output)
    if removed_dirs > 0:
        print(f"\n🧹 Cleaned up {removed_dirs} empty directories")

    # Export reports
    (ns.output / "summary.json").write_text(json.dumps(group_report, indent=2, ensure_ascii=False), encoding="utf-8")
    (ns.output / "failed_images.json").write_text(json.dumps(failed_images, indent=2, ensure_ascii=False), encoding="utf-8")
    (ns.output / "skipped_groups.json").write_text(json.dumps(skipped_groups, indent=2, ensure_ascii=False), encoding="utf-8")
    (ns.output / "non_radiometric_images.json").write_text(json.dumps(non_radiometric_images, indent=2, ensure_ascii=False), encoding="utf-8")

    with open(ns.output / "group_summary.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["group_id", "count", "radiometric_count", "p_low", "p_high", "hist_clip", "tmin_c", "tmax_c", "palette"])
        for gid, info in group_report.items():
            w.writerow([
                gid, info["count"], info.get("radiometric_count", 0),
                info["p_low"], info["p_high"], info["hist_clip"],
                f"{info['tmin_c']:.6f}", f"{info['tmax_c']:.6f}", info["palette"],
            ])

    with open(ns.output / "per_image.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["group_id", "original_description", "file", "tmin_c", "tmax_c", "palette", "tuned"])
        for r in per_image_rows:
            w.writerow([
                r["group_id"], r["original_description"], r["file"],
                f"{r['tmin_c']:.6f}", f"{r['tmax_c']:.6f}", r["palette"], int(bool(r["tuned"])),
            ])

    print("\n" + "=" * 50)
    print("✅ Completed.")
    print("=" * 50)
    print(f"Groups processed: {len(group_report)}")
    print(f"Skipped groups: {len(skipped_groups)}")
    print(f"Failed images: {len(failed_images)}")
    print(f"Non-radiometric images (skipped tuning): {len(non_radiometric_images)}")

    if ns.strict and (skipped_groups or failed_images or non_radiometric_images):
        print(
            "\n[STRICT] Failing because there were skipped groups and/or failed/non-radiometric images. "
            "See JSON reports in output folder.",
            file=sys.stderr,
        )
        sys.exit(2)


if __name__ == "__main__":
    start_ts = time.strftime("%Y-%m-%d %H:%M:%S")
    t1 = time.perf_counter()
    main()
    t2 = time.perf_counter()
    end_ts = time.strftime("%Y-%m-%d %H:%M:%S")
    delta = t2 - t1
    print(f"\nStart: {start_ts}  |  End: {end_ts}")
    print(f"Total runtime: {delta:.2f} s ({delta/60:.2f} min)")