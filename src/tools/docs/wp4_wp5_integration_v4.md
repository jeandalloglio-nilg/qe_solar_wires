# wp4_wp5_integration_v4.py

## Documentação Técnica

**Módulo:** `src.tools.wp4_wp5_integration_v4`  
**Versão:** v4  
**Autor:** Elton Gino / QE Solar  
**Última atualização:** Fevereiro 2025

---

## Sumário

1. [Descrição](#1-descrição)
2. [Localização e Execução](#2-localização-e-execução)
3. [Dependências](#3-dependências)
4. [Arquitetura do Sistema](#4-arquitetura-do-sistema)
5. [Extração de Keypoints](#5-extração-de-keypoints)
6. [Mapeamento de Coordenadas](#6-mapeamento-de-coordenadas)
7. [Extração de Temperaturas](#7-extração-de-temperaturas)
8. [Clustering via OpenAI](#8-clustering-via-openai)
9. [Detecção de Anomalias](#9-detecção-de-anomalias)
10. [Visualizações](#10-visualizações)
11. [Injeção de Spots (EXIF)](#11-injeção-de-spots-exif)
12. [Argumentos CLI](#12-argumentos-cli)
13. [Fluxo de Execução](#13-fluxo-de-execução)
14. [Formatos de Saída](#14-formatos-de-saída)
15. [Exemplos de Uso](#15-exemplos-de-uso)
16. [Integração com Pipeline](#16-integração-com-pipeline)
17. [Notas de Implementação](#17-notas-de-implementação)

---

## 1. Descrição

### 1.1 Propósito

O `wp4_wp5_integration_v4.py` é o **núcleo de análise** do pipeline QE Solar. Ele integra:

- **WP4:** Detecção de keypoints (via YOLO ou EXIF)
- **WP5:** Clustering semântico (via OpenAI Vision) + Detecção de anomalias térmicas

### 1.2 Funcionalidades Principais

| Funcionalidade | Descrição |
|----------------|-----------|
| **Extração de Keypoints** | YOLO (modelo treinado) ou EXIF (spots existentes) |
| **Mapeamento Geométrico** | Thermal → RGB com alinhamento Real2IR/Offset |
| **Extração de Temperaturas** | Via flirimageextractor ou Planck fallback |
| **Clustering Semântico** | OpenAI Vision para agrupar componentes similares |
| **Detecção de Anomalias** | Hotspots e dead connections por delta intra-cluster |
| **Visualização Dual** | Overlays em thermal E RGB simultaneamente |
| **Injeção EXIF** | Spots detectados compatíveis com FLIR Tools |

### 1.3 Contexto no Pipeline

```
Pipeline QE Solar
      │
      ├── Etapa 0: Extração RGB
      │
      ├── Etapa 1: Clean UI
      │       └── Output: 02_thermal_clean/
      │
      ├── Etapa 2: WP1 Sorting
      │
      ├── Etapa 3: WP2 Thermal Tuning
      │       └── Output: 04_wp2_thermal_tuned/
      │
      ├── Etapa 4: WP4+WP5 Analysis  ◄─── ESTE MÓDULO
      │       │
      │       ├── Input: imagens tuned + RGB + thermal clean
      │       │
      │       └── Output: 05_wp4_wp5_analysis/
      │             ├── thermal/
      │             ├── rgb/
      │             ├── anomalies.json
      │             └── keypoints_used.json
      │
      └── Etapa 5: WP6 Report
```

### 1.4 Modos de Operação

| Modo | Descrição | Uso |
|------|-----------|-----|
| **RAW-only** | Processa imagens FLIR RAW | ✅ Recomendado para produção |
| **Edited** | Processa imagens já editadas | Legacy, compatibilidade |

---

## 2. Localização e Execução

### 2.1 Caminho do Arquivo

```
qe_solar_wires/src/tools/wp4_wp5_integration_v4.py
```

### 2.2 Execução como Módulo

```bash
python -m src.tools.wp4_wp5_integration_v4 [argumentos]
```

### 2.3 Execução Direta

```bash
python src/tools/wp4_wp5_integration_v4.py [argumentos]
```

---

## 3. Dependências

### 3.1 Bibliotecas Python

```python
# Standard Library
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

# Third-party (obrigatórias)
import cv2
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

# Third-party (condicionais)
from PIL import Image                    # Sempre necessário
from dotenv import load_dotenv           # Opcional
from openai import OpenAI                # Para clustering
from ultralytics import YOLO             # Para keypoint-source=yolo
import flirimageextractor                # Para extração de temperaturas
```

### 3.2 Ferramentas Externas

| Ferramenta | Obrigatório | Função |
|------------|-------------|--------|
| `exiftool` | ✅ SIM | Extração de metadados EXIF/FLIR |
| `gcc`/`cc` | ⚠️ Para inject-exif | Compilação do atlas_write_spots |
| Atlas C SDK | ⚠️ Para inject-exif | Headers e libs para injeção |

### 3.3 APIs Externas

| API | Variável de Ambiente | Uso |
|-----|----------------------|-----|
| **OpenAI** | `OPENAI_API_KEY` | Clustering visual via GPT-4o Vision |

### 3.4 Modelo YOLO

Necessário se `--keypoint-source yolo`:

```
models/best.pt
# ou
runs/wp4_keypoints/train_YYYYMMDD_HHMMSS/weights/best.pt
```

### 3.5 Instalação de Dependências

```bash
# Python
pip install numpy opencv-python matplotlib pillow python-dotenv
pip install openai ultralytics flirimageextractor

# Sistema
sudo apt-get install libimage-exiftool-perl build-essential

# Variável de ambiente
export OPENAI_API_KEY="sk-..."
```

---

## 4. Arquitetura do Sistema

### 4.1 Fluxo de Dados

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    WP4+WP5 INTEGRATION PIPELINE                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INPUTS:                                                                    │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐        │
│  │ RAW/Tuned    │ │ RGB Image    │ │ Thermal      │ │ YOLO Model   │        │
│  │ FLIR Image   │ │ (embedded)   │ │ Clean PNG    │ │ (best.pt)    │        │
│  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘ └──────┬───────┘        │
│         │                │                │                │                │
│         ▼                │                ▼                │                │
│  ┌──────────────┐        │         ┌──────────────┐        │                │
│  │ EXIF Extract │        │         │ YOLO Detect  │◄───────┘                │
│  │ (exiftool)   │        │         │ (ultralytics)│                         │
│  └──────┬───────┘        │         └──────┬───────┘                         │
│         │                │                │                                 │
│         ▼                │                ▼                                 │
│  ┌─────────────────────────────────────────────┐                            │
│  │           KEYPOINTS (thermal space)         │                            │
│  │  [{id, x_thermal, y_thermal, ...}, ...]     │                            │
│  └─────────────────────┬───────────────────────┘                            │
│                        │                                                    │
│                        ▼                                                    │
│  ┌─────────────────────────────────────────────┐                            │
│  │        TEMPERATURE EXTRACTION               │                            │
│  │  (flirimageextractor / Planck fallback)     │                            │
│  └─────────────────────┬───────────────────────┘                            │
│                        │                                                    │
│                        ▼                                                    │
│  ┌─────────────────────────────────────────────┐                            │
│  │     COORDINATE MAPPING (thermal → RGB)      │                            │
│  │  Real2IR + OffsetX/Y + Scale                │◄──────────────┐            │
│  └─────────────────────┬───────────────────────┘               │            │
│                        │                                       │            │
│                        ▼                                       │            │
│  ┌─────────────────────────────────────────────┐               │            │
│  │         OPENAI VISION CLUSTERING            │               │            │
│  │  (GPT-4o + JSON Schema)                     │◄──────────────┤            │
│  └─────────────────────┬───────────────────────┘               │            │
│                        │                                       │            │
│                        ▼                                       │            │
│  ┌─────────────────────────────────────────────┐               │            │
│  │         ANOMALY DETECTION                   │               │            │
│  │  (delta intra-cluster > threshold)          │               │            │
│  └─────────────────────┬───────────────────────┘               │            │
│                        │                                       │            │
│         ┌──────────────┼──────────────┐                        │            │
│         │              │              │                        │            │
│         ▼              ▼              ▼                        │            │
│  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐   │            │
│  │ thermal/   │ │ rgb/       │ │ JSONs      │ │ EXIF       │   │            │
│  │ viz        │ │ viz        │ │ outputs    │ │ injection  │   │            │
│  └────────────┘ └────────────┘ └────────────┘ └────────────┘   │            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Componentes Principais

| Componente | Função |
|------------|--------|
| `extract_exif_from_image` | Extrai EXIF via exiftool |
| `extract_keypoints_from_yolo` | Detecta pontos via YOLO |
| `extract_keypoints_from_exif` | Lê spots do EXIF FLIR |
| `transform_thermal_to_rgb` | Mapeamento geométrico |
| `extract_temperatures_for_keypoints` | Temperaturas em cada ponto |
| `call_openai_clusters` | Clustering via GPT-4o Vision |
| `compute_anomalies_within_clusters` | Detecção de anomalias |
| `generate_dual_visualizations` | Overlays thermal + RGB |
| `inject_spots_to_exif` | Injeção via Atlas SDK |

---

## 5. Extração de Keypoints

### 5.1 Fonte: YOLO

```python
def extract_keypoints_from_yolo(
    image_path: Path,           # Imagem para detecção (thermal clean)
    model_path: Path,           # Modelo YOLO (.pt)
    conf_threshold: float,      # Confiança mínima (default: 0.30)
    iou_threshold: float,       # IoU para NMS (default: 0.45)
    device: str,                # "auto", "cpu", "0", "1"...
    verbose: bool,
) -> List[Dict[str, Any]]:
```

**Processo:**

1. Carrega modelo YOLO (`ultralytics.YOLO`)
2. Executa predição na imagem
3. Para cada detecção:
   - Extrai centro do bounding box (x, y)
   - Extrai confiança e classe
4. Ordena por posição (y, x)
5. Atribui IDs sequenciais (1, 2, 3...)

**Retorno:**

```python
[
    {
        "id": 1,
        "label": "Sp1",
        "x": 156.5,          # Centro X na imagem YOLO
        "y": 89.3,           # Centro Y na imagem YOLO
        "x_yolo": 156.5,     # Coordenadas originais YOLO
        "y_yolo": 89.3,
        "conf": 0.87,        # Confiança
        "cls": 0,            # Classe
        "name": "keypoint",  # Nome da classe (se disponível)
        "source": "yolo"
    },
    ...
]
```

### 5.2 Fonte: EXIF

```python
def extract_keypoints_from_exif(exif_item: Dict[str, Any]) -> List[Dict[str, Any]]:
```

**Processo:**

1. Busca tags `FLIR:MeasNParams` no EXIF
2. Filtra por `MeasNType == 1` (spots)
3. Extrai coordenadas X, Y
4. Extrai label e temperatura (se disponível)

**Tags EXIF relevantes:**

| Tag | Descrição |
|-----|-----------|
| `FLIR:Meas1Params` | Coordenadas do spot 1 |
| `FLIR:Meas1Type` | Tipo (1 = spot) |
| `FLIR:Meas1Label` | Label do spot |
| `FLIR:Meas1Value` | Temperatura em °C |

### 5.3 Mapeamento YOLO → Thermal

```python
def map_yolo_keypoints_to_thermal_space(
    yolo_keypoints: List[Dict],
    yolo_size: Tuple[int, int],    # (width, height) da imagem YOLO
    thermal_size: Tuple[int, int], # (width, height) térmica
) -> List[Dict]:
```

**Transformação:**

```
x_thermal = x_yolo × (thermal_width / yolo_width)
y_thermal = y_yolo × (thermal_height / yolo_height)
```

---

## 6. Mapeamento de Coordenadas

### 6.1 Thermal → RGB

```python
def transform_thermal_to_rgb(
    keypoints: List[Dict],
    thermal_size: Tuple[int, int],
    rgb_size: Tuple[int, int],
    real2ir: float,             # Fator de escala FLIR
    offset_x: float,            # Offset horizontal
    offset_y: float,            # Offset vertical
    clip_outside: bool = False, # Remover pontos fora da imagem
) -> List[Dict]:
```

**Algoritmo de transformação:**

```python
# Centros das imagens
th_cx, th_cy = thermal_width / 2, thermal_height / 2
rgb_cx, rgb_cy = rgb_width / 2, rgb_height / 2

# Escala base e efetiva
base_scale = rgb_width / thermal_width
effective_scale = base_scale / real2ir

# Para cada keypoint:
x_off = x_thermal + offset_x
y_off = y_thermal + offset_y

rel_x = x_off - th_cx
rel_y = y_off - th_cy

x_rgb = rgb_cx + rel_x * effective_scale
y_rgb = rgb_cy + rel_y * effective_scale
```

**Parâmetros típicos (FLIR T-series):**

| Parâmetro | Valor típico | Fonte |
|-----------|--------------|-------|
| `real2ir` | 1.74 - 1.76 | EXIF ou CLI |
| `offset_x` | -10 a -24 | EXIF ou CLI |
| `offset_y` | -13 a -24 | EXIF ou CLI |

### 6.2 Extração de Parâmetros do EXIF

```python
def get_alignment_params_from_exif(exif: Dict) -> Dict:
    return {
        "real2ir": exif.get("FLIR:Real2IR", 1.76),
        "offset_x": exif.get("FLIR:OffsetX", -24.0),
        "offset_y": exif.get("FLIR:OffsetY", -24.0),
        "thermal_width": exif.get("FLIR:RawThermalImageWidth", 320),
        "thermal_height": exif.get("FLIR:RawThermalImageHeight", 240),
        "rgb_width": exif.get("FLIR:EmbeddedImageWidth", 1280),
        "rgb_height": exif.get("FLIR:EmbeddedImageHeight", 960),
    }
```

---

## 7. Extração de Temperaturas

### 7.1 Método Principal: flirimageextractor

```python
def extract_temperatures_for_keypoints(
    flir_source_image: Path,    # Imagem FLIR RAW/Tuned
    exif: Dict,                 # EXIF extraído
    keypoints: List[Dict],      # Keypoints com x_thermal, y_thermal
    kernel_size: int = 3,       # Tamanho do kernel para média
) -> Tuple[List[Dict], Dict]:
```

**Processo:**

1. Carrega imagem via `flirimageextractor.FlirImageExtractor`
2. Obtém matriz de temperaturas `get_thermal_np()` em °C
3. Para cada keypoint:
   - Extrai patch `kernel_size × kernel_size` centrado no ponto
   - Calcula média e desvio padrão
4. Retorna keypoints atualizados + sumário

**Retorno:**

```python
# Keypoints atualizados
[
    {
        "id": 1,
        "x_thermal": 156.5,
        "y_thermal": 89.3,
        "temperature": 45.32,      # Média do patch
        "temperature_std": 0.15,   # Desvio padrão
        ...
    }
]

# Sumário
{
    "min_c": 32.5,
    "max_c": 67.8,
    "delta_c": 35.3,
    "count": 48,
    "kernel_size": 3,
    "method": "flirimageextractor"
}
```

### 7.2 Fallback: Cálculo Planck

Se `flirimageextractor` falhar, usa cálculo manual com parâmetros Planck do EXIF:

```python
def _extract_temperatures_planck_fallback(...)
```

**Parâmetros Planck usados:**

| Tag EXIF | Descrição |
|----------|-----------|
| `FLIR:PlanckR1` | Constante R1 |
| `FLIR:PlanckR2` | Constante R2 |
| `FLIR:PlanckB` | Constante B |
| `FLIR:PlanckF` | Constante F |
| `FLIR:PlanckO` | Offset |
| `FLIR:Emissivity` | Emissividade |

**Fórmula:**

```
T_kelvin = B / ln(R1 / (R2 × (RAW + O)) + F)
T_celsius = T_kelvin - 273.15
```

---

## 8. Clustering via OpenAI

### 8.1 Prompt de Sistema

O sistema usa um prompt especializado para análise térmica:

```python
THERMAL_GROUPING_PROMPT = """
Look at the electrical equipment image with circles marking keypoints.

Goal (thermal analysis):
Cluster keypoints into **thermal comparison groups**: each cluster should 
contain keypoints whose temperatures are meaningfully comparable for anomaly 
detection (they should normally run at similar temperature under the same 
operating conditions).

Core idea:
- Clusters are NOT strictly "one physical component instance".
- Clusters are "thermal-equivalence groups" based on function, load path, 
  and surface/target type...
"""
```

### 8.2 JSON Schema

O output é estruturado via JSON Schema:

```python
CLUSTER_SCHEMA = {
    "type": "object",
    "properties": {
        "clusters": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "cluster_id": {"type": "integer"},
                    "component_type": {"type": "string"},
                    "keypoint_ids": {"type": "array", "items": {"type": "integer"}}
                }
            }
        },
        "annotation_spec": {...}
    }
}
```

### 8.3 Chamada à API

```python
def call_openai_clusters(
    image_bgr_with_ids: np.ndarray,  # Imagem com IDs desenhados
    keypoints: List[Dict],
    model: str = "gpt-4o",
    prompt: str = THERMAL_GROUPING_PROMPT,
    temperature: float = 0.1,
) -> Dict:
    from openai import OpenAI
    client = OpenAI()
    
    # Converter imagem para base64
    b64 = _image_to_base64_jpeg(image_bgr_with_ids, max_size=1280)
    
    # Chamada à Responses API com JSON Schema
    resp = client.responses.create(
        model=model,
        input=[{
            "role": "user",
            "content": [
                {"type": "input_text", "text": prompt},
                {"type": "input_image", "image_url": f"data:image/jpeg;base64,{b64}"}
            ]
        }],
        text={
            "format": {
                "type": "json_schema",
                "name": "cluster_result",
                "schema": CLUSTER_SCHEMA,
                "strict": True
            }
        },
        temperature=temperature
    )
    
    return json.loads(resp.output_text)
```

### 8.4 Validação e Correção

```python
def fix_and_validate_clusters(clusters_obj: Dict, keypoints: List[Dict]) -> Dict:
```

**Correções aplicadas:**

1. Remove IDs duplicados
2. Remove IDs inexistentes
3. Adiciona keypoints não atribuídos ao cluster "unassigned"
4. Reindexa cluster_ids sequencialmente
5. Gera legend_entries

---

## 9. Detecção de Anomalias

### 9.1 Algoritmo

```python
def compute_anomalies_within_clusters(
    keypoints: List[Dict],
    clusters_fixed: Dict,
    delta_threshold: float,  # Default: 3.0°C
) -> Dict:
```

**Para cada cluster:**

1. Coleta temperaturas válidas de todos os keypoints
2. Calcula mediana do cluster
3. Para cada keypoint:
   - Calcula delta = `temperatura - mediana`
   - Se `|delta| >= threshold`: marca como anomalia

**Classificação de severidade:**

| Delta | Severidade |
|-------|------------|
| ≥ 2 × threshold (6°C) | **HIGH** |
| ≥ 1.5 × threshold (4.5°C) | **MEDIUM** |
| ≥ threshold (3°C) | **LOW** |

**Tipos de anomalia:**

| Tipo | Condição |
|------|----------|
| `hotspot` | `temperatura > mediana + threshold` |
| `dead_connection` | `temperatura < mediana - threshold` |

### 9.2 Estrutura de Saída

```python
{
    "delta_threshold_c": 3.0,
    "clusters": [
        {
            "cluster_id": 0,
            "component_type": "busbar_connection",
            "keypoint_ids": [1, 2, 3, 4],
            "valid_temperature_points": 4,
            "median_c": 45.5,
            "min_c": 42.1,
            "max_c": 52.3,
            "range_c": 10.2,
            "range_exceeds_threshold": True,
            "anomaly_count": 1,
            "anomaly_keypoint_ids": [4]
        }
    ],
    "anomalies": [
        {
            "keypoint_id": 4,
            "cluster_id": 0,
            "component_type": "busbar_connection",
            "temperature_c": 52.3,
            "cluster_median_c": 45.5,
            "delta_to_median_c": 6.8,
            "anomaly_type": "hotspot",
            "severity": "high"
        }
    ]
}
```

---

## 10. Visualizações

### 10.1 Dual Visualization (thermal + RGB)

```python
def generate_dual_visualizations(
    thermal_clean_bgr: np.ndarray,
    rgb_bgr: np.ndarray,
    keypoints: List[Dict],
    clusters: List[Dict],
    anomalies: List[Dict],
    output_dir: Path,
    prefix: str,
) -> Dict[str, Path]:
```

**Outputs gerados:**

```
output_dir/
├── thermal/
│   ├── prefix_thermal_keypoints.png
│   ├── prefix_thermal_clustered.png
│   ├── prefix_thermal_anomalies.png
│   └── prefix_thermal_side_by_side.png
│
├── rgb/
│   ├── prefix_rgb_keypoints.png
│   ├── prefix_rgb_clustered.png
│   ├── prefix_rgb_anomalies.png
│   └── prefix_rgb_side_by_side.png
│
└── prefix_combined_view.png  (4 painéis)
```

### 10.2 Funções de Desenho

| Função | Descrição |
|--------|-----------|
| `draw_keypoints_simple` | Círculos + labels "SpN" |
| `draw_clusters_simple` | Cores por cluster + "N:CN" |
| `draw_anomalies_simple` | Cores por severidade + temperatura |
| `create_simple_side_by_side` | 3 painéis horizontais |
| `create_combined_4panel` | 2×2 (thermal/rgb × original/clustered) |

### 10.3 Cores de Severidade

| Severidade | Cor (BGR) | Visual |
|------------|-----------|--------|
| LOW | (0, 255, 255) | Amarelo |
| MEDIUM | (0, 165, 255) | Laranja |
| HIGH | (0, 0, 255) | Vermelho |

### 10.4 Side-by-Side com Matplotlib

```python
def render_side_by_side_clusters_and_anomalies(
    base_bgr: np.ndarray,
    keypoints: List[Dict],
    clusters_fixed: Dict,
    anomaly_report: Dict,
    out_path: Path,
    title_left: str,
    title_right: str,
    temp_summary: Optional[Dict],
) -> None:
```

**Layout:**

```
┌─────────────────────────┬─────────────────────────┐
│                         │                         │
│   CLUSTERING RESULTS    │    THERMAL ANOMALIES    │
│                         │                         │
│   - Keypoints coloridos │   - Anomalias marcadas  │
│     por cluster         │   - Info box com stats  │
│   - Legenda de clusters │   - Legenda severidade  │
│                         │                         │
└─────────────────────────┴─────────────────────────┘
```

---

## 11. Injeção de Spots (EXIF)

### 11.1 Propósito

Injetar spots detectados de volta na imagem FLIR para que sejam visíveis e editáveis no **FLIR Tools**.

### 11.2 Limitação do exiftool

O `exiftool` **não consegue** escrever as tags proprietárias `FLIR:MeasNParams`. É necessário usar o **Atlas C SDK**.

### 11.3 Função de Injeção

```python
def inject_spots_to_exif(
    source_raw: Path,           # Imagem FLIR original
    output_path: Path,          # Saída com spots injetados
    keypoints: List[Dict],      # Lista de keypoints
    atlas_include: str,         # Path para includes Atlas
    atlas_libdir: str,          # Path para libs Atlas
    atlas_libs: str,            # Nome das libs
) -> bool:
```

### 11.4 Código C Embarcado

O script contém código C que é compilado dinamicamente:

```c
_ATLAS_WRITE_SPOTS_C = r'''
#include <acs/thermal_image.h>
#include <acs/measurements.h>
#include <acs/measurement_spot.h>

int main(int argc, char** argv) {
    // 1. Abrir imagem FLIR
    ACS_ThermalImage* image = ACS_ThermalImage_alloc();
    ACS_ThermalImage_openFromFile(image, in_path);
    
    // 2. Obter medições
    ACS_Measurements* ms = ACS_ThermalImage_getMeasurements(image);
    
    // 3. Adicionar spots
    for (int i = 0; i < n; ++i) {
        ACS_Measurements_addSpot(ms, x[i], y[i]);
    }
    
    // 4. Salvar
    ACS_ThermalImage_saveAs(image, out_path, ACS_FileFormat_jpeg);
    ACS_ThermalImage_free(image);
}
'''
```

### 11.5 Compilação Automática

```python
def _ensure_atlas_tool(atlas_include, atlas_libdir, atlas_libs) -> Optional[Path]:
    build_dir = Path.home() / ".atlas_build"
    exe_path = build_dir / "atlas_write_spots"
    
    if exe_path.exists():
        return exe_path
    
    # Compilar
    cmd = [
        "gcc", "-o", str(exe_path), str(src_path),
        "-I", atlas_include,
        "-L", atlas_libdir,
        f"-Wl,-rpath,{atlas_libdir}",
        f"-l{atlas_libs}"
    ]
    subprocess.run(cmd, ...)
```

### 11.6 Estrutura de Saída

```
05_wp4_wp5_analysis/
├── <image_name>/
│   ├── thermal/
│   ├── rgb/
│   └── ...
│
└── edited/                    # ← Pasta centralizada
    └── <image_name>_with_spots.jpg
```

---

## 12. Argumentos CLI

### 12.1 Imagens de Entrada

| Argumento | Obrigatório | Descrição |
|-----------|-------------|-----------|
| `--raw-image` | ✅* | Imagem FLIR RAW (modo recomendado) |
| `--edited-image` | ✅* | Imagem FLIR editada (modo legacy) |
| `--rgb-image` | ✅** | Imagem RGB para clustering |
| `--thermal-clean-image` | ✅** | Thermal clean para YOLO |

*Um dos dois é obrigatório  
**Pelo menos um é obrigatório

### 12.2 Configuração de Keypoints

| Argumento | Default | Descrição |
|-----------|---------|-----------|
| `--keypoint-source` | `yolo` | `yolo` ou `exif` |
| `--yolo-model` | - | Caminho do modelo .pt |
| `--yolo-conf` | `0.30` | Confiança mínima |
| `--yolo-iou` | `0.45` | IoU para NMS |
| `--yolo-device` | `auto` | Device (cpu, 0, 1...) |
| `--yolo-image` | - | Imagem para detecção |

### 12.3 Alinhamento

| Argumento | Default | Descrição |
|-----------|---------|-----------|
| `--scale-to-rgb` | False | Mapear thermal → RGB |
| `--real2ir` | EXIF | Override do fator Real2IR |
| `--offset-x` | EXIF | Override do offset X |
| `--offset-y` | EXIF | Override do offset Y |
| `--clip-outside` | False | Remover pontos fora da imagem |

### 12.4 Temperaturas e Anomalias

| Argumento | Default | Descrição |
|-----------|---------|-----------|
| `--extract-temperatures` | False | Extrair temperaturas |
| `--kernel-size` | `3` | Tamanho do kernel |
| `--delta-threshold` | `3.0` | Threshold para anomalia (°C) |

### 12.5 OpenAI

| Argumento | Default | Descrição |
|-----------|---------|-----------|
| `--openai-model` | `gpt-4o` | Modelo de visão |
| `--llm-temperature` | `0.1` | Temperature de sampling |

### 12.6 Output e Visualização

| Argumento | Default | Descrição |
|-----------|---------|-----------|
| `--output` / `-o` | `wp5_results` | Diretório de saída |
| `--output-prefix` | stem da imagem | Prefixo dos arquivos |
| `--dual-viz` | `True` | Gerar viz thermal + RGB |
| `--inject-exif` | False | Injetar spots via Atlas |

### 12.7 Atlas SDK

| Argumento | Default | Descrição |
|-----------|---------|-----------|
| `--atlas-include` | `/home/elton/.../include` | Headers Atlas |
| `--atlas-libdir` | `/home/elton/.../lib` | Libs Atlas |
| `--atlas-libs` | `atlas_c_sdk` | Nome da biblioteca |

---

## 13. Fluxo de Execução

```
main()
  │
  ├── Parsing de argumentos
  │
  ├── Determinar source_image (RAW ou Edited)
  │
  ├── extract_exif_from_image()
  │     └── subprocess: exiftool
  │
  ├── get_alignment_params_from_exif()
  │
  ├── Carregar base_image (RGB ou thermal_clean)
  │
  ├── Extração de keypoints:
  │     ├── YOLO: extract_keypoints_from_yolo()
  │     │         └── ultralytics.YOLO.predict()
  │     └── EXIF: extract_keypoints_from_exif()
  │
  ├── map_yolo_keypoints_to_thermal_space() [se YOLO]
  │
  ├── extract_temperatures_for_keypoints() [se --extract-temperatures]
  │     ├── flirimageextractor.FlirImageExtractor
  │     └── Fallback: _extract_temperatures_planck_fallback()
  │
  ├── transform_thermal_to_rgb() [se --scale-to-rgb]
  │
  ├── Salvar keypoints_used.json
  │
  ├── draw_keypoints_for_llm()
  │
  ├── call_openai_clusters()
  │     └── openai.OpenAI.responses.create()
  │
  ├── fix_and_validate_clusters()
  │
  ├── Salvar clusters.json
  │
  ├── render_cluster_overlay_with_legend()
  │
  ├── compute_anomalies_within_clusters()
  │
  ├── Salvar anomalies.json
  │
  ├── render_side_by_side_clusters_and_anomalies()
  │
  ├── generate_dual_visualizations() [se --dual-viz]
  │
  ├── inject_spots_to_exif() [se --inject-exif]
  │     ├── _ensure_atlas_tool()
  │     │     └── subprocess: gcc (compilação)
  │     └── subprocess: atlas_write_spots
  │
  └── Salvar run_metadata.json
```

---

## 14. Formatos de Saída

### 14.1 Estrutura de Diretórios

```
output_dir/
├── keypoints_used.json
├── <base>_keypoints_used.json
├── <base>_llm_keypoints.png
├── <base>_clusters.json
├── <base>_clustered_overlay.png
├── <base>_analysis_side_by_side.png
├── anomalies.json
├── run_metadata.json
│
├── thermal/
│   ├── prefix_thermal_keypoints.png
│   ├── prefix_thermal_clustered.png
│   ├── prefix_thermal_anomalies.png
│   └── prefix_thermal_side_by_side.png
│
├── rgb/
│   ├── prefix_rgb_keypoints.png
│   ├── prefix_rgb_clustered.png
│   ├── prefix_rgb_anomalies.png
│   └── prefix_rgb_side_by_side.png
│
└── prefix_combined_view.png
```

### 14.2 keypoints_used.json

```json
[
    {
        "id": 1,
        "label": "Sp1",
        "x": 456.7,
        "y": 234.5,
        "x_thermal": 114.2,
        "y_thermal": 58.6,
        "x_yolo": 228.4,
        "y_yolo": 117.2,
        "conf": 0.87,
        "cls": 0,
        "source": "yolo",
        "temperature": 45.32,
        "temperature_std": 0.15
    }
]
```

### 14.3 clusters.json

```json
{
    "clusters": [
        {
            "cluster_id": 0,
            "component_type": "busbar_connection",
            "keypoint_ids": [1, 2, 3, 4]
        },
        {
            "cluster_id": 1,
            "component_type": "fuse_holder",
            "keypoint_ids": [5, 6, 7]
        }
    ],
    "annotation_spec": {
        "draw_cluster_tag_next_to_each_keypoint": true,
        "cluster_tag_format": "C{cluster_id}",
        "legend_box": true,
        "legend_entries": [
            {"cluster_id": 0, "label": "C0: busbar_connection"},
            {"cluster_id": 1, "label": "C1: fuse_holder"}
        ]
    }
}
```

### 14.4 anomalies.json

```json
{
    "delta_threshold_c": 3.0,
    "clusters": [...],
    "anomalies": [
        {
            "keypoint_id": 4,
            "cluster_id": 0,
            "component_type": "busbar_connection",
            "temperature_c": 52.3,
            "cluster_median_c": 45.5,
            "delta_to_median_c": 6.8,
            "anomaly_type": "hotspot",
            "severity": "high"
        }
    ]
}
```

### 14.5 run_metadata.json

```json
{
    "mode": "rgb",
    "raw_mode": true,
    "source_image": "/path/to/raw.jpg",
    "base_image": "/path/to/rgb.jpg",
    "keypoint_source": "yolo",
    "yolo_model": "models/best.pt",
    "temperature_extraction": true,
    "temperature_summary": {
        "min_c": 32.5,
        "max_c": 67.8,
        "delta_c": 35.3,
        "count": 48
    },
    "delta_threshold_c": 3.0,
    "openai_model": "gpt-4o",
    "dual_viz": true,
    "exif_injected": true,
    "timestamp": "2025-02-12 10:30:45",
    "version": "v5"
}
```

---

## 15. Exemplos de Uso

### 15.1 Modo RAW-only (Recomendado para Produção)

```bash
python -m src.tools.wp4_wp5_integration_v4 \
    --raw-image <raw_original> \
    --rgb-image <rgb_image> \
    --thermal-clean-image <clean_image> \
    --keypoint-source yolo \
    --yolo-model <model_path> \
    --yolo-conf 0.15 \
    --yolo-image <clean_image> \
    --extract-temperatures \
    --kernel-size 3 \
    --delta-threshold 3.0 \
    --output <output_dir> \
    --output-prefix <name> \
    --scale-to-rgb \
    --clip-outside \
    --dual-viz \
    --inject-exif
```

### 15.2 Modo Mínimo (apenas clustering)

```bash
python -m src.tools.wp4_wp5_integration_v4 \
    --raw-image "image.jpg" \
    --thermal-clean-image "image_clean.png" \
    --keypoint-source yolo \
    --yolo-model "models/best.pt" \
    --output "output/"
```

### 15.3 Usando Keypoints do EXIF

```bash
python -m src.tools.wp4_wp5_integration_v4 \
    --edited-image "Edited/FLIR3672.jpg" \
    --rgb-image "rgb/FLIR3672_rgb.jpg" \
    --keypoint-source exif \
    --scale-to-rgb \
    --extract-temperatures \
    --output "wp5_exif_test/"
```

---

## 16. Integração com Pipeline

### 16.1 Chamada pelo run_full_pipeline.py

```python
# Em step_4_wp4_wp5_analysis()
cmd = [
    sys.executable, "-m", "src.tools.wp4_wp5_integration_v4",
    "--raw-image", str(raw_path),        # ← RAW original (fonte radiométrica + base para injeção EXIF)
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

# Alignment overrides (se CLI)
if config.real2ir is not None:
    cmd.extend(["--real2ir", str(config.real2ir)])
```

### 16.2 Nota Importante

O `run_full_pipeline.py` **itera** sobre imagens em `04_wp2_thermal_tuned/` (ou a cópia fallback) para manter o naming/agrupamento pós-WP1/WP2, mas o `--raw-image` passado para este script é o **RAW original**. Isso garante:

- extração radiométrica de temperaturas consistente;
- geração do JPG final com spots no EXIF para abrir no FLIR Tools (`*_with_spots.jpg`).

---

## 17. Notas de Implementação

### 17.1 Design Decisions

1. **OpenAI Responses API com JSON Schema**
   - Garante output estruturado
   - Strict mode para validação

2. **Dual visualization separada**
   - Permite análise em thermal E RGB
   - Diferentes use cases (inspeção vs documentação)

3. **Atlas SDK para injeção**
   - exiftool não consegue escrever tags FLIR proprietárias
   - Compilação automática do executável C

4. **Fallback Planck**
   - Se flirimageextractor falhar
   - Cálculo manual com parâmetros EXIF

### 17.2 Limitações Conhecidas

1. **Dependência de API externa**
   - OpenAI API necessária para clustering
   - Custo por imagem (GPT-4o Vision)

2. **Atlas SDK paths hardcoded**
   - Defaults apontam para `/home/elton/...`
   - Deve ser configurado via CLI

3. **Sem cache de clustering**
   - Cada execução chama OpenAI
   - Potencial para cache local

### 17.3 Performance

| Etapa | Tempo típico |
|-------|--------------|
| EXIF extraction | ~0.5s |
| YOLO detection | ~1-3s |
| Temperature extraction | ~1-2s |
| OpenAI clustering | ~5-15s |
| Visualizations | ~2-5s |
| EXIF injection | ~2-5s |
| **Total** | **~15-30s/imagem** |

### 17.4 Tratamento de Erros

| Situação | Comportamento |
|----------|---------------|
| OpenAI API falha | Erro fatal, aborta |
| YOLO sem detecções | Erro fatal, aborta |
| flirimageextractor falha | Fallback Planck |
| Planck falha | Continua sem temperaturas |
| Atlas SDK falha | Warning, continua sem injeção |

### 17.5 Variáveis de Ambiente

```bash
OPENAI_API_KEY=sk-...  # Obrigatório para clustering
```

Suporta `.env` via `python-dotenv`.

---

## Changelog

### v5 (Fevereiro 2025)
- Dual visualization (thermal + RGB)
- Injeção de spots via Atlas SDK
- Responses API com JSON Schema
- Fallback Planck para temperaturas

### v4 (anterior)
- Versão inicial documentada
- Suporte a YOLO e EXIF keypoints

---

## Referências

- [OpenAI Vision API](https://platform.openai.com/docs/guides/vision)
- [Ultralytics YOLO](https://docs.ultralytics.com/)
- [flirimageextractor](https://github.com/nationaldronesau/FlirImageExtractor)
- [FLIR Atlas SDK](https://www.flir.com/products/flir-atlas-sdk/)
- [exiftool FLIR tags](https://exiftool.org/TagNames/FLIR.html)

---

*Documentação gerada para handoff do projeto QE Solar.*
