# train_yolo_wp4.py

## Documentação Técnica

**Módulo:** `src.tools.train_yolo_wp4`  
**Versão:** 1.0  
**Autor:** Elton Gino / QE Solar  
**Última atualização:** Fevereiro 2025

---

## Sumário

1. [Descrição](#1-descrição)
2. [Localização e Execução](#2-localização-e-execução)
3. [Dependências](#3-dependências)
4. [Estrutura do Dataset](#4-estrutura-do-dataset)
5. [Funções Principais](#5-funções-principais)
6. [Argumentos CLI](#6-argumentos-cli)
7. [Fluxo de Execução](#7-fluxo-de-execução)
8. [Exemplos de Uso](#8-exemplos-de-uso)
9. [Modelo Treinado](#9-modelo-treinado)
10. [Notas de Implementação](#10-notas-de-implementação)

---

## 1. Descrição

### 1.1 Propósito

O `train_yolo_wp4.py` é o script de **configuração e treino do modelo YOLO** para detecção de keypoints térmicos. Ele automatiza todo o processo desde validação do dataset até execução do treino.

### 1.2 Funcionalidades

| Funcionalidade | Descrição |
|----------------|-----------|
| **Validação de Dataset** | Verifica estrutura e consistência dos dados |
| **Análise de Labels** | Estatísticas de distribuição de classes |
| **Geração de YAML** | Cria arquivo de configuração YOLOv8n |
| **Treino** | Executa treino via CLI do Ultralytics |
| **Validação de Modelo** | Avalia modelo treinado no conjunto de validação |

### 1.3 Contexto no Pipeline

```
PREPARAÇÃO (offline)
      │
      ├── Preparação do dataset (manual/outros scripts)
      │     └── data/wp4_dataset/
      │           ├── standard_images/
      │           ├── labels/
      │           └── splits/
      │
      └── train_yolo_wp4.py  ◄─── ESTE MÓDULO
            │
            └── Output: runs/wp4_keypoints/train_*/weights/best.pt
                              │
                              └── Usado por: wp4_wp5_integration_v4.py
```

---

## 2. Localização e Execução

### 2.1 Caminho do Arquivo

```
qe_solar_wires/src/tools/train_yolo_wp4.py
```

### 2.2 Execução

```bash
# Treino completo
python src/tools/train_yolo_wp4.py --data-dir data/wp4_dataset/ --epochs 100

# Apenas validação do dataset
python src/tools/train_yolo_wp4.py --data-dir data/wp4_dataset/ --action validate

# Validação de modelo treinado
python src/tools/train_yolo_wp4.py --data-dir data/wp4_dataset/ --action val --weights runs/wp4_keypoints/train_*/weights/best.pt
```

---

## 3. Dependências

### 3.1 Bibliotecas Python

```python
# Standard Library
import argparse
import json
import subprocess
import sys
from collections import defaultdict
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# Third-party
import yaml                 # PyYAML para geração do dataset.yaml
```

### 3.2 Ferramentas Externas

| Ferramenta | Obrigatório | Função |
|------------|-------------|--------|
| `yolo` CLI | ✅ SIM | Execução do treino (Ultralytics) |

### 3.3 Instalação

```bash
pip install ultralytics pyyaml
```

---

## 4. Estrutura do Dataset

### 4.1 Estrutura Esperada

```
data/wp4_dataset/
├── standard_images/     # ou images/
│   ├── image1_clean.png
│   ├── image2_clean.png
│   └── ...
│
├── labels/
│   ├── image1_clean.txt
│   ├── image2_clean.txt
│   └── ...
│
└── splits/
    ├── train.txt        # Lista de imagens para treino
    └── val.txt          # Lista de imagens para validação
```

### 4.2 Formato dos Labels (YOLO)

```
# labels/image1_clean.txt
# class_id center_x center_y width height (valores normalizados 0-1)
0 0.4531 0.3125 0.0234 0.0312
0 0.5123 0.4567 0.0198 0.0287
0 0.6789 0.2345 0.0256 0.0301
```

### 4.3 Formato dos Splits

```
# splits/train.txt
/absolute/path/to/image1_clean.png
/absolute/path/to/image2_clean.png

# splits/val.txt
/absolute/path/to/image3_clean.png
/absolute/path/to/image4_clean.png
```

---

## 5. Funções Principais

### 5.1 Classes Padrão

```python
DEFAULT_CLASSES = [
    "hotspot",           # Ponto quente genérico
    "terminal",          # Terminal/conexão
    "wire_connection",   # Conexão de fio
    "lug",              # Conector tipo lug
    "junction",         # Junção
    "cold_spot",        # Ponto frio (anomalia)
]
```

**Nota:** No treino atual, apenas a classe 0 (`hotspot`) é usada para detecção de keypoints genéricos.

### 5.2 `validate_dataset_structure(data_dir: Path) -> Dict`

Valida a estrutura do dataset.

```python
def validate_dataset_structure(data_dir: Path) -> Dict:
    """
    Valida estrutura do dataset e retorna estatísticas.
    
    Returns:
        {
            "valid": bool,
            "errors": List[str],
            "warnings": List[str],
            "stats": {
                "total_images": int,
                "total_labels": int,
                "images_with_labels": int,
                "train_images": int,
                "val_images": int
            }
        }
    """
```

**Verificações realizadas:**

| Verificação | Tipo |
|-------------|------|
| Diretório de imagens existe | Erro |
| Diretório de labels existe | Warning |
| Diretório de splits existe | Warning |
| Correspondência imagem ↔ label | Warning |
| train.txt existe | Warning |
| val.txt existe | Warning |

### 5.3 `analyze_labels(labels_dir: Path, classes: List[str]) -> Dict`

Analisa distribuição de classes nos labels.

```python
def analyze_labels(labels_dir: Path, classes: List[str]) -> Dict:
    """
    Returns:
        {
            "total_annotations": int,
            "class_distribution": Dict[str, int],
            "images_per_class": Dict[str, int],
            "annotations_stats": {
                "min": int,
                "max": int,
                "mean": float,
                "total_files": int
            }
        }
    """
```

### 5.4 `generate_yolo_yaml(...) -> Path`

Gera arquivo YAML de configuração para YOLOv8n.

```python
def generate_yolo_yaml(
    data_dir: Path,
    output_path: Path,
    classes: List[str],
    train_path: Optional[str] = None,
    val_path: Optional[str] = None,
) -> Path:
```

**YAML gerado:**

```yaml
path: /absolute/path/to/data_dir
train: /absolute/path/to/splits/train.txt
val: /absolute/path/to/splits/val.txt
names:
  0: hotspot
  1: terminal
  2: wire_connection
  3: lug
  4: junction
  5: cold_spot
```

### 5.5 `run_yolo_training(...) -> bool`

Executa treino YOLOv8n via subprocess.

```python
def run_yolo_training(
    yaml_path: Path,
    model: str = "yolov8n.pt",      # Modelo base
    epochs: int = 100,
    imgsz: int = 640,
    batch: int = 16,
    project: str = "runs/wp4_keypoints",
    name: str = None,                # Auto: train_YYYYMMDD_HHMMSS
    device: str = None,
    resume: bool = False,
    extra_args: List[str] = None,
) -> bool:
```

**Comando gerado:**

```bash
yolo detect train \
    data=dataset.yaml \
    model=yolov8n.pt \
    epochs=100 \
    imgsz=640 \
    batch=16 \
    project=runs/wp4_keypoints \
    name=train_20251211_010508 \
    exist_ok=True \
    plots=True \
    save=True
```

### 5.6 `run_yolo_validation(...) -> Optional[Dict]`

Executa validação do modelo treinado.

```python
def run_yolo_validation(
    model_path: Path,
    yaml_path: Path,
    split: str = "val",
    project: str = "runs/wp4_keypoints",
    name: str = None,
) -> Optional[Dict]:
```

**Comando gerado:**

```bash
yolo detect val \
    model=runs/wp4_keypoints/train_*/weights/best.pt \
    data=dataset.yaml \
    split=val \
    project=runs/wp4_keypoints \
    name=val_20251211_120000 \
    plots=True
```

---

## 6. Argumentos CLI

### 6.1 Tabela de Argumentos

| Argumento | Obrigatório | Default | Descrição |
|-----------|-------------|---------|-----------|
| `--data-dir` / `-d` | ✅ | - | Diretório do dataset |
| `--action` | ❌ | `all` | `validate`, `train`, `val`, `all` |
| `--model` | ❌ | `yolov8n.pt` | Modelo base |
| `--epochs` | ❌ | `100` | Número de épocas |
| `--imgsz` | ❌ | `640` | Tamanho da imagem |
| `--batch` | ❌ | `16` | Batch size |
| `--device` | ❌ | Auto | Device (cuda:0, cpu) |
| `--resume` | ❌ | False | Continuar treino anterior |
| `--classes` | ❌ | DEFAULT_CLASSES | Lista de classes |
| `--classes-file` | ❌ | - | JSON com classes |
| `--project` | ❌ | `runs/wp4_keypoints` | Diretório do projeto |
| `--name` | ❌ | Auto | Nome do experimento |
| `--weights` | ❌ | - | Pesos para validação |

### 6.2 Ações Disponíveis

| Ação | Descrição |
|------|-----------|
| `validate` | Apenas valida o dataset |
| `train` | Gera YAML e executa treino |
| `val` | Valida modelo existente (requer `--weights`) |
| `all` | Validação + Treino completo |

---

## 7. Fluxo de Execução

```
main()
  │
  ├── Parse argumentos
  │
  ├── Carregar classes (args ou JSON)
  │
  ├── [Se action in (validate, all)]
  │     │
  │     ├── validate_dataset_structure()
  │     │
  │     ├── print_validation_report()
  │     │
  │     ├── analyze_labels() [se labels/ existir]
  │     │
  │     └── [Se inválido e action=all]: exit(1)
  │
  ├── [Se action in (train, all)]
  │     │
  │     ├── generate_yolo_yaml()
  │     │
  │     └── run_yolo_training()
  │           └── subprocess: yolo detect train ...
  │
  └── [Se action = val]
        │
        └── run_yolo_validation()
              └── subprocess: yolo detect val ...
```

---

## 8. Exemplos de Uso

### 8.1 Treino Completo (Recomendado)

```bash
python src/tools/train_yolo_wp4.py \
    --data-dir data/wp4_dataset/ \
    --epochs 100 \
    --batch 16 \
    --imgsz 640 \
    --model yolov8n.pt
```

### 8.2 Treino com GPU Específica

```bash
python src/tools/train_yolo_wp4.py \
    --data-dir data/wp4_dataset/ \
    --epochs 150 \
    --batch 32 \
    --device cuda:0
```

### 8.3 Continuar Treino Interrompido

```bash
python src/tools/train_yolo_wp4.py \
    --data-dir data/wp4_dataset/ \
    --resume \
    --name train_20251211_010508
```

### 8.4 Apenas Validar Dataset

```bash
python src/tools/train_yolo_wp4.py \
    --data-dir data/wp4_dataset/ \
    --action validate
```

### 8.5 Validar Modelo Treinado

```bash
python src/tools/train_yolo_wp4.py \
    --data-dir data/wp4_dataset/ \
    --action val \
    --weights runs/wp4_keypoints/train_20251211_010508/weights/best.pt
```

### 8.6 Treino com Classes Customizadas

```bash
python src/tools/train_yolo_wp4.py \
    --data-dir data/wp4_dataset/ \
    --classes hotspot terminal junction \
    --epochs 100
```

---

## 9. Modelo Treinado

### 9.1 Localização

```
runs/wp4_keypoints/train_20251211_010508/
├── weights/
│   ├── best.pt          # ← Melhor modelo (usar este)
│   └── last.pt          # Último checkpoint
│
├── args.yaml            # Parâmetros do treino
├── results.png          # Gráficos de métricas
├── confusion_matrix.png
├── confusion_matrix_normalized.png
├── BoxF1_curve.png
├── BoxPR_curve.png
├── BoxP_curve.png
├── BoxR_curve.png
├── labels.jpg           # Distribuição de labels
├── train_batch*.jpg     # Exemplos de treino
└── val_batch*_*.jpg     # Exemplos de validação
```

### 9.2 Métricas do Modelo Atual

**Treino:** `train_20251211_010508`

| Métrica | Valor |
|---------|-------|
| Épocas | 100 |
| mAP50 | ~82.7% |
| Confiança recomendada | 0.15 - 0.30 |

### 9.3 Uso do Modelo

```bash
# No pipeline
python -m src.tools.wp4_wp5_integration_v4 \
    --yolo-model runs/wp4_keypoints/train_20251211_010508/weights/best.pt \
    --yolo-conf 0.15 \
    ...
```

---

## 10. Notas de Implementação

### 10.1 Design Decisions

1. **Execução via subprocess**
   - Usa CLI do Ultralytics (`yolo` command)
   - Permite usar todas as features do YOLO sem importar

2. **Validação antes do treino**
   - Evita treinos longos com dados incorretos
   - Relatório detalhado de problemas

3. **Naming automático**
   - Formato: `train_YYYYMMDD_HHMMSS`
   - Evita sobrescrita acidental

### 10.2 Modelos Base Disponíveis

| Modelo | Tamanho | Speed | Accuracy |
|--------|---------|-------|----------|
| `yolov8n.pt` | 6MB | Mais rápido | Baseline |
| `yolov8s.pt` | 22MB | Rápido | Melhor |
| `yolov8m.pt` | 52MB | Médio | Boa |
| `yolov8l.pt` | 87MB | Lento | Alta |
| `yolov8x.pt` | 131MB | Mais lento | Máxima |

**Recomendação:** `yolov8n.pt` para imagens térmicas (resolução baixa, features simples).

### 10.3 Parâmetros de Treino Recomendados

```bash
--epochs 100      # Suficiente para convergência
--batch 16        # Ajustar conforme VRAM
--imgsz 640       # Padrão YOLO
--model yolov8n.pt  # Leve e eficiente
```

### 10.4 Troubleshooting

| Problema | Solução |
|----------|---------|
| CUDA out of memory | Reduzir `--batch` |
| Treino não converge | Aumentar `--epochs` |
| mAP baixo | Verificar qualidade dos labels |
| YOLO não encontrado | `pip install ultralytics` |

---

## Changelog

### v1.0 (Fevereiro 2025)
- Documentação inicial
- Treino funcional com YOLOv8

---

## Referências

- [Ultralytics YOLOv8 Docs](https://docs.ultralytics.com/)
- [YOLO Training Guide](https://docs.ultralytics.com/modes/train/)
- [Dataset Format](https://docs.ultralytics.com/datasets/detect/)

---

*Documentação gerada para handoff do projeto QE Solar.*
