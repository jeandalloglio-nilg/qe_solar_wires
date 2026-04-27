# run_full_pipeline.py

## Documentação Técnica

**Módulo:** `src.tools.run_full_pipeline`  
**Versão:** 1.0  
**Autor:** Elton Gino / QE Solar  
**Última atualização:** Fevereiro 2025

---

## Sumário

1. [Descrição](#1-descrição)
2. [Localização e Execução](#2-localização-e-execução)
3. [Dependências](#3-dependências)
4. [Estruturas de Dados](#4-estruturas-de-dados)
5. [Funções Utilitárias](#5-funções-utilitárias)
6. [Funções de Pipeline (Steps)](#6-funções-de-pipeline-steps)
7. [Função Principal](#7-função-principal)
8. [Argumentos CLI](#8-argumentos-cli)
9. [Fluxo de Execução](#9-fluxo-de-execução)
10. [Integrações com Outros Módulos](#10-integrações-com-outros-módulos)
11. [Exemplos de Uso](#11-exemplos-de-uso)
12. [Notas de Implementação](#12-notas-de-implementação)

---

## 1. Descrição

### 1.1 Propósito

O `run_full_pipeline.py` é o **orquestrador principal** do sistema de inspeção térmica QE Solar. Ele coordena a execução sequencial de todas as etapas do pipeline, desde a extração de imagens RGB até a geração de relatórios finais.

### 1.2 Modo de Operação

Opera exclusivamente em modo **RAW-Only**, processando imagens FLIR radiométricas originais. Este modo garante:

- Acesso completo aos dados térmicos
- Consistência entre visualização e valores numéricos
- Compatibilidade com FLIR Tools para validação

### 1.3 Responsabilidades

- Validação de inputs e configurações
- Orquestração das 6 etapas do pipeline
- Gerenciamento de mapeamento de nomes (original → sorted)
- Consolidação de resultados
- Geração de logs e métricas

Além disso, o pipeline possui um **modo de exportação minimal** (`--minimal-output`) que executa o processamento em uma pasta de trabalho temporária (`_work/`) e ao final exporta apenas os artefatos finais essenciais (pasta `visible/` e pasta `FLIR/`) + `pipeline_log.json`.

---

## 2. Localização e Execução

### 2.1 Caminho do Arquivo

```
qe_solar_wires/src/tools/run_full_pipeline.py
```

### 2.2 Execução como Módulo

```bash
python -m src.tools.run_full_pipeline [argumentos]
```

### 2.3 Execução Direta

```bash
python src/tools/run_full_pipeline.py [argumentos]
```

### 2.4 Requisito de Diretório

Deve ser executado a partir da raiz do projeto (`qe_solar_wires/`) para que os imports relativos funcionem corretamente.

---

## 3. Dependências

### 3.1 Bibliotecas Python

```python
# Standard Library
import argparse          # Parsing de argumentos CLI
import csv               # Exportação CSV
import json              # Serialização JSON
import os                # Operações de sistema
import re                # Expressões regulares
import shutil            # Cópia de arquivos
import subprocess        # Execução de subprocessos
import sys               # Acesso ao sistema
import time              # Medição de tempo
from dataclasses import dataclass, field, asdict
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

# Opcionais (com fallback)
cv2                      # OpenCV - processamento de imagem
numpy                    # NumPy - arrays numéricos
flirimageextractor       # Extração de dados FLIR
```

### 3.2 Verificação de Dependências Opcionais

```python
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
```

### 3.3 Módulos Internos (Subprocessos)

| Módulo | Etapa | Função |
|--------|-------|--------|
| `src.tools.clean_ui` | Etapa 1 | Limpeza de UI térmica |
| `src.tools.tuning_thermal` | Etapa 3 | Ajuste de range térmico |
| `src.tools.wp4_wp5_integration_v4` | Etapa 4 | Detecção e análise |
| `src.tools.wp6_repx_generator` | Etapa 5 | Geração de relatórios |

### 3.4 Ferramentas Externas

| Ferramenta | Uso | Obrigatório |
|------------|-----|-------------|
| `exiftool` | Extração de EXIF/RGB | Fallback |
| `atlas_scale_set` | Thermal tuning | Opcional |

---

## 4. Estruturas de Dados

### 4.1 PipelineConfig

Dataclass principal de configuração do pipeline.

```python
@dataclass
class PipelineConfig:
    """Configuração do pipeline."""
    
    # === Paths obrigatórios ===
    site_folder: Path           # Pasta raiz do site
    output_folder: Path         # Pasta de saída
    
    # === Flags de controle ===
    skip_clean: bool = False    # Pular limpeza de UI
    skip_wp1: bool = False      # Pular sorting
    skip_wp2: bool = False      # Pular thermal tuning
    skip_wp4_wp5: bool = False  # Pular análise
    skip_wp6: bool = False      # Pular relatório
    
    # === Configuração YOLO ===
    keypoint_source: str = "yolo"   # "yolo" ou "exif"
    yolo_model: Path = None         # Caminho do modelo
    yolo_conf: float = 0.30         # Confiança mínima
    
    # === Configuração WP1 ===
    wp1_model: str = "gpt-4o-mini"  # Modelo LLM (não usado atualmente)
    
    # === Configuração WP2 ===
    wp2_palette: str = "iron"       # Paleta de cores
    atlas_sdk_dir: Optional[Path] = None
    
    # === Configuração WP4+WP5 ===
    openai_model: str = "gpt-4o"    # Modelo para classificação
    delta_threshold: float = 3.0    # Delta mínimo para anomalia (°C)
    kernel_size: int = 3            # Kernel para extração de temperatura
    
    # === Alinhamento Thermal → RGB ===
    real2ir: Optional[float] = None   # Fator de escala
    offset_x: Optional[float] = None  # Deslocamento X
    offset_y: Optional[float] = None  # Deslocamento Y
    scale_to_rgb: bool = True         # Escalar para resolução RGB
    clip_outside: bool = True         # Clipar pontos fora da imagem
    
    # === Subpastas ===
    raw_subdir: str = None            # Nome customizado da pasta RAW
    raw_images_dir: Path = field(default=None)  # Path calculado
```

#### Método `__post_init__`

Executado automaticamente após inicialização:

1. Converte strings para `Path`
2. Auto-detecta pasta RAW via `_find_raw_dir()`
3. Localiza modelo YOLO padrão
4. Localiza Atlas SDK

#### Método `_find_raw_dir()`

Busca pasta de imagens RAW com fallback:

```python
def _find_raw_dir(self) -> Path:
    candidates = ["Raw Images", "Raw", "RAW", "raw_images", "raw"]
    for name in candidates:
        path = self.site_folder / name
        if path.exists() and path.is_dir():
            return path
    return self.site_folder / "Raw Images"  # Default
```

---

### 4.2 ImageResult

Resultado do processamento de uma única imagem.

```python
@dataclass
class ImageResult:
    filename: str                           # Nome original (stem)
    sorted_name: str = ""                   # Nome após sorting
    status: str = "pending"                 # "pending", "success", "error"
    temp_min: Optional[float] = None        # Temperatura mínima
    temp_max: Optional[float] = None        # Temperatura máxima
    keypoint_count: int = 0                 # Keypoints detectados
    cluster_count: int = 0                  # Clusters formados
    anomaly_count: int = 0                  # Anomalias encontradas
    anomalies: List[Dict] = field(default_factory=list)  # Detalhes
    error_message: Optional[str] = None     # Mensagem de erro
```

**Valores de `status`:**
- `"pending"`: Aguardando processamento
- `"success"`: Processado com sucesso
- `"error"`: Falha no processamento

---

### 4.3 PipelineResult

Resultado consolidado do pipeline completo.

```python
@dataclass
class PipelineResult:
    site_name: str                    # Nome do site
    start_time: str                   # ISO timestamp início
    end_time: str = ""                # ISO timestamp fim
    duration_seconds: float = 0.0     # Duração total
    total_images: int = 0             # Total de imagens
    processed_images: int = 0         # Processadas com sucesso
    failed_images: int = 0            # Falhas
    total_anomalies: int = 0          # Total de anomalias
    high_severity: int = 0            # Severidade alta
    medium_severity: int = 0          # Severidade média
    low_severity: int = 0             # Severidade baixa
    image_results: List[ImageResult] = field(default_factory=list)
```

---

## 5. Funções Utilitárias

### 5.1 Constantes

```python
IMG_EXTS = {".jpg", ".jpeg", ".png", ".tif", ".tiff"}
```

### 5.2 `list_images(directory: Path) -> List[Path]`

Lista imagens em um diretório (não recursivo).

```python
def list_images(directory: Path) -> List[Path]:
    if not directory.exists():
        return []
    return sorted([
        p for p in directory.iterdir() 
        if p.is_file() and p.suffix.lower() in IMG_EXTS
    ])
```

**Retorno:** Lista ordenada de Paths

---

### 5.3 `list_images_recursive(directory: Path) -> List[Path]`

Lista imagens recursivamente em subdiretórios.

```python
def list_images_recursive(directory: Path) -> List[Path]:
    if not directory.exists():
        return []
    return sorted([
        p for p in directory.rglob("*") 
        if p.is_file() and p.suffix.lower() in IMG_EXTS
    ])
```

**Uso:** Buscar imagens em estrutura de pastas agrupadas (após WP1).

---

### 5.4 `extract_rgb_from_flir(flir_path: Path, output_path: Path) -> bool`

Extrai imagem RGB embutida de arquivo FLIR.

```python
def extract_rgb_from_flir(flir_path: Path, output_path: Path) -> bool:
    # Método 1: flirimageextractor (preferido)
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
    
    # Método 2: exiftool (fallback)
    try:
        cmd = ["exiftool", "-b", "-EmbeddedImage", str(flir_path)]
        result = subprocess.run(cmd, capture_output=True)
        if result.returncode == 0 and result.stdout:
            output_path.write_bytes(result.stdout)
            return True
    except: pass
    
    return False
```

**Estratégia de fallback:**
1. Tenta `flirimageextractor` (mais confiável)
2. Se falhar, tenta `exiftool` diretamente

---

### 5.5 `sanitize_filename(name: str) -> str`

Remove caracteres inválidos para nomes de arquivo.

```python
def sanitize_filename(name: str) -> str:
    # Remove separadores de path e caracteres especiais
    name = name.replace("/", "_").replace("\\", "_")
    name = name.replace(":", "_").replace(";", "_")
    name = name.replace("\x00", "").replace("\0", "")
    
    # Remove caracteres não-printáveis
    name = "".join(c for c in name if c.isprintable())
    
    return name.strip() or "UNKNOWN"
```

**Uso:** Sanitizar valores EXIF para uso como nomes de pasta.

---

### 5.6 `get_exif_description(img_path: Path) -> str`

Extrai campo ImageDescription do EXIF.

```python
def get_exif_description(img_path: Path) -> str:
    try:
        from PIL import Image
        with Image.open(img_path) as im:
            exif = im.getexif() or {}
            desc = str(exif.get(270, "UNKNOWN")).strip() or "UNKNOWN"
            return sanitize_filename(desc)
    except:
        return "UNKNOWN"
```

**Tag EXIF 270:** ImageDescription (usado pelo FLIR para identificar área/equipamento)

---

### 5.7 `find_tuned_image_for_raw(...) -> Optional[Path]`

Encontra imagem tuned correspondente a uma RAW original.

```python
def find_tuned_image_for_raw(
    raw_stem: str,              # Ex: "IR_54393"
    tuned_dir: Path,            # 04_wp2_thermal_tuned/
    name_mapping: Dict[str, str]  # original_stem → sorted_name
) -> Optional[Path]:
    sorted_name = name_mapping.get(raw_stem, raw_stem)
    
    for img_path in tuned_dir.rglob("*"):
        if not img_path.is_file():
            continue
        if img_path.suffix.lower() not in IMG_EXTS:
            continue
        
        # Verifica se nome contém o stem
        if raw_stem in img_path.stem or sorted_name in img_path.stem:
            return img_path
    
    return None
```

**Contexto:** Após WP1+WP2, as imagens são renomeadas. Esta função faz o mapeamento reverso.

---

## 6. Funções de Pipeline (Steps)

### 6.1 `step_0_extract_rgb(config: PipelineConfig) -> Path`

**Etapa 0: Extração de RGB das imagens RAW.**

```python
def step_0_extract_rgb(config: PipelineConfig) -> Path:
    rgb_dir = config.output_folder / "01_rgb_extracted"
    rgb_dir.mkdir(parents=True, exist_ok=True)
    
    raw_images = list_images(config.raw_images_dir)
    
    for img in raw_images:
        rgb_path = rgb_dir / f"{img.stem}_rgb.jpg"
        if rgb_path.exists():
            continue  # Skip se já existe
        extract_rgb_from_flir(img, rgb_path)
    
    return rgb_dir
```

**Input:** Imagens FLIR RAW  
**Output:** `01_rgb_extracted/<stem>_rgb.jpg`  
**Retorno:** Path do diretório de saída

---

### 6.2 `step_1_clean_thermal(config: PipelineConfig) -> Path`

**Etapa 1: Limpeza de UI das imagens térmicas.**

```python
def step_1_clean_thermal(config: PipelineConfig) -> Path:
    if config.skip_clean:
        return config.raw_images_dir
    
    clean_dir = config.output_folder / "02_thermal_clean"
    
    cmd = [
        sys.executable, "-m", "src.tools.clean_ui",
        "--input", str(config.raw_images_dir),
        "--output", str(clean_dir),
        "--colormap", "inferno",
    ]
    
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    
    if proc.returncode == 0:
        return clean_dir
    else:
        # Fallback: usar RAW sem limpeza
        return config.raw_images_dir
```

**Dependência:** `src.tools.clean_ui`  
**Timeout:** 600 segundos (10 minutos)  
**Fallback:** Retorna pasta RAW original se falhar

---

### 6.3 `step_2_wp1_sorting(config: PipelineConfig) -> Tuple[Path, Dict[str, str]]`

**Etapa 2: WP1 - Sorting por EXIF ImageDescription.**

```python
def step_2_wp1_sorting(config: PipelineConfig) -> Tuple[Path, Dict[str, str]]:
    if config.skip_wp1:
        mapping = {img.stem: img.stem for img in list_images(config.raw_images_dir)}
        return config.raw_images_dir, mapping
    
    output_dir = config.output_folder / "03_wp1_sorted"
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Agrupar por EXIF
    raw_images = list_images(config.raw_images_dir)
    groups = {}
    for img_path in raw_images:
        desc = get_exif_description(img_path)
        groups.setdefault(desc, []).append(img_path)
    
    # Criar nomes sorted
    name_mapping = {}  # original_stem → sorted_name
    global_idx = 1
    
    for group_idx, group_name in enumerate(sorted(groups.keys()), start=1):
        imgs = sorted(groups[group_name], key=lambda p: p.name)
        group_dir = output_dir / group_name
        group_dir.mkdir(parents=True, exist_ok=True)
        
        for position, img_path in enumerate(imgs, start=1):
            sorted_name = f"{global_idx:02d}-{total_count:04d}-{len(imgs):02d}-INSIDE-{group_idx:02d}-{group_name}_{img_path.stem}"
            name_mapping[img_path.stem] = sorted_name
            
            new_path = group_dir / f"{sorted_name}{img_path.suffix}"
            shutil.copy2(img_path, new_path)
            global_idx += 1
    
    return output_dir, name_mapping
```

**Formato do nome sorted:**
```
01-0036-15-INSIDE-01-Inverter1_IR_54393
│   │    │    │    │     │         └── Stem original
│   │    │    │    │     └── Nome do grupo EXIF
│   │    │    │    └── Índice do grupo
│   │    │    └── Posição (hardcoded "INSIDE")
│   │    └── Quantidade de imagens no grupo
│   └── Total de imagens no site
└── Índice global da imagem
```

**Retorno:** Tupla (path_sorted, mapeamento)

---

### 6.4 `step_3_wp2_thermal_tuning(config: PipelineConfig, input_dir: Path) -> Path`

**Etapa 3: WP2 - Thermal Range Tuning.**

```python
def step_3_wp2_thermal_tuning(config: PipelineConfig, input_dir: Path) -> Path:
    if config.skip_wp2:
        return input_dir
    
    output_dir = config.output_folder / "04_wp2_thermal_tuned"
    
    # Verificar Atlas SDK
    atlas_cli = None
    if config.atlas_sdk_dir:
        atlas_cli = config.atlas_sdk_dir / "build" / "atlas_scale_set"
    
    if atlas_cli and atlas_cli.exists():
        cmd = [
            sys.executable, "-m", "src.tools.tuning_thermal",
            "--input", str(input_dir),
            "--output", str(output_dir),
            "--atlas-sdk-dir", str(config.atlas_sdk_dir),
            "--palette", config.wp2_palette,
        ]
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
        
        if proc.returncode == 0:
            return output_dir
    
    # Fallback: copiar sem tuning
    output_dir.mkdir(parents=True, exist_ok=True)
    for img in list_images_recursive(input_dir):
        rel_path = img.relative_to(input_dir)
        dest = output_dir / rel_path
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(img, dest)
    
    return output_dir
```

**Dependência:** `src.tools.tuning_thermal` + Atlas SDK  
**Fallback:** Copia imagens sem modificação

---

### 6.5 `step_4_wp4_wp5_analysis(...) -> Tuple[Path, List[ImageResult]]`

**Etapa 4: WP4+WP5 - YOLO Keypoints + Clustering + Anomalias.**

Esta é a etapa mais complexa do pipeline.

```python
def step_4_wp4_wp5_analysis(
    config: PipelineConfig,
    rgb_dir: Path,              # 01_rgb_extracted/
    clean_dir: Path,            # 02_thermal_clean/
    tuned_dir: Path,            # 04_wp2_thermal_tuned/
    name_mapping: Dict[str, str],
) -> Tuple[Path, List[ImageResult]]:
```

**Fluxo interno:**

1. **Listar imagens tuned** (não RAW originais)
2. **Para cada imagem:**
   - Extrair `original_stem` do nome sorted
   - Localizar RGB correspondente
   - Localizar thermal clean correspondente
   - Montar comando `wp4_wp5_integration_v4`
   - Executar como subprocesso
   - Parsear resultados (anomalies.json, keypoints_used.json)
   - Criar `ImageResult`

**Comando montado (comportamento atual):**
```python
cmd = [
    sys.executable, "-m", "src.tools.wp4_wp5_integration_v4",
    "--raw-image", str(raw_path),        # ← RAW original (fonte radiométrica + injeção EXIF)
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
```

**Notas importantes (RAW vs Tuned):**

- O `run_full_pipeline.py` **itera** sobre os arquivos de `04_wp2_thermal_tuned/` (ou a cópia fallback, caso WP2 seja pulado/falhe), pois é essa pasta que representa o estado pós-WP1/WP2 e define o naming final por grupo.
- Porém, o `--raw-image` passado para `wp4_wp5_integration_v4` é o **RAW original**, porque:
  - é a fonte radiométrica confiável para extração de temperaturas;
  - é o arquivo correto para gerar o JPG final compatível com FLIR Tools com spots injetados.

**Timeout:** 300 segundos por imagem

---

### 6.6 `step_5_wp6_report(...) -> Path`

**Etapa 5: WP6 - Relatório REPX.**

```python
def step_5_wp6_report(
    config: PipelineConfig,
    analysis_dir: Path,
    results: List[ImageResult],
    pipeline_result: PipelineResult
) -> Path:
    if config.skip_wp6:
        return config.output_folder / "06_report"
    
    report_dir = config.output_folder / "06_report"
    
    # Chamar gerador REPX
    cmd = [
        sys.executable, "-m", "src.tools.wp6_repx_generator",
        "--edited-dir", str(analysis_dir),
        "--output", str(report_dir / "report.repx"),
        "--site-name", pipeline_result.site_name,
    ]
    
    # Adicionar logo se existir
    # ...
    
    subprocess.run(cmd, capture_output=True, text=True, timeout=300)
    
    # Gerar summary JSON
    # Gerar CSV de anomalias
    # ...
    
    return report_dir
```

**Outputs gerados:**
- `report.repx`: Relatório FLIR
- `site_summary.json`: Resumo do site
- `anomalies_report.csv`: Lista de anomalias

---

## 7. Função Principal

### 7.1 `run_pipeline(config: PipelineConfig) -> PipelineResult`

Função que executa o pipeline completo.

```python
def run_pipeline(config: PipelineConfig) -> PipelineResult:
    start = time.time()
    site_name = config.site_folder.name
    
    result = PipelineResult(
        site_name=site_name,
        start_time=datetime.now().isoformat(),
    )
    
    # Validação
    if not config.raw_images_dir.exists():
        print(f"❌ RAW Images não encontrado")
        return result
    
    config.output_folder.mkdir(parents=True, exist_ok=True)
    
    # Execução sequencial
    rgb_dir = step_0_extract_rgb(config)
    clean_dir = step_1_clean_thermal(config)
    sorted_dir, name_mapping = step_2_wp1_sorting(config)
    tuned_dir = step_3_wp2_thermal_tuning(config, sorted_dir)
    analysis_dir, image_results = step_4_wp4_wp5_analysis(
        config, rgb_dir, clean_dir, tuned_dir, name_mapping
    )
    report_dir = step_5_wp6_report(config, analysis_dir, image_results, result)
    
    # Consolidar resultados
    result.total_images = len(image_results)
    result.processed_images = sum(1 for r in image_results if r.status == "success")
    result.failed_images = sum(1 for r in image_results if r.status == "error")
    result.image_results = image_results
    result.end_time = datetime.now().isoformat()
    result.duration_seconds = round(time.time() - start, 1)
    
    # Salvar log
    log_path = config.output_folder / "pipeline_log.json"
    log_path.write_text(json.dumps(asdict(result), indent=2, default=str))
    
    return result
```

### 7.2 `main()`

Entry point com parsing de argumentos.

```python
def main():
    parser = argparse.ArgumentParser(...)
    # ... definição de argumentos ...
    args = parser.parse_args()
    
    config = PipelineConfig(
        site_folder=args.site_folder,
        output_folder=args.output,
        # ... mapeamento de argumentos ...
    )
    
    run_pipeline(config)

if __name__ == "__main__":
    main()
```

---

## 8. Argumentos CLI

### 8.1 Argumentos Obrigatórios

| Argumento | Tipo | Descrição |
|-----------|------|-----------|
| `--site-folder` | str | Pasta do site com RAW Images |
| `--output` / `-o` | str | Pasta de output |

### 8.2 Configuração de Subpasta

| Argumento | Default | Descrição |
|-----------|---------|-----------|
| `--raw-subdir` | Auto | Nome da subpasta RAW |

### 8.3 Configuração YOLO

| Argumento | Default | Descrição |
|-----------|---------|-----------|
| `--keypoint-source` | `"yolo"` | Fonte: `yolo` ou `exif` |
| `--yolo-model` | `models/best.pt` | Caminho do modelo |
| `--yolo-conf` | `0.30` | Confiança mínima |

### 8.4 Flags de Skip

| Flag | Descrição |
|------|-----------|
| `--skip-clean` | Pular etapa 1 |
| `--skip-wp1` | Pular etapa 2 |
| `--skip-wp2` | Pular etapa 3 |
| `--skip-wp4-wp5` | Pular etapa 4 |
| `--skip-wp6` | Pular etapa 5 |

### 8.5 Alinhamento Manual

| Argumento | Default | Descrição |
|-----------|---------|-----------|
| `--real2ir` | Auto | Fator de escala |
| `--offset-x` | Auto | Offset X (pixels) |
| `--offset-y` | Auto | Offset Y (pixels) |

### 8.6 Outros

| Argumento | Default | Descrição |
|-----------|---------|-----------|
| `--atlas-sdk-dir` | Auto | Diretório Atlas SDK |
| `--openai-model` | `"gpt-4o"` | Modelo OpenAI |
| `--delta-threshold` | `3.0` | Threshold anomalia (°C) |
| `--minimal-output` | `False` | Executa o pipeline, mas exporta apenas `visible/`, `FLIR/` e `pipeline_log.json` |
| `--force-clean-output` | `False` | No modo `--minimal-output`, permite apagar conteúdo existente do output |
| `--retry-errors-from` | `None` | Reprocessa apenas imagens com `status=error` de um `pipeline_log.json` anterior |

---

## 9. Fluxo de Execução

```
main()
  │
  ├── Parsing de argumentos (argparse)
  │
  ├── Criação de PipelineConfig
  │     ├── __post_init__()
  │     ├── _find_raw_dir()
  │     └── Auto-detecção de YOLO e Atlas
  │
  └── run_pipeline(config)
        │
        ├── Validação de inputs
        │
        ├── step_0_extract_rgb()
        │     └── extract_rgb_from_flir() [para cada imagem]
        │
        ├── step_1_clean_thermal()
        │     └── subprocess: src.tools.clean_ui
        │
        ├── step_2_wp1_sorting()
        │     ├── get_exif_description() [para cada imagem]
        │     └── Agrupamento e renomeação
        │
        ├── step_3_wp2_thermal_tuning()
        │     └── subprocess: src.tools.tuning_thermal
        │
        ├── step_4_wp4_wp5_analysis()
        │     └── subprocess: src.tools.wp4_wp5_integration_v4 [para cada imagem]
        │
        ├── step_5_wp6_report()
        │     └── subprocess: src.tools.wp6_repx_generator
        │
        ├── Consolidação de resultados
        │
        └── Salvar pipeline_log.json

### 9.1 Fluxo no modo `--minimal-output`

Quando `--minimal-output` está ativo:

1. O pipeline valida se o diretório `--output` está vazio (ou exige `--force-clean-output`).
2. Cria uma pasta de trabalho: `--output/_work/`.
3. Executa o pipeline “normal” dentro de `_work/`, porém com:
   - WP2 **desativado** (`skip_wp2=True`) para manter o range térmico original.
   - WP6 **desativado** (`skip_wp6=True`) porque o objetivo é exportar apenas imagens finais.
4. Exporta artefatos finais para o `--output`:
   - `visible/`: PNGs de visualização (keypoints/anomalias/side-by-side), ou fallback para o `*_clean.png` quando não há keypoints.
   - `FLIR/`: JPGs para FLIR Tools:
     - quando há keypoints/injeção EXIF: `*_with_spots.jpg` vindo de `05_wp4_wp5_analysis/<sorted_name>/thermal/FLIR_keypoints/`.
     - fallback quando não há keypoints: copia o RAW original.
   - `pipeline_log.json`: cópia do log consolidado.
5. Apaga `_work/`.
```

---

## 10. Integrações com Outros Módulos

### 10.1 clean_ui.py

**Chamada:**
```bash
python -m src.tools.clean_ui \
    --input <raw_dir> \
    --output <clean_dir> \
    --colormap inferno
```

**Responsabilidade:** Remover overlays de UI das imagens FLIR

---

### 10.2 tuning_thermal.py

**Chamada:**
```bash
python -m src.tools.tuning_thermal \
    --input <sorted_dir> \
    --output <tuned_dir> \
    --atlas-sdk-dir <atlas_dir> \
    --palette iron
```

**Responsabilidade:** Ajustar scale range térmico (P5-P95)

---

### 10.3 wp4_wp5_integration_v4.py

**Chamada:**
```bash
python -m src.tools.wp4_wp5_integration_v4 \
    --raw-image <tuned_image> \
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

**Responsabilidade:** Detecção de keypoints, clustering, análise de anomalias

---

### 10.4 wp6_repx_generator.py

**Chamada:**
```bash
python -m src.tools.wp6_repx_generator \
    --edited-dir <edited_images_dir> \
    --output <report.repx> \
    --site-name <site_name> \
    --logo <logo_path>
```

**Responsabilidade:** Geração de relatório REPX

---

## 11. Exemplos de Uso

### 11.1 Execução Básica

```bash
cd ~/qe_solar_wires
source .venv/bin/activate

python -m src.tools.run_full_pipeline \
    --site-folder "data/Cliente/Site/2025" \
    --output "output/Site_2025"
```

### 11.2 Execução Completa com Todos os Parâmetros

```bash
python -m src.tools.run_full_pipeline \
    --site-folder "data/Syncarpha/Syncarpha/Syncarpha/Dodge 1/2025" \
    --output "output/Dodge_2025" \
    --atlas-sdk-dir "atlas_sdk" \
    --yolo-model "runs/wp4_keypoints/train_20251211_010508/weights/best.pt" \
    --yolo-conf 0.15 \
    --real2ir 1.75 \
    --offset-x -10 \
    --offset-y -13 \
    --delta-threshold 3.0 \
    --openai-model "gpt-4o"

### 11.2.1 Execução no modo minimal (export final: `visible/` + `FLIR/`)

```bash
python -m src.tools.run_full_pipeline \
  --site-folder "data/Syncarpha/Syncarpha/Syncarpha/Dodge 1/2025" \
  --output "output/Dodge_2025_min" \
  --minimal-output \
  --force-clean-output \
  --yolo-model "runs/wp4_keypoints/train_20251211_010508/weights/best.pt" \
  --yolo-conf 0.15 \
  --real2ir 1.75 \
  --offset-x -10 \
  --offset-y -13 \
  --delta-threshold 3.0 \
  --openai-model "gpt-4o"
```

### 11.3 Reprocessamento Parcial

```bash
# Apenas análise (pular preparação)
python -m src.tools.run_full_pipeline \
    --site-folder "data/Site/2025" \
    --output "output/Site_reprocess" \
    --skip-clean \
    --skip-wp1 \
    --skip-wp2
```

### 11.3.1 Retry automático de erros a partir de um log anterior

```bash
python -m src.tools.run_full_pipeline \
  --site-folder "data/Site/2025" \
  --output "output/Site_retry" \
  --minimal-output \
  --force-clean-output \
  --retry-errors-from "output/Site/pipeline_log.json"
```

### 11.4 Apenas Organização (sem análise)

```bash
python -m src.tools.run_full_pipeline \
    --site-folder "data/Site/2025" \
    --output "output/Site_organized" \
    --skip-wp4-wp5 \
    --skip-wp6
```

---

## 12. Notas de Implementação

### 12.1 Design Decisions

1. **Subprocessos vs Imports Diretos**
   - Módulos são chamados como subprocessos (`subprocess.run`)
   - Permite isolamento de erros e timeouts
   - Trade-off: overhead de processo vs robustez

2. **Mapeamento de Nomes**
   - `name_mapping: Dict[str, str]` mantém relação original → sorted
   - Essencial para rastrear imagens através das etapas

3. **Uso de Imagens Tuned no WP4+WP5**
   - Decisão crítica: WP4+WP5 usa imagens TUNED, não RAW
   - Garante consistência com FLIR Tools (mesmo scale range)

### 12.2 Limitações Conhecidas

1. **Posição hardcoded "INSIDE"**
   - O campo de posição no nome sorted é sempre "INSIDE"
   - Futuro: extrair de metadados ou CLI

2. **Timeout fixo**
   - 300s para wp4_wp5, 600s para clean/tuning
   - Pode ser insuficiente para sites muito grandes

3. **Sem paralelização**
   - Processamento sequencial de imagens
   - Potencial para threading/multiprocessing

### 12.3 Tratamento de Erros

- **Fallbacks implementados:**
  - RGB extraction: flirimageextractor → exiftool
  - Thermal tuning: Atlas SDK → cópia simples
  - Clean thermal: clean_ui → usar RAW

- **Erros não-fatais:**
  - Falha em uma imagem não interrompe pipeline
  - Status "error" registrado em `ImageResult`

### 12.4 Logs e Debug

- **Console:** Progresso em tempo real com emojis
- **pipeline_log.json:** Resultado completo serializado
- **Outputs intermediários:** Cada etapa gera pasta separada

---

## Changelog

### v1.0 (Fevereiro 2025)
- Documentação inicial
- Modo RAW-only estável
- Integração WP1-WP6 completa

---

*Documentação gerada para handoff do projeto QE Solar.*
