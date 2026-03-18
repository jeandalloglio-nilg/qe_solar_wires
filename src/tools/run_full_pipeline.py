#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
QE Solar - Full Pipeline (RAW-Only Mode)
=========================================

Pipeline para processamento de inspeções térmicas usando APENAS imagens RAW.

Fluxo:
  0. Extrair RGB das imagens RAW
  1. Limpar UI das RAW → thermal_clean (para YOLO)
  2. WP1: Sorting por EXIF ImageDescription
  3. WP2: Thermal Range Tuning (P5-P95)
  4. WP4+WP5: YOLO keypoints + Clustering + Anomalias
     - Detecção de keypoints na imagem TÉRMICA
     - Mapeamento de coords para RGB
     - Clustering na RGB
     - Visualizações em AMBAS (thermal + RGB)
     - Injeção de spots no EXIF (usando imagens TUNED)
  5. WP6: Relatório REPX

IMPORTANTE: WP4+WP5 agora usa as imagens do thermal tuning (04_wp2_thermal_tuned)
para que os spots injetados tenham o mesmo scale range visível no FLIR Tools.

Uso:
    python -m src.tools.run_full_pipeline \\
        --site-folder "data/site/2025" \\
        --output "output/site_2025" \\
        --yolo-model "models/best.pt" \\
        --yolo-conf 0.15

Author: QE Solar / Elton Gino
"""

from __future__ import annotations
import argparse, csv, json, os, re, shutil, subprocess, sys, time
from dataclasses import dataclass, field, asdict
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

try:
    import cv2
    import numpy as np
    HAS_CV2 = True
except ImportError:
    HAS_CV2 = False

try:
    import flirimageextractor
    HAS_FLIR = True
except ImportError:
    HAS_FLIR = False


# =============================================================================
# Data Classes
# =============================================================================

@dataclass
class PipelineConfig:
    """Configuração do pipeline."""
    site_folder: Path
    output_folder: Path
    
    # Flags de controle
    skip_clean: bool = False
    skip_wp1: bool = False
    skip_wp2: bool = False
    skip_wp4_wp5: bool = False
    skip_wp6: bool = False
    
    # Keypoint source (YOLO é o padrão para RAW-only)
    keypoint_source: str = "yolo"
    yolo_model: Path = None
    yolo_conf: float = 0.30
    
    # WP1 config
    wp1_model: str = "gpt-4o-mini"
    
    # WP2 config
    wp2_palette: str = "iron"
    atlas_sdk_dir: Optional[Path] = None
    
    # WP4+WP5 config
    openai_model: str = "gpt-4o"
    delta_threshold: float = 3.0
    kernel_size: int = 3
    
    # Alignment (thermal → RGB) - manual override
    real2ir: Optional[float] = None
    offset_x: Optional[float] = None
    offset_y: Optional[float] = None
    scale_to_rgb: bool = True
    clip_outside: bool = True
    
    # Subpastas customizáveis
    raw_subdir: str = None
    
    # Paths auxiliares
    raw_images_dir: Path = field(default=None)
    
    def __post_init__(self):
        self.site_folder = Path(self.site_folder)
        self.output_folder = Path(self.output_folder)
        
        # Detectar pasta RAW
        self.raw_images_dir = self._find_raw_dir()
        
        # YOLO model default
        if self.yolo_model is None:
            possible = [Path("models/best.pt"), Path("/home/elton/qe_solar_wires/models/best.pt")]
            for p in possible:
                if p.exists():
                    self.yolo_model = p
                    break
        elif self.yolo_model:
            self.yolo_model = Path(self.yolo_model)
        
        # Atlas SDK
        if self.atlas_sdk_dir is None:
            possible = [Path("atlas_sdk"), Path("/home/elton/qe_solar_wires/atlas_sdk")]
            for p in possible:
                if p.exists() and (p / "build" / "atlas_scale_set").exists():
                    self.atlas_sdk_dir = p
                    break
        elif self.atlas_sdk_dir:
            self.atlas_sdk_dir = Path(self.atlas_sdk_dir)
    
    def _find_raw_dir(self) -> Path:
        """Encontra pasta de imagens RAW."""
        if self.raw_subdir:
            return self.site_folder / self.raw_subdir
        
        candidates = ["Raw Images", "Raw", "RAW", "raw_images", "raw"]
        for name in candidates:
            path = self.site_folder / name
            if path.exists() and path.is_dir():
                return path
        return self.site_folder / "Raw Images"


@dataclass
class ImageResult:
    filename: str
    sorted_name: str = ""
    status: str = "pending"
    temp_min: Optional[float] = None
    temp_max: Optional[float] = None
    keypoint_count: int = 0
    cluster_count: int = 0
    anomaly_count: int = 0
    anomalies: List[Dict] = field(default_factory=list)
    error_message: Optional[str] = None


@dataclass
class PipelineResult:
    site_name: str
    start_time: str
    end_time: str = ""
    duration_seconds: float = 0.0
    total_images: int = 0
    processed_images: int = 0
    failed_images: int = 0
    total_anomalies: int = 0
    high_severity: int = 0
    medium_severity: int = 0
    low_severity: int = 0
    image_results: List[ImageResult] = field(default_factory=list)


# =============================================================================
# Utilities
# =============================================================================

IMG_EXTS = {".jpg", ".jpeg", ".png", ".tif", ".tiff"}

def list_images(directory: Path) -> List[Path]:
    if not directory.exists():
        return []
    return sorted([p for p in directory.iterdir() if p.is_file() and p.suffix.lower() in IMG_EXTS])

def list_images_recursive(directory: Path) -> List[Path]:
    if not directory.exists():
        return []
    return sorted([p for p in directory.rglob("*") if p.is_file() and p.suffix.lower() in IMG_EXTS])

def extract_rgb_from_flir(flir_path: Path, output_path: Path) -> bool:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    if HAS_FLIR:
        try:
            flir = flirimageextractor.FlirImageExtractor(palettes=[])
            flir.process_image(str(flir_path))
            rgb = flir.get_embedded_image()
            if rgb is not None:
                from PIL import Image
                Image.fromarray(rgb).save(str(output_path), quality=95)
                return True
        except: pass
    try:
        cmd = ["exiftool", "-b", "-EmbeddedImage", str(flir_path)]
        result = subprocess.run(cmd, capture_output=True)
        if result.returncode == 0 and result.stdout:
            output_path.write_bytes(result.stdout)
            return True
    except: pass
    return False


def sanitize_filename(name: str) -> str:
    """Remove caracteres inválidos para nomes de arquivo/pasta."""
    name = name.replace("/", "_").replace("\\", "_").replace(":", "_").replace(";", "_")
    name = name.replace("\x00", "").replace("\0", "")
    name = "".join(c for c in name if c.isprintable())
    return name.strip() or "UNKNOWN"


def get_exif_description(img_path: Path) -> str:
    """Extrai ImageDescription do EXIF."""
    try:
        from PIL import Image
        with Image.open(img_path) as im:
            exif = im.getexif() or {}
            desc = str(exif.get(270, "UNKNOWN")).strip() or "UNKNOWN"
            return sanitize_filename(desc)
    except:
        return "UNKNOWN"


def find_tuned_image_for_raw(raw_stem: str, tuned_dir: Path, name_mapping: Dict[str, str]) -> Optional[Path]:
    """
    Encontra a imagem tuned correspondente a uma RAW original.
    
    Args:
        raw_stem: Nome base da imagem RAW (ex: "IR_54393")
        tuned_dir: Pasta 04_wp2_thermal_tuned/
        name_mapping: Mapeamento original_stem → sorted_name
    
    Returns:
        Path da imagem tuned ou None se não encontrada
    """
    sorted_name = name_mapping.get(raw_stem, raw_stem)
    
    # Buscar recursivamente na pasta tuned
    for img_path in tuned_dir.rglob("*"):
        if not img_path.is_file():
            continue
        if img_path.suffix.lower() not in IMG_EXTS:
            continue
        
        # Verificar se o nome da imagem contém o stem original ou o sorted_name
        if raw_stem in img_path.stem or sorted_name in img_path.stem:
            return img_path
    
    return None


# =============================================================================
# Pipeline Steps
# =============================================================================

def step_0_extract_rgb(config: PipelineConfig) -> Path:
    """Etapa 0: Extrair RGB das imagens RAW."""
    print("\n" + "=" * 70)
    print("ETAPA 0: Extração de RGB das RAW")
    print("=" * 70)
    
    rgb_dir = config.output_folder / "01_rgb_extracted"
    rgb_dir.mkdir(parents=True, exist_ok=True)
    
    raw_images = list_images(config.raw_images_dir)
    print(f"\n📸 RAW Images: {len(raw_images)}")
    
    success = 0
    for img in raw_images:
        rgb_path = rgb_dir / f"{img.stem}_rgb.jpg"
        if rgb_path.exists():
            success += 1
            continue
        print(f"   ⏳ {img.name} → ", end="")
        if extract_rgb_from_flir(img, rgb_path):
            print("✅")
            success += 1
        else:
            print("❌")
    
    print(f"\n✅ RGB extraído: {success}/{len(raw_images)}")
    return rgb_dir


def step_1_clean_thermal(config: PipelineConfig) -> Path:
    """Etapa 1: Limpar UI das imagens RAW (gerar thermal clean para YOLO)."""
    print("\n" + "=" * 70)
    print("ETAPA 1: Gerar Thermal Clean (para YOLO)")
    print("=" * 70)
    
    if config.skip_clean:
        print("⏭️  Clean ignorado (--skip-clean)")
        return config.raw_images_dir
    
    clean_dir = config.output_folder / "02_thermal_clean"
    
    cmd = [
        sys.executable, "-m", "src.tools.clean_ui",
        "--input", str(config.raw_images_dir),
        "--output", str(clean_dir),
        "--colormap", "inferno",
    ]
    
    try:
        print("   ⏳ Limpando UI das imagens...")
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
        if proc.returncode == 0:
            clean_count = len(list_images(clean_dir))
            print(f"   ✅ Thermal clean: {clean_count} imagens")
            return clean_dir
        else:
            print(f"   ❌ Erro: {proc.stderr[:300] if proc.stderr else 'Unknown'}")
    except Exception as e:
        print(f"   ❌ Erro: {e}")
    
    print("   ⚠️  Fallback: usando RAW sem limpeza")
    return config.raw_images_dir


def step_2_wp1_sorting(config: PipelineConfig) -> Tuple[Path, Dict[str, str]]:
    """
    Etapa 2: WP1 - Sorting por EXIF ImageDescription.
    Retorna: (sorted_dir, mapping de original_stem → sorted_name)
    """
    print("\n" + "=" * 70)
    print("ETAPA 2: WP1 - Sorting & Classification")
    print("=" * 70)
    
    if config.skip_wp1:
        print("⏭️  WP1 ignorado (--skip-wp1)")
        # Retorna mapping identidade
        mapping = {img.stem: img.stem for img in list_images(config.raw_images_dir)}
        return config.raw_images_dir, mapping
    
    output_dir = config.output_folder / "03_wp1_sorted"
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Agrupar por EXIF ImageDescription
    raw_images = list_images(config.raw_images_dir)
    groups = {}
    
    for img_path in raw_images:
        desc = get_exif_description(img_path)
        groups.setdefault(desc, []).append(img_path)
    
    print(f"   📂 Grupos: {len(groups)}")
    
    # Ordenar e criar nomes sorted
    group_order = sorted(groups.keys())
    total_count = len(raw_images)
    
    # Mapping: original_stem → sorted_name
    name_mapping = {}
    global_idx = 1
    
    for group_idx, group_name in enumerate(group_order, start=1):
        imgs = sorted(groups[group_name], key=lambda p: p.name)
        group_dir = output_dir / group_name
        group_dir.mkdir(parents=True, exist_ok=True)
        
        print(f"      - {group_name}: {len(imgs)}")
        
        for position, img_path in enumerate(imgs, start=1):
            # Formato: 01-0036-15-INSIDE-01-GroupName_FLIR3925
            sorted_name = f"{global_idx:02d}-{total_count:04d}-{len(imgs):02d}-INSIDE-{group_idx:02d}-{group_name}_{img_path.stem}"
            name_mapping[img_path.stem] = sorted_name
            
            # Copiar arquivo com novo nome
            new_path = group_dir / f"{sorted_name}{img_path.suffix}"
            shutil.copy2(img_path, new_path)
            global_idx += 1
    
    print(f"\n✅ WP1 concluído: {total_count} imagens organizadas")
    return output_dir, name_mapping


def step_3_wp2_thermal_tuning(config: PipelineConfig, input_dir: Path) -> Path:
    """Etapa 3: WP2 - Thermal Range Tuning."""
    print("\n" + "=" * 70)
    print("ETAPA 3: WP2 - Thermal Range Tuning")
    print("=" * 70)
    
    if config.skip_wp2:
        print("⏭️  WP2 ignorado (--skip-wp2)")
        return input_dir
    
    output_dir = config.output_folder / "04_wp2_thermal_tuned"
    
    atlas_cli = None
    if config.atlas_sdk_dir:
        atlas_cli = config.atlas_sdk_dir / "build" / "atlas_scale_set"
    
    if atlas_cli and atlas_cli.exists():
        print(f"✅ Atlas SDK encontrado: {atlas_cli}")
        cmd = [
            sys.executable, "-m", "src.tools.tuning_thermal",
            "--input", str(input_dir),
            "--output", str(output_dir),
            "--atlas-sdk-dir", str(config.atlas_sdk_dir),
            "--palette", config.wp2_palette,
        ]
        try:
            proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
            if proc.returncode == 0:
                print("   ✅ Thermal tuning concluído")
                return output_dir
            else:
                print(f"   ❌ tuning_thermal falhou")
        except Exception as e:
            print(f"   ❌ Erro: {e}")
    else:
        print("⚠️  Atlas SDK não encontrado")
    
    # Fallback: copiar
    print("   ℹ️  Copiando imagens sem tuning...")
    output_dir.mkdir(parents=True, exist_ok=True)
    for img in list_images_recursive(input_dir):
        rel_path = img.relative_to(input_dir) if input_dir in img.parents else Path(img.name)
        dest = output_dir / rel_path
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(img, dest)
    
    return output_dir


def step_4_wp4_wp5_analysis(
    config: PipelineConfig,
    rgb_dir: Path,
    clean_dir: Path,
    tuned_dir: Path,
    name_mapping: Dict[str, str],
) -> Tuple[Path, List[ImageResult]]:
    """
    Etapa 4: WP4+WP5 - YOLO keypoints + Clustering + Anomalias.
    
    IMPORTANTE: Usa imagens do thermal tuning (tuned_dir) como fonte para
    injeção de spots, garantindo que o scale range seja consistente com FLIR Tools.
    
    Detecta keypoints nas imagens TÉRMICAS (clean), mapeia para RGB, faz clustering.
    Gera visualizações em AMBAS as imagens (thermal + RGB).
    """
    print("\n" + "=" * 70)
    print("ETAPA 4: WP4+WP5 - Keypoint Analysis (usando Thermal Tuned)")
    print("=" * 70)
    
    if config.skip_wp4_wp5:
        print("⏭️  WP4+WP5 ignorado")
        return config.output_folder / "05_wp4_wp5_analysis", []
    
    output_dir = config.output_folder / "05_wp4_wp5_analysis"
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"\n🎯 Keypoint source: {config.keypoint_source.upper()}")
    if config.keypoint_source == "yolo":
        print(f"   YOLO model: {config.yolo_model}")
        print(f"   YOLO conf:  {config.yolo_conf}")
    
    # Listar imagens TUNED (não RAW originais!)
    tuned_images = list_images_recursive(tuned_dir)
    print(f"\n📸 Tuned Images: {len(tuned_images)}")
    print(f"   📂 Source: {tuned_dir}")
    
    # Criar mapeamento reverso: sorted_name_part → original_stem
    reverse_mapping = {}
    for orig_stem, sorted_name in name_mapping.items():
        reverse_mapping[sorted_name] = orig_stem
        # Também mapear parcialmente (caso o nome seja truncado)
        reverse_mapping[orig_stem] = orig_stem
    
    results: List[ImageResult] = []
    
    for tuned_path in tuned_images:
        tuned_stem = tuned_path.stem
        
        # Extrair o original_stem do nome tuned
        # Formato típico: 01-0036-15-INSIDE-01-GroupName_FLIR3925
        # O original_stem está após o último "_" ou é o próprio nome
        original_stem = None
        
        # Tentar encontrar o stem original no nome
        for orig, sorted_name in name_mapping.items():
            if orig in tuned_stem or sorted_name == tuned_stem:
                original_stem = orig
                break
        
        if original_stem is None:
            # Fallback: usar o próprio stem (pode ser que não tenha passado pelo sorting)
            original_stem = tuned_stem
        
        sorted_name = name_mapping.get(original_stem, tuned_stem)
        
        print(f"\n{'─' * 50}")
        print(f"📷 {tuned_path.name}")
        print(f"   Original: {original_stem}")
        
        result = ImageResult(filename=original_stem, sorted_name=sorted_name)
        
        # RGB image (baseado no original_stem)
        rgb_path = rgb_dir / f"{original_stem}_rgb.jpg"
        if not rgb_path.exists():
            print(f"   ⚠️  RGB não encontrado: {rgb_path.name}")
            result.status = "error"
            result.error_message = "RGB not found"
            results.append(result)
            continue
        
        # Clean thermal image (baseado no original_stem)
        clean_path = None
        clean_candidates = [
            clean_dir / f"{original_stem}_clean.png",
            clean_dir / f"{original_stem}.png",
            clean_dir / f"{original_stem}_clean.jpg",
        ]
        for cp in clean_candidates:
            if cp.exists():
                clean_path = cp
                break
        
        if clean_path is None:
            print(f"   ⚠️  Clean thermal não encontrado")
            result.status = "error"
            result.error_message = "Clean thermal not found"
            results.append(result)
            continue
        
        print(f"   📷 Clean: {clean_path.name}")
        print(f"   📷 Tuned: {tuned_path.name}")
        
        # Criar pasta de output
        img_output_dir = output_dir / sorted_name
        img_output_dir.mkdir(parents=True, exist_ok=True)
        
        # Criar subpastas para thermal e rgb
        (img_output_dir / "thermal").mkdir(exist_ok=True)
        (img_output_dir / "rgb").mkdir(exist_ok=True)
        
        # Montar comando wp4_wp5_integration_v4
        # IMPORTANTE: --raw-image agora aponta para a imagem TUNED!
        cmd = [
            sys.executable, "-m", "src.tools.wp4_wp5_integration_v4",
            "--raw-image", str(tuned_path),  # ← TUNED, não RAW original!
            "--rgb-image", str(rgb_path),
            "--thermal-clean-image", str(clean_path),
            "--keypoint-source", "yolo",
            "--yolo-model", str(config.yolo_model),
            "--yolo-conf", str(config.yolo_conf),
            "--yolo-image", str(clean_path),
            "--extract-temperatures",
            "--kernel-size", str(config.kernel_size),
            "--delta-threshold", str(config.delta_threshold),
            "--openai-model", config.openai_model,
            "--output", str(img_output_dir),
            "--output-prefix", sorted_name,
            "--scale-to-rgb",
            "--clip-outside",
            "--dual-viz",
            "--inject-exif",
        ]
        
        # Alignment override (se especificado manualmente)
        if config.real2ir is not None:
            cmd.extend(["--real2ir", str(config.real2ir)])
        if config.offset_x is not None:
            cmd.extend(["--offset-x", str(config.offset_x)])
        if config.offset_y is not None:
            cmd.extend(["--offset-y", str(config.offset_y)])
        
        print(f"   ⏳ Análise WP4+WP5...")
        
        try:
            proc = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
            if proc.returncode != 0:
                print(f"   ❌ Erro")
                err_text = (proc.stderr or "").strip()
                out_text = (proc.stdout or "").strip()
                combined = err_text if err_text else out_text
                if combined:
                    # Mostrar apenas últimas linhas relevantes
                    tail_lines = combined.split('\n')[-8:]
                    for line in tail_lines:
                        if line.strip():
                            print(f"      {line[:160]}")
                result.status = "error"
                if combined:
                    result.error_message = combined[:800]
                else:
                    result.error_message = "Unknown"
            else:
                # Parse results
                anomalies_path = img_output_dir / "anomalies.json"
                if anomalies_path.exists():
                    try:
                        data = json.loads(anomalies_path.read_text(encoding="utf-8"))
                        anomalies = data.get("anomalies", []) if isinstance(data, dict) else []
                        result.anomalies = anomalies if isinstance(anomalies, list) else []
                        result.anomaly_count = int(data.get("total_anomalies", len(result.anomalies))) if isinstance(data, dict) else len(result.anomalies)
                    except json.JSONDecodeError as e:
                        result.status = "error"
                        result.error_message = f"Invalid anomalies.json: {e}"

                kp_path = img_output_dir / "keypoints_used.json"
                if kp_path.exists():
                    try:
                        result.keypoint_count = len(json.loads(kp_path.read_text(encoding="utf-8")))
                    except json.JSONDecodeError as e:
                        result.status = "error"
                        result.error_message = f"Invalid keypoints_used.json: {e}"

                if result.status != "error":
                    result.status = "success"
                    print(f"   ✅ {result.keypoint_count} kps, {result.anomaly_count} anomalias")

        except subprocess.TimeoutExpired:
            print(f"   ❌ Timeout (300s)")
            result.status = "error"
            result.error_message = "Timeout"
        except Exception as e:
            print(f"   ❌ {e}")
            result.status = "error"
            result.error_message = str(e)
        
        results.append(result)
    
    success = sum(1 for r in results if r.status == "success")
    errors = sum(1 for r in results if r.status == "error")
    print(f"\n{'=' * 50}")
    print(f"✅ WP4+WP5: {success} ok, {errors} erros")
    
    return output_dir, results


def step_5_wp6_report(
    config: PipelineConfig,
    analysis_dir: Path,
    results: List[ImageResult],
    pipeline_result: PipelineResult
) -> Path:
    """Etapa 5: WP6 - Relatório REPX."""
    print("\n" + "=" * 70)
    print("ETAPA 5: WP6 - Report Generation")
    print("=" * 70)
    
    if config.skip_wp6:
        print("⏭️  WP6 ignorado")
        return config.output_folder / "06_report"
    
    report_dir = config.output_folder / "06_report"
    
    # Usar wp6_repx_generator.py
    cmd = [
        sys.executable, "-m", "src.tools.wp6_repx_generator",
        "--edited-dir", str(analysis_dir / "edited"),  # pasta edited/
        "--output", str(report_dir / "report.repx"),   # arquivo .repx 
        "--site-name", pipeline_result.site_name,
    ]
    
    # Adicionar logo se existir
    logo_candidates = [
        Path(__file__).parent / "QE_SOLAR_logo.png",
        Path("/home/elton/qe_solar_wires/src/tools/QE_SOLAR_logo.png"),
    ]
    for logo_path in logo_candidates:
        if logo_path.exists():
            cmd.extend(["--logo", str(logo_path)])
            break
    
    try:
        print("   ⏳ Gerando relatório...")
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
        if proc.returncode == 0:
            print("   ✅ Relatório gerado")
        else:
            print(f"   ⚠️  Erro: {proc.stderr[:200] if proc.stderr else ''}")
    except Exception as e:
        print(f"   ❌ Erro: {e}")
    
    # Gerar summary básico
    report_dir.mkdir(parents=True, exist_ok=True)
    
    all_anomalies = []
    for r in results:
        for a in r.anomalies:
            a["image"] = r.sorted_name
            a["original"] = r.filename
            all_anomalies.append(a)
    
    pipeline_result.total_anomalies = len(all_anomalies)
    pipeline_result.high_severity = sum(1 for a in all_anomalies if a.get("severity") == "high")
    pipeline_result.medium_severity = sum(1 for a in all_anomalies if a.get("severity") == "medium")
    pipeline_result.low_severity = sum(1 for a in all_anomalies if a.get("severity") == "low")
    
    summary = {
        "site_name": pipeline_result.site_name,
        "generated_at": datetime.now().isoformat(),
        "mode": "RAW-only (Tuned)",
        "total_images": pipeline_result.total_images,
        "processed": pipeline_result.processed_images,
        "anomalies": {
            "total": pipeline_result.total_anomalies,
            "high": pipeline_result.high_severity,
            "medium": pipeline_result.medium_severity,
            "low": pipeline_result.low_severity,
        },
    }
    (report_dir / "site_summary.json").write_text(json.dumps(summary, indent=2))
    
    # CSV
    if all_anomalies:
        csv_path = report_dir / "anomalies_report.csv"
        with csv_path.open("w", newline="") as f:
            fieldnames = ["image", "original", "keypoint_id", "component_type",
                         "temperature_c", "delta_to_median_c", "severity"]
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            for a in all_anomalies:
                writer.writerow({k: a.get(k, "") for k in fieldnames})
    
    return report_dir


# =============================================================================
# Main
# =============================================================================

def run_pipeline(config: PipelineConfig) -> PipelineResult:
    """Executa pipeline completo (RAW-only mode)."""
    
    start = time.time()
    site_name = config.site_folder.name
    
    result = PipelineResult(
        site_name=site_name,
        start_time=datetime.now().isoformat(),
    )
    
    print("\n" + "=" * 70)
    print("🔥 QE SOLAR - FULL PIPELINE (RAW-Only Mode)")
    print("=" * 70)
    print(f"Site: {site_name}")
    print(f"Output: {config.output_folder}")
    print(f"RAW dir: {config.raw_images_dir}")
    print(f"Keypoint source: {config.keypoint_source.upper()}")
    if config.keypoint_source == "yolo":
        print(f"YOLO model: {config.yolo_model}")
    
    # Verificar RAW
    if not config.raw_images_dir.exists():
        print(f"❌ RAW Images não encontrado: {config.raw_images_dir}")
        return result
    
    config.output_folder.mkdir(parents=True, exist_ok=True)
    
    # Etapa 0: Extrair RGB
    rgb_dir = step_0_extract_rgb(config)
    
    # Etapa 1: Clean thermal
    clean_dir = step_1_clean_thermal(config)
    
    # Etapa 2: WP1 Sorting
    sorted_dir, name_mapping = step_2_wp1_sorting(config)
    
    # Etapa 3: WP2 Thermal tuning
    tuned_dir = step_3_wp2_thermal_tuning(config, sorted_dir)
    
    # Etapa 4: WP4+WP5 (agora usa tuned_dir!)
    analysis_dir, image_results = step_4_wp4_wp5_analysis(
        config, rgb_dir, clean_dir, tuned_dir, name_mapping
    )
    
    result.total_images = len(image_results)
    result.processed_images = sum(1 for r in image_results if r.status == "success")
    result.failed_images = sum(1 for r in image_results if r.status == "error")
    result.image_results = image_results
    
    # Etapa 5: WP6 Report
    report_dir = step_5_wp6_report(config, analysis_dir, image_results, result)
    
    # Finalizar
    result.end_time = datetime.now().isoformat()
    result.duration_seconds = round(time.time() - start, 1)
    
    # Salvar log
    log_path = config.output_folder / "pipeline_log.json"
    log_path.write_text(json.dumps(asdict(result), indent=2, default=str))
    
    # Resumo final
    print("\n" + "=" * 70)
    print("✅ PIPELINE CONCLUÍDO!")
    print("=" * 70)
    print(f"\n📊 Resumo:")
    print(f"   Total de imagens:    {result.total_images}")
    print(f"   Processadas:         {result.processed_images}")
    print(f"   Erros:               {result.failed_images}")
    print(f"   Anomalias totais:    {result.total_anomalies}")
    print(f"      🔴 High:          {result.high_severity}")
    print(f"      🟠 Medium:        {result.medium_severity}")
    print(f"      🟡 Low:           {result.low_severity}")
    print(f"⏱️  Duração: {result.duration_seconds}s")
    print(f"📁 Outputs em: {config.output_folder}")
    
    return result


def main():
    parser = argparse.ArgumentParser(
        description="QE Solar Full Pipeline (RAW-Only Mode)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Exemplos:
  # Básico
  python -m src.tools.run_full_pipeline \\
      --site-folder "data/site/2025" \\
      --output "output/site_2025"

  # Com YOLO customizado
  python -m src.tools.run_full_pipeline \\
      --site-folder "data/site/2025" \\
      --output "output/site_2025" \\
      --yolo-model "models/best.pt" \\
      --yolo-conf 0.15

  # Com alinhamento manual
  python -m src.tools.run_full_pipeline \\
      --site-folder "data/site/2025" \\
      --output "output/site_2025" \\
      --real2ir 1.74 --offset-x -10 --offset-y -13
        """
    )
    
    parser.add_argument("--site-folder", required=True, help="Pasta do site com RAW Images")
    parser.add_argument("--output", "-o", required=True, help="Pasta de output")
    
    parser.add_argument("--raw-subdir", default=None, help="Nome da subpasta RAW (auto-detecta)")
    
    parser.add_argument("--keypoint-source", choices=["yolo", "exif"], default="yolo")
    parser.add_argument("--yolo-model", default="models/best.pt")
    parser.add_argument("--yolo-conf", type=float, default=0.30)
    
    parser.add_argument("--skip-clean", action="store_true")
    parser.add_argument("--skip-wp1", action="store_true")
    parser.add_argument("--skip-wp2", action="store_true")
    parser.add_argument("--skip-wp4-wp5", action="store_true")
    parser.add_argument("--skip-wp6", action="store_true")
    
    # Alinhamento manual
    parser.add_argument("--real2ir", type=float, help="Real2IR override")
    parser.add_argument("--offset-x", type=float, help="Offset X override")
    parser.add_argument("--offset-y", type=float, help="Offset Y override")
    
    parser.add_argument("--atlas-sdk-dir", help="Atlas SDK directory")
    parser.add_argument("--openai-model", default="gpt-4o")
    parser.add_argument("--delta-threshold", type=float, default=3.0)
    
    args = parser.parse_args()
    
    config = PipelineConfig(
        site_folder=args.site_folder,
        output_folder=args.output,
        raw_subdir=args.raw_subdir,
        keypoint_source=args.keypoint_source,
        yolo_model=Path(args.yolo_model) if args.yolo_model else None,
        yolo_conf=args.yolo_conf,
        skip_clean=args.skip_clean,
        skip_wp1=args.skip_wp1,
        skip_wp2=args.skip_wp2,
        skip_wp4_wp5=args.skip_wp4_wp5,
        skip_wp6=args.skip_wp6,
        real2ir=args.real2ir,
        offset_x=args.offset_x,
        offset_y=args.offset_y,
        atlas_sdk_dir=Path(args.atlas_sdk_dir) if args.atlas_sdk_dir else None,
        openai_model=args.openai_model,
        delta_threshold=args.delta_threshold,
    )
    
    run_pipeline(config)


if __name__ == "__main__":
    main()