#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
WP4 YOLO Training Setup
========================
Configuração e execução de treino YOLO para detecção de keypoints térmicos.

Este script:
1. Valida a estrutura de dados
2. Gera arquivo YAML para YOLOv8
3. Configura e executa o treino
4. Gera relatório de validação

Uso:
    python train_yolo_wp4.py --data-dir wp4_data/ --epochs 100
"""

import argparse
import json
import subprocess
import sys
from collections import defaultdict
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple

import yaml


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]

# Classes para detecção de keypoints térmicos
DEFAULT_CLASSES = [
    "hotspot",           # Ponto quente genérico
    "terminal",          # Terminal/conexão
    "wire_connection",   # Conexão de fio
    "lug",              # Conector tipo lug
    "junction",         # Junção
    "cold_spot",        # Ponto frio (anomalia)
]


def validate_dataset_structure(data_dir: Path) -> Dict:
    """
    Valida estrutura do dataset e retorna estatísticas.
    
    Estrutura esperada:
    data_dir/
    ├── standard_images/  ou  images/
    ├── labels/
    └── splits/
        ├── train.txt
        └── val.txt
    """
    report = {
        "valid": True,
        "errors": [],
        "warnings": [],
        "stats": {},
    }
    
    # Verificar diretórios
    images_dir = data_dir / "standard_images"
    if not images_dir.exists():
        images_dir = data_dir / "images"
    
    labels_dir = data_dir / "labels"
    splits_dir = data_dir / "splits"
    
    if not images_dir.exists():
        report["errors"].append(f"Diretório de imagens não encontrado: {images_dir}")
        report["valid"] = False
    
    if not labels_dir.exists():
        report["warnings"].append(f"Diretório de labels não encontrado: {labels_dir}")
        # Não é erro fatal - pode ser dataset sem anotações ainda
    
    if not splits_dir.exists():
        report["warnings"].append(f"Diretório de splits não encontrado: {splits_dir}")
    
    # Contar arquivos
    if images_dir.exists():
        img_exts = {".jpg", ".jpeg", ".png"}
        images = [f for f in images_dir.rglob("*") if f.suffix.lower() in img_exts]
        report["stats"]["total_images"] = len(images)
    
    if labels_dir.exists():
        labels = list(labels_dir.glob("*.txt"))
        report["stats"]["total_labels"] = len(labels)
        
        # Verificar correspondência imagens <-> labels
        if images_dir.exists():
            def _norm_stem(stem: str) -> str:
                return stem[:-6] if stem.endswith("_clean") else stem

            img_stems = {_norm_stem(f.stem) for f in images}
            label_stems = {_norm_stem(f.stem) for f in labels}
            
            missing_labels = img_stems - label_stems
            orphan_labels = label_stems - img_stems
            
            if missing_labels:
                report["warnings"].append(
                    f"{len(missing_labels)} imagens sem label"
                )
            if orphan_labels:
                report["warnings"].append(
                    f"{len(orphan_labels)} labels sem imagem correspondente"
                )
            
            report["stats"]["images_with_labels"] = len(img_stems & label_stems)
    
    # Verificar splits
    if splits_dir.exists():
        train_txt = splits_dir / "train.txt"
        val_txt = splits_dir / "val.txt"
        
        if train_txt.exists():
            train_lines = [l.strip() for l in train_txt.read_text().splitlines() if l.strip()]
            report["stats"]["train_images"] = len(train_lines)
        else:
            report["warnings"].append("train.txt não encontrado")
        
        if val_txt.exists():
            val_lines = [l.strip() for l in val_txt.read_text().splitlines() if l.strip()]
            report["stats"]["val_images"] = len(val_lines)
        else:
            report["warnings"].append("val.txt não encontrado")
    
    return report


def analyze_labels(labels_dir: Path, classes: List[str]) -> Dict:
    """Analisa distribuição de classes nos labels."""
    stats = {
        "total_annotations": 0,
        "class_distribution": defaultdict(int),
        "images_per_class": defaultdict(set),
        "annotations_per_image": [],
    }
    
    for label_file in labels_dir.glob("*.txt"):
        lines = label_file.read_text().strip().splitlines()
        ann_count = 0
        
        for line in lines:
            if not line.strip():
                continue
            parts = line.strip().split()
            if len(parts) >= 5:  # class x y w h
                try:
                    cls_id = int(parts[0])
                    cls_name = classes[cls_id] if cls_id < len(classes) else f"class_{cls_id}"
                    stats["class_distribution"][cls_name] += 1
                    stats["images_per_class"][cls_name].add(label_file.stem)
                    stats["total_annotations"] += 1
                    ann_count += 1
                except (ValueError, IndexError):
                    continue
        
        stats["annotations_per_image"].append(ann_count)
    
    # Converter sets para contagens
    stats["images_per_class"] = {k: len(v) for k, v in stats["images_per_class"].items()}
    
    # Estatísticas de anotações por imagem
    if stats["annotations_per_image"]:
        ann_arr = stats["annotations_per_image"]
        stats["annotations_stats"] = {
            "min": min(ann_arr),
            "max": max(ann_arr),
            "mean": sum(ann_arr) / len(ann_arr),
            "total_files": len(ann_arr),
        }
    
    return stats


def generate_yolo_yaml(
    data_dir: Path,
    output_path: Path,
    classes: List[str],
    train_path: Optional[str] = None,
    val_path: Optional[str] = None,
) -> Path:
    """
    Gera arquivo YAML de configuração para YOLOv8.
    """
    data_dir = data_dir.resolve()
    
    # Determinar caminhos
    if train_path is None:
        train_path = str(data_dir / "splits" / "train.txt")
    if val_path is None:
        val_path = str(data_dir / "splits" / "val.txt")
    
    config = {
        "path": str(data_dir),
        "train": train_path,
        "val": val_path,
        "names": {i: name for i, name in enumerate(classes)},
    }
    
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w") as f:
        yaml.dump(config, f, default_flow_style=False, allow_unicode=True)
    
    return output_path


def run_yolo_training(
    yaml_path: Path,
    model: str = "yolov8n.pt",
    epochs: int = 100,
    imgsz: int = 640,
    batch: int = 16,
    project: str = "runs/wp4_keypoints",
    name: str = None,
    device: str = None,
    resume: bool = False,
    extra_args: List[str] = None,
) -> bool:
    """
    Executa treino YOLOv8.
    
    Returns:
        True se treino completou com sucesso
    """
    if name is None:
        name = f"train_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    
    cmd = [
        "yolo", "detect", "train",
        f"data={yaml_path}",
        f"model={model}",
        f"epochs={epochs}",
        f"imgsz={imgsz}",
        f"batch={batch}",
        f"project={project}",
        f"name={name}",
        "exist_ok=True",
        "plots=True",
        "save=True",
    ]
    
    if device:
        cmd.append(f"device={device}")
    
    if resume:
        cmd.append("resume=True")
    
    if extra_args:
        cmd.extend(extra_args)
    
    print(f"\n🚀 Executando treino YOLO:")
    print(f"   Comando: {' '.join(cmd)}")
    print("-" * 60)
    
    try:
        result = subprocess.run(cmd, check=True, cwd=str(_repo_root()))
        return result.returncode == 0
    except subprocess.CalledProcessError as e:
        print(f"\n❌ Erro no treino: {e}")
        return False
    except FileNotFoundError:
        print("\n❌ YOLO não encontrado. Instale com: pip install ultralytics")
        return False


def run_yolo_validation(
    model_path: Path,
    yaml_path: Path,
    split: str = "val",
    project: str = "runs/wp4_keypoints",
    name: str = None,
) -> Optional[Dict]:
    """
    Executa validação do modelo treinado.
    
    Returns:
        Métricas de validação ou None se falhou
    """
    if name is None:
        name = f"val_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    
    cmd = [
        "yolo", "detect", "val",
        f"model={model_path}",
        f"data={yaml_path}",
        f"split={split}",
        f"project={project}",
        f"name={name}",
        "plots=True",
    ]
    
    print(f"\n📊 Executando validação:")
    print(f"   Comando: {' '.join(cmd)}")
    print("-" * 60)
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=str(_repo_root()))
        print(result.stdout)
        if result.returncode == 0:
            # Tentar parsear métricas do output
            return {"status": "success", "output": result.stdout}
        return None
    except Exception as e:
        print(f"\n❌ Erro na validação: {e}")
        return None


def print_validation_report(report: Dict):
    """Imprime relatório de validação do dataset."""
    print("\n" + "=" * 60)
    print("📋 RELATÓRIO DE VALIDAÇÃO DO DATASET")
    print("=" * 60)
    
    if report["errors"]:
        print("\n❌ ERROS:")
        for err in report["errors"]:
            print(f"   - {err}")
    
    if report["warnings"]:
        print("\n⚠️  AVISOS:")
        for warn in report["warnings"]:
            print(f"   - {warn}")
    
    print("\n📊 ESTATÍSTICAS:")
    for key, value in report["stats"].items():
        print(f"   {key}: {value}")
    
    print("\n" + "=" * 60)
    print(f"✅ Dataset válido: {report['valid']}")
    print("=" * 60)


def main():
    parser = argparse.ArgumentParser(
        description="Setup e treino YOLO para keypoints WP4",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    
    parser.add_argument("--data-dir", "-d", required=True,
                       help="Diretório do dataset preparado")
    parser.add_argument("--action", choices=["validate", "train", "val", "all"],
                       default="all", help="Ação a executar")
    
    # Parâmetros de treino
    parser.add_argument("--model", default="yolov8n.pt",
                       help="Modelo base (yolov8n/s/m/l/x.pt)")
    parser.add_argument("--epochs", type=int, default=100,
                       help="Número de épocas")
    parser.add_argument("--imgsz", type=int, default=640,
                       help="Tamanho da imagem")
    parser.add_argument("--batch", type=int, default=16,
                       help="Batch size")
    parser.add_argument("--device", help="Device (cuda:0, cpu, etc)")
    parser.add_argument("--resume", action="store_true",
                       help="Continuar treino anterior")
    
    # Classes customizadas
    parser.add_argument("--classes", nargs="+", default=DEFAULT_CLASSES,
                       help="Lista de classes")
    parser.add_argument("--classes-file", help="Arquivo JSON com classes")
    
    # Outputs
    parser.add_argument("--project", default=str(_repo_root() / "runs" / "wp4_keypoints"),
                       help="Diretório do projeto")
    parser.add_argument("--name", help="Nome do experimento")
    
    # Modelo para validação
    parser.add_argument("--weights", help="Pesos para validação (best.pt)")
    
    args = parser.parse_args()
    
    data_dir = Path(args.data_dir)
    
    if not data_dir.exists():
        print(f"❌ Diretório não encontrado: {data_dir}")
        sys.exit(1)
    
    # Carregar classes
    classes = args.classes
    if args.classes_file:
        with open(args.classes_file) as f:
            classes = json.load(f)
    
    print(f"\n🎯 Classes configuradas ({len(classes)}):")
    for i, c in enumerate(classes):
        print(f"   {i}: {c}")
    
    # ==========================================================================
    # VALIDAÇÃO
    # ==========================================================================
    if args.action in ["validate", "all"]:
        print("\n" + "=" * 60)
        print("ETAPA: VALIDAÇÃO DO DATASET")
        print("=" * 60)
        
        report = validate_dataset_structure(data_dir)
        print_validation_report(report)
        
        # Análise de labels se existirem
        labels_dir = data_dir / "labels"
        if labels_dir.exists():
            print("\n📊 Análise de Labels:")
            label_stats = analyze_labels(labels_dir, classes)
            print(f"   Total de anotações: {label_stats['total_annotations']}")
            print(f"   Distribuição por classe:")
            for cls, count in sorted(label_stats["class_distribution"].items()):
                print(f"      {cls}: {count}")
        
        if not report["valid"] and args.action == "all":
            print("\n⛔ Dataset inválido. Corrija os erros antes de treinar.")
            sys.exit(1)
    
    # ==========================================================================
    # PREPARAÇÃO DO YAML
    # ==========================================================================
    yaml_path = data_dir / "dataset.yaml"
    
    if args.action in ["train", "all"]:
        print("\n" + "=" * 60)
        print("ETAPA: PREPARAÇÃO DO TREINO")
        print("=" * 60)
        
        generate_yolo_yaml(data_dir, yaml_path, classes)
        print(f"✅ YAML gerado: {yaml_path}")
    
    # ==========================================================================
    # TREINO
    # ==========================================================================
    if args.action in ["train", "all"]:
        print("\n" + "=" * 60)
        print("ETAPA: TREINO")
        print("=" * 60)
        
        success = run_yolo_training(
            yaml_path=yaml_path,
            model=args.model,
            epochs=args.epochs,
            imgsz=args.imgsz,
            batch=args.batch,
            project=args.project,
            name=args.name,
            device=args.device,
            resume=args.resume,
        )
        
        if success:
            print("\n✅ Treino concluído com sucesso!")
        else:
            print("\n❌ Treino falhou")
            if args.action == "all":
                sys.exit(1)
    
    # ==========================================================================
    # VALIDAÇÃO DO MODELO
    # ==========================================================================
    if args.action in ["val"]:
        if not args.weights:
            print("❌ Especifique --weights para validação")
            sys.exit(1)
        
        print("\n" + "=" * 60)
        print("ETAPA: VALIDAÇÃO DO MODELO")
        print("=" * 60)
        
        metrics = run_yolo_validation(
            model_path=Path(args.weights),
            yaml_path=yaml_path,
            project=args.project,
        )
        
        if metrics:
            print("\n✅ Validação concluída!")


if __name__ == "__main__":
    main()
