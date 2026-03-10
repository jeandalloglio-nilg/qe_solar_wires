# tuning_thermal.py

## Documentação Técnica

**Módulo:** `src.tools.tuning_thermal`  
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
6. [Funções de Processamento](#6-funções-de-processamento)
7. [Função Principal](#7-função-principal)
8. [Argumentos CLI](#8-argumentos-cli)
9. [Fluxo de Execução](#9-fluxo-de-execução)
10. [Formatos de Saída](#10-formatos-de-saída)
11. [Atlas SDK](#11-atlas-sdk)
12. [Exemplos de Uso](#12-exemplos-de-uso)
13. [Integração com Pipeline](#13-integração-com-pipeline)
14. [Notas de Implementação](#14-notas-de-implementação)

---

## 1. Descrição

### 1.1 Propósito

O `tuning_thermal.py` implementa o **WP2 (Work Package 2)** do pipeline QE Solar: ajuste de escala térmica por grupo de equipamentos. Ele normaliza o range de temperatura das imagens FLIR usando percentis (P5-P95), garantindo visualização consistente dentro de cada grupo de equipamentos.

### 1.2 Funcionalidades Principais

| Funcionalidade | Descrição |
|----------------|-----------|
| **Agrupamento por EXIF** | Agrupa imagens por tipo de equipamento (CB, INV, XMFR) |
| **Cálculo de Window** | Computa P5-P95 de todas as temperaturas do grupo |
| **Aplicação de Scale** | Usa Atlas SDK C para aplicar novo range térmico |
| **Preservação de Metadados** | Mantém EXIF original via exiftool |
| **Geração de Relatórios** | JSON e CSV com estatísticas por grupo |

### 1.3 Contexto no Pipeline

```
Pipeline QE Solar
      │
      ├── Etapa 0: Extração RGB
      │
      ├── Etapa 1: Clean UI
      │
      ├── Etapa 2: WP1 Sorting
      │       │
      │       └── Output: 03_wp1_sorted/<Group>/<images>
      │
      ├── Etapa 3: WP2 Thermal Tuning  ◄─── ESTE MÓDULO
      │       │
      │       └── Output: 04_wp2_thermal_tuned/<Group>/<images>
      │
      └── Etapa 4: WP4+WP5 Analysis
```

### 1.4 Conceito de Tuning

O problema que este módulo resolve:

```
ANTES (imagens individuais):
┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│ Img1: 20-80°C│ │ Img2: 25-65°C│ │ Img3: 30-90°C│
└─────────────┘ └─────────────┘ └─────────────┘
     ↓ Visualmente inconsistentes (cores diferentes para mesma temp)

DEPOIS (tuning por grupo):
┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│ Img1: 22-85°C│ │ Img2: 22-85°C│ │ Img3: 22-85°C│
└─────────────┘ └─────────────┘ └─────────────┘
     ↓ Visualmente consistentes (mesma cor = mesma temperatura)
     
Range calculado: P5=22°C, P95=85°C (de TODAS as imagens do grupo)
```

---

## 2. Localização e Execução

### 2.1 Caminho do Arquivo

```
qe_solar_wires/src/tools/tuning_thermal.py
```

### 2.2 Execução como Módulo

```bash
python -m src.tools.tuning_thermal \
    --input <dir> \
    --output <dir> \
    --atlas-sdk-dir <atlas_dir>
```

### 2.3 Execução Direta

```bash
python src/tools/tuning_thermal.py \
    --input <dir> \
    --output <dir> \
    --atlas-sdk-dir <atlas_dir>
```

---

## 3. Dependências

### 3.1 Bibliotecas Python

```python
# Standard Library
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

# Third-party (obrigatórias)
import numpy as np
from PIL import Image, ExifTags
from tqdm import tqdm

# Third-party (opcionais com fallback)
import piexif                    # Fallback para EXIF se exiftool indisponível
from flirimageextractor import FlirImageExtractor
```

### 3.2 Ferramentas Externas (CRÍTICAS)

| Ferramenta | Obrigatório | Função |
|------------|-------------|--------|
| `atlas_scale_set` | ✅ **SIM** | Aplica scale range nas imagens FLIR |
| `exiftool` | ⚠️ Recomendado | Copia metadados EXIF completos |
| `piexif` | ❌ Fallback | Copia EXIF básico se exiftool indisponível |

### 3.3 Atlas SDK

O Atlas SDK é um projeto C que deve ser compilado separadamente:

```bash
# Estrutura esperada
atlas_sdk/
├── src/
│   └── atlas_scale_set.c
├── build/
│   └── atlas_scale_set      # ← Executável compilado
└── Makefile

# Compilação
cd atlas_sdk
mkdir build
make
# ou
cmake .. && make
```

**Verificação:**
```bash
./atlas_sdk/build/atlas_scale_set --help
```

### 3.4 Instalação de Dependências

```bash
# Python
pip install numpy Pillow tqdm flirimageextractor piexif

# Sistema (Ubuntu/Debian)
sudo apt-get install libimage-exiftool-perl

# Atlas SDK (compilar manualmente)
cd atlas_sdk && make
```

---

## 4. Estruturas de Dados

### 4.1 ArgsNS (Dataclass)

Namespace tipado para argumentos CLI.

```python
@dataclass
class ArgsNS:
    input: Path           # Diretório de entrada
    output: Path          # Diretório de saída
    atlas_sdk_dir: Path   # Diretório do Atlas SDK
    palette: str          # Paleta de cores (iron, rainbow, etc.)
    p_low: float          # Percentil inferior (default: 5.0)
    p_high: float         # Percentil superior (default: 95.0)
    hist_clip: float      # Clipping de histograma (default: 0.0)
    strict: bool          # Modo estrito (falhar em erros)
```

### 4.2 Constantes

```python
SUPPORTED_EXT = {".jpg", ".jpeg", ".png", ".tif", ".tiff"}
EXIF_TAGS = {v: k for k, v in ExifTags.TAGS.items()}  # Nome → ID
INVALID_FS = r'<>:"/\|?*'  # Caracteres inválidos para filesystem
```

---

## 5. Funções Utilitárias

### 5.1 `safe_dirname(name: str) -> str`

Sanitiza string para uso como nome de diretório.

```python
def safe_dirname(name: str) -> str:
    s = (name or "UNKNOWN").strip()
    for ch in INVALID_FS:
        s = s.replace(ch, "_")
    return s or "UNKNOWN"
```

### 5.2 `has_exiftool() -> bool`

Verifica se exiftool está disponível no PATH.

```python
def has_exiftool() -> bool:
    return shutil.which("exiftool") is not None
```

### 5.3 `run(cmd: List[str], check: bool = True) -> CompletedProcess`

Wrapper para subprocess.run com captura de output.

```python
def run(cmd: List[str], check: bool = True) -> subprocess.CompletedProcess:
    return subprocess.run(
        cmd, 
        check=check, 
        stdout=subprocess.PIPE, 
        stderr=subprocess.STDOUT, 
        text=True
    )
```

### 5.4 `discover_images(root: Path) -> List[Path]`

Lista todas as imagens recursivamente.

```python
def discover_images(root: Path) -> List[Path]:
    return [
        p for p in root.rglob("*") 
        if p.is_file() and p.suffix.lower() in SUPPORTED_EXT
    ]
```

### 5.5 `get_imagedescription(p: Path) -> str`

Extrai campo ImageDescription do EXIF.

```python
def get_imagedescription(p: Path) -> str:
    try:
        with Image.open(p) as im:
            exif = im.getexif() or {}
            tag = EXIF_TAGS.get("ImageDescription", -1)
            v = exif.get(tag)
            # ... tratamento de bytes/string ...
            return v.strip().replace("/", "_").replace("\\", "_")
    except Exception:
        pass
    return "UNKNOWN"
```

### 5.6 `normalize_group_id(desc: str) -> str`

Normaliza descrição EXIF para tipo de equipamento.

```python
def normalize_group_id(desc: str) -> str:
    """
    Extrai tipo de equipamento da ImageDescription.
    
    Exemplos:
        "CB-02" → "CB"
        "INVERTER-3" → "INV"
        "XMFR01" → "XMFR"
    """
    # Extrai prefixo alfabético
    m = re.match(r"^([A-Za-z]+)", desc.strip())
    if not m:
        return "UNKNOWN"
    
    prefix = m.group(1).upper()
    
    # Aliases para normalização
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
```

**Mapeamento de aliases:**

| Input | Output |
|-------|--------|
| `INVERTER`, `INV` | `INV` |
| `TRANSFORMER`, `XFMR`, `XMFR` | `XMFR` |
| `BREAKER`, `CB` | `CB` |
| Outros | Mantém original |

### 5.7 `copy_preserve(src: Path, dst: Path) -> None`

Copia arquivo preservando metadados de tempo.

```python
def copy_preserve(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)  # copy2 preserva timestamps
```

### 5.8 `unique_dest(folder: Path, filename: str) -> Path`

Gera path único, tratando colisões de nome.

```python
def unique_dest(folder: Path, filename: str) -> Path:
    """
    Gera path único para evitar sobrescrita.
    
    Exemplo:
        image.jpg existe → image_001.jpg
        image_001.jpg existe → image_002.jpg
    """
    base = Path(filename).stem
    ext = Path(filename).suffix
    cand = folder / (base + ext)
    
    if not cand.exists():
        return cand
    
    for i in range(1, 10000):
        cand = folder / f"{base}_{i:03d}{ext}"
        if not cand.exists():
            return cand
    
    raise RuntimeError(f"Too many collisions for {filename}")
```

### 5.9 `cleanup_empty_dirs(root: Path) -> int`

Remove diretórios vazios recursivamente.

```python
def cleanup_empty_dirs(root: Path) -> int:
    removed = 0
    # Ordenar reverso para processar folhas primeiro
    for d in sorted([p for p in root.rglob("*") if p.is_dir()], reverse=True):
        try:
            if d.exists() and not any(d.iterdir()):
                d.rmdir()
                removed += 1
        except Exception:
            pass
    return removed
```

---

## 6. Funções de Processamento

### 6.1 `copy_all_metadata(src: Path, dst: Path) -> None`

Copia todos os metadados EXIF via exiftool.

```python
def copy_all_metadata(src: Path, dst: Path) -> None:
    if not has_exiftool():
        raise RuntimeError("exiftool not found")
    
    cmd = [
        "exiftool",
        "-overwrite_original",
        "-P",                      # Preservar timestamps
        "-TagsFromFile", str(src),
        "-all:all",                # Copiar TODOS os tags
        str(dst),
    ]
    
    cp = run(cmd, check=False)
    if cp.returncode != 0:
        raise RuntimeError(f"exiftool failed: {cp.stdout}")
```

**Flags do exiftool:**
- `-overwrite_original`: Não criar backup `_original`
- `-P`: Preservar timestamps do arquivo
- `-TagsFromFile`: Fonte dos metadados
- `-all:all`: Copiar todos os grupos de tags

### 6.2 `copy_basic_exif_python(src: Path, dst: Path) -> None`

Fallback: copia EXIF básico via piexif (Python puro).

```python
def copy_basic_exif_python(src: Path, dst: Path) -> None:
    if piexif is None:
        print("[warn] piexif not installed; metadata may be lost.")
        return
    
    try:
        src_exif = piexif.load(str(src))
        piexif.insert(piexif.dump(src_exif), str(dst))
    except Exception as e:
        print(f"[warn] fallback EXIF failed: {e}")
```

**Limitação:** piexif copia apenas EXIF padrão, não metadados FLIR customizados.

### 6.3 `thermal_celsius(p: Path) -> np.ndarray`

Extrai matriz de temperaturas em Celsius.

```python
def thermal_celsius(p: Path) -> np.ndarray:
    if FlirImageExtractor is None:
        raise RuntimeError("flirimageextractor not installed")
    
    try:
        fie = FlirImageExtractor(is_debug=False)
    except TypeError:
        fie = FlirImageExtractor()
    
    fie.process_image(str(p))
    arr = fie.get_thermal_np()
    
    if arr is None:
        raise RuntimeError(f"not radiometric: {p.name}")
    
    return np.asarray(arr, dtype=np.float32)
```

**Retorno:**
- Shape: `(height, width)`
- Dtype: `float32`
- Valores: Temperaturas em °C

### 6.4 `robust_window_group(...) -> Tuple[float, float, Set[Path]]`

Calcula janela térmica robusta para um grupo de imagens.

```python
def robust_window_group(
    files: List[Path], 
    p_low: float,      # Percentil inferior (ex: 5.0)
    p_high: float,     # Percentil superior (ex: 95.0)
    hist_clip: float   # Clipping de histograma (ex: 0.0)
) -> Tuple[float, float, Set[Path]]:
    """
    Calcula window P5-P95 de todas as temperaturas do grupo.
    
    Returns:
        (tmin, tmax, set_de_arquivos_radiometricos)
    """
```

**Algoritmo:**

```
1. Para cada arquivo:
   ├── Tentar extrair matriz térmica
   ├── Se sucesso: adicionar valores ao array global
   └── Se falha: ignorar (não é radiométrico)
         │
         ▼
2. Concatenar todos os valores
         │
         ▼
3. Aplicar hist_clip (se > 0):
   ├── lo = percentile(hist_clip)
   ├── hi = percentile(100 - hist_clip)
   └── Filtrar valores fora de [lo, hi]
         │
         ▼
4. Calcular percentis finais:
   ├── tmin = percentile(p_low)
   └── tmax = percentile(p_high)
         │
         ▼
5. Validação:
   ├── Se tmax <= tmin ou range < 2°C
   └── Usar mediana ± 1°C como fallback
         │
         ▼
6. Retornar (tmin, tmax, radiometric_set)
```

**Exemplo numérico:**

```
Grupo com 3 imagens:
  Img1: temperaturas 20-60°C
  Img2: temperaturas 25-55°C  
  Img3: temperaturas 22-70°C

Todas concatenadas: [20, 21, ..., 70] (milhões de valores)

P5 = 22°C, P95 = 65°C

Todas as imagens do grupo usarão range [22, 65]°C
```

### 6.5 `atlas_cli_path(atlas_sdk_dir: Path) -> Path`

Localiza executável do Atlas SDK.

```python
def atlas_cli_path(atlas_sdk_dir: Path) -> Path:
    cli = atlas_sdk_dir / "build" / "atlas_scale_set"
    
    if not cli.exists():
        raise RuntimeError(
            f"atlas_scale_set not found at: {cli}\n"
            f"Build the atlas_sdk project first."
        )
    
    return cli
```

### 6.6 `apply_scale_with_cli(...) -> None`

Aplica scale range usando Atlas SDK CLI.

```python
def apply_scale_with_cli(
    cli: Path,        # Caminho do executável
    img: Path,        # Imagem a processar
    tmin: float,      # Temperatura mínima (°C)
    tmax: float,      # Temperatura máxima (°C)
    palette: str      # Paleta de cores
) -> None:
```

**Processo:**

```
1. Criar path temporário
   img.atlas_tmp
         │
         ▼
2. Executar atlas_scale_set
   ./atlas_scale_set <img> <tmin> <tmax> <palette> <tmp>
         │
         ▼
3. Copiar metadados EXIF
   exiftool -TagsFromFile <img> <tmp>
   (ou piexif como fallback)
         │
         ▼
4. Substituir original
   mv <tmp> <img>
         │
         ▼
5. Restaurar timestamps
   os.utime(img, (atime, mtime))
```

**Comando Atlas SDK:**

```bash
./atlas_scale_set input.jpg 22.5 65.3 iron output.jpg
#                 │         │    │    │    └── Output
#                 │         │    │    └── Paleta
#                 │         │    └── Tmax (°C)
#                 │         └── Tmin (°C)
#                 └── Input (radiométrico)
```

---

## 7. Função Principal

### 7.1 `main() -> None`

Função principal que orquestra todo o processamento.

**Fluxo resumido:**

```python
def main() -> None:
    # 1. Parse argumentos
    ns = ArgsNS(**vars(ap.parse_args()))
    
    # 2. Validar input
    if not ns.input.is_dir():
        sys.exit(1)
    
    # 3. Localizar Atlas CLI
    cli = atlas_cli_path(ns.atlas_sdk_dir)
    
    # 4. Descobrir imagens
    files_in = discover_images(ns.input)
    
    # 5. Copiar para output (preservando estrutura)
    for src in files_in:
        copy_preserve(src, ns.output / src.relative_to(ns.input))
    
    # 6. Agrupar por tipo de equipamento
    groups = {}
    for p in files_out:
        gid = normalize_group_id(get_imagedescription(p))
        groups.setdefault(gid, []).append(p)
    
    # 7. Para cada grupo:
    for gid, gfiles in groups.items():
        # 7a. Calcular window térmica
        tmin, tmax, radiometric_set = robust_window_group(...)
        
        # 7b. Aplicar scale em cada imagem radiométrica
        for img in gfiles:
            if img in radiometric_set:
                apply_scale_with_cli(cli, img, tmin, tmax, palette)
        
        # 7c. Mover para pasta do grupo (estrutura flat)
        for img in gfiles:
            shutil.move(img, ns.output / gid / img.name)
    
    # 8. Limpar diretórios vazios
    cleanup_empty_dirs(ns.output)
    
    # 9. Gerar relatórios
    # - summary.json
    # - group_summary.csv
    # - per_image.csv
    # - failed_images.json
    # - skipped_groups.json
    # - non_radiometric_images.json
```

---

## 8. Argumentos CLI

### 8.1 Tabela de Argumentos

| Argumento | Obrigatório | Default | Descrição |
|-----------|-------------|---------|-----------|
| `--input` | ✅ | - | Diretório com imagens |
| `--output` | ✅ | - | Diretório de saída |
| `--atlas-sdk-dir` | ✅ | - | Diretório do Atlas SDK |
| `--palette` | ❌ | `"iron"` | Paleta de cores |
| `--p-low` | ❌ | `5.0` | Percentil inferior (°C) |
| `--p-high` | ❌ | `95.0` | Percentil superior (°C) |
| `--hist-clip` | ❌ | `0.0` | Clipping de histograma (%) |
| `--strict` | ❌ | `False` | Falhar em erros |

### 8.2 Paletas Disponíveis

| Paleta | Descrição |
|--------|-----------|
| `iron` | Preto → Roxo → Vermelho → Amarelo → Branco (padrão FLIR) |
| `rainbow` | Arco-íris completo |
| `bw` | Preto e branco (grayscale) |
| `whitehot` | Preto → Branco |
| `blackhot` | Branco → Preto |

### 8.3 Modo Strict

Com `--strict`, o script falha (exit code 2) se:
- Qualquer grupo for pulado (sem imagens radiométricas)
- Qualquer imagem falhar no tuning
- Houver imagens não-radiométricas

Útil para CI/CD ou quando qualidade é crítica.

---

## 9. Fluxo de Execução

```
main()
  │
  ├── argparse → ArgsNS
  │
  ├── Validar input existe
  │
  ├── atlas_cli_path() → localizar executável
  │
  ├── discover_images() → listar todas as imagens
  │
  ├── Copiar originais para output
  │     └── copy_preserve() [preserva timestamps]
  │
  ├── Agrupar por tipo de equipamento
  │     ├── get_imagedescription() → ler EXIF
  │     └── normalize_group_id() → normalizar (CB, INV, XMFR...)
  │
  ├── Para cada grupo:
  │     │
  │     ├── robust_window_group()
  │     │     ├── thermal_celsius() [para cada imagem]
  │     │     ├── Concatenar valores
  │     │     └── Calcular P5, P95
  │     │
  │     ├── Para cada imagem radiométrica:
  │     │     │
  │     │     └── apply_scale_with_cli()
  │     │           ├── subprocess: atlas_scale_set
  │     │           ├── subprocess: exiftool (ou piexif)
  │     │           └── Restaurar timestamps
  │     │
  │     └── Mover para pasta do grupo (flat)
  │           └── unique_dest() [evitar colisões]
  │
  ├── cleanup_empty_dirs()
  │
  └── Gerar relatórios
        ├── summary.json
        ├── group_summary.csv
        ├── per_image.csv
        ├── failed_images.json
        ├── skipped_groups.json
        └── non_radiometric_images.json
```

---

## 10. Formatos de Saída

### 10.1 Estrutura de Diretórios

```
output/
├── CB/
│   ├── 01-0036-15-INSIDE-01-CB_IR_54393.jpg
│   ├── 02-0036-15-INSIDE-01-CB_IR_54394.jpg
│   └── ...
├── INV/
│   ├── 05-0036-10-INSIDE-02-INV_IR_54400.jpg
│   └── ...
├── XMFR/
│   └── ...
├── summary.json
├── group_summary.csv
├── per_image.csv
├── failed_images.json
├── skipped_groups.json
└── non_radiometric_images.json
```

**Nota:** Estrutura FLAT dentro de cada grupo (sem subpastas).

### 10.2 summary.json

```json
{
  "CB": {
    "count": 15,
    "radiometric_count": 15,
    "p_low": 5.0,
    "p_high": 95.0,
    "hist_clip": 0.0,
    "tmin_c": 22.345,
    "tmax_c": 65.890,
    "palette": "iron",
    "files": [
      "/output/CB/image1.jpg",
      "/output/CB/image2.jpg"
    ]
  },
  "INV": {
    ...
  }
}
```

### 10.3 group_summary.csv

```csv
group_id,count,radiometric_count,p_low,p_high,hist_clip,tmin_c,tmax_c,palette
CB,15,15,5.0,95.0,0.0,22.345678,65.890123,iron
INV,10,10,5.0,95.0,0.0,25.123456,70.456789,iron
XMFR,5,5,5.0,95.0,0.0,20.111111,60.222222,iron
```

### 10.4 per_image.csv

```csv
group_id,original_description,file,tmin_c,tmax_c,palette,tuned
CB,CB-01,/output/CB/image1.jpg,22.345678,65.890123,iron,1
CB,CB-02,/output/CB/image2.jpg,22.345678,65.890123,iron,1
INV,INVERTER-1,/output/INV/image3.jpg,25.123456,70.456789,iron,1
```

### 10.5 failed_images.json

```json
[
  {
    "group_id": "CB",
    "original_description": "CB-03",
    "file": "/output/CB/image_broken.jpg",
    "reason": "atlas_scale_set failed (rc=1): Error message"
  }
]
```

### 10.6 skipped_groups.json

```json
[
  {
    "group_id": "UNKNOWN",
    "count": 3,
    "reason": "No valid radiometric image in group"
  }
]
```

### 10.7 non_radiometric_images.json

```json
[
  {
    "group_id": "CB",
    "original_description": "CB-04",
    "file": "/output/CB/visual_only.jpg",
    "reason": "non-radiometric (skipped tuning)"
  }
]
```

---

## 11. Atlas SDK

### 11.1 Visão Geral

O Atlas SDK é um projeto C separado que manipula imagens FLIR radiométricas. O executável principal é `atlas_scale_set`.

### 11.2 Localização

```
atlas_sdk/
├── src/
│   └── atlas_scale_set.c
├── include/
│   └── atlas.h
├── lib/
│   └── libatlas.so
├── build/
│   └── atlas_scale_set    ← Executável usado pelo Python
└── Makefile
```

### 11.3 Compilação

```bash
cd atlas_sdk
mkdir -p build
make

# ou com CMake
mkdir build && cd build
cmake ..
make
```

### 11.4 Uso do Executável

```bash
./atlas_scale_set <input> <tmin> <tmax> <palette> <output>
```

**Parâmetros:**
| Posição | Tipo | Descrição |
|---------|------|-----------|
| 1 | Path | Imagem FLIR de entrada |
| 2 | Float | Temperatura mínima (°C) |
| 3 | Float | Temperatura máxima (°C) |
| 4 | String | Nome da paleta |
| 5 | Path | Imagem de saída |

**Exemplo:**
```bash
./atlas_scale_set IR_54393.jpg 22.5 65.3 iron IR_54393_tuned.jpg
```

### 11.5 O que o Atlas SDK faz

1. Lê imagem FLIR radiométrica
2. Extrai matriz de temperaturas
3. Aplica novo scale range (min, max)
4. Aplica paleta de cores
5. Salva imagem mantendo formato radiométrico

**Importante:** A imagem de saída continua sendo radiométrica (contém dados de temperatura), não é apenas visual.

---

## 12. Exemplos de Uso

### 12.1 Uso Básico

```bash
python -m src.tools.tuning_thermal \
    --input "output/03_wp1_sorted" \
    --output "output/04_wp2_thermal_tuned" \
    --atlas-sdk-dir "atlas_sdk"
```

### 12.2 Com Parâmetros Customizados

```bash
python -m src.tools.tuning_thermal \
    --input "output/03_wp1_sorted" \
    --output "output/04_wp2_thermal_tuned" \
    --atlas-sdk-dir "atlas_sdk" \
    --palette rainbow \
    --p-low 10 \
    --p-high 90 \
    --hist-clip 1.0
```

### 12.3 Modo Strict (para CI/CD)

```bash
python -m src.tools.tuning_thermal \
    --input "output/03_wp1_sorted" \
    --output "output/04_wp2_thermal_tuned" \
    --atlas-sdk-dir "atlas_sdk" \
    --strict
```

### 12.4 Uso Programático

```python
from pathlib import Path
from src.tools.tuning_thermal import (
    thermal_celsius,
    robust_window_group,
    apply_scale_with_cli,
    atlas_cli_path
)

# Extrair temperaturas
temps = thermal_celsius(Path("image.jpg"))
print(f"Range: {temps.min():.1f} - {temps.max():.1f} °C")

# Calcular window para grupo
files = [Path("img1.jpg"), Path("img2.jpg"), Path("img3.jpg")]
tmin, tmax, radiometric = robust_window_group(files, p_low=5.0, p_high=95.0, hist_clip=0.0)
print(f"Group window: {tmin:.1f} - {tmax:.1f} °C")

# Aplicar scale
cli = atlas_cli_path(Path("atlas_sdk"))
apply_scale_with_cli(cli, Path("image.jpg"), tmin, tmax, "iron")
```

---

## 13. Integração com Pipeline

### 13.1 Chamada pelo run_full_pipeline.py

```python
# Em step_3_wp2_thermal_tuning()
cmd = [
    sys.executable, "-m", "src.tools.tuning_thermal",
    "--input", str(input_dir),
    "--output", str(output_dir),
    "--atlas-sdk-dir", str(config.atlas_sdk_dir),
    "--palette", config.wp2_palette,
]

proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
```

### 13.2 Fluxo de Dados

```
03_wp1_sorted/           tuning_thermal.py       04_wp2_thermal_tuned/
     │                         │                        │
     ├── CB/                   │                        ├── CB/
     │   ├── img1.jpg ─────────┼────────────────────►   │   ├── img1.jpg (tuned)
     │   └── img2.jpg ─────────┼────────────────────►   │   └── img2.jpg (tuned)
     │                         │                        │
     ├── INV/                  │                        ├── INV/
     │   └── img3.jpg ─────────┼────────────────────►   │   └── img3.jpg (tuned)
     │                         │                        │
     └── XMFR/                 │                        └── XMFR/
         └── img4.jpg ─────────┴────────────────────►       └── img4.jpg (tuned)

                        + Relatórios JSON/CSV
```

### 13.3 Consumidores

| Etapa | Módulo | Uso |
|-------|--------|-----|
| WP4+WP5 | `wp4_wp5_integration_v4.py` | Input `--raw-image` (imagens tuned) |

---

## 14. Notas de Implementação

### 14.1 Design Decisions

1. **Agrupamento por TIPO, não instância**
   - `CB-01`, `CB-02`, `CB-03` → Mesmo grupo `CB`
   - Garante range consistente para comparação visual

2. **Estrutura flat de saída**
   - Sem subpastas dentro dos grupos
   - Simplifica navegação e processamento downstream

3. **Preservação de metadados via exiftool**
   - exiftool copia TODOS os metadados (FLIR proprietários inclusos)
   - piexif é fallback limitado (apenas EXIF padrão)

4. **Imagens não-radiométricas são copiadas, não tuned**
   - Permite manter imagens visuais no dataset
   - Relatório indica quais foram puladas

### 14.2 Limitações Conhecidas

1. **Dependência do Atlas SDK**
   - Executável C deve ser compilado manualmente
   - Não há fallback Python puro

2. **Aliases limitados**
   - Apenas CB, INV, XMFR pré-definidos
   - Outros tipos usam prefixo literal

3. **Sem paralelização**
   - Processamento sequencial
   - Potencial para multiprocessing

### 14.3 Tratamento de Erros

| Situação | Comportamento |
|----------|---------------|
| Atlas SDK não encontrado | RuntimeError, aborta |
| exiftool não encontrado | Warning, usa piexif |
| Imagem não radiométrica | Copia sem tuning, registra |
| Grupo sem radiométricas | Pula grupo, registra |
| atlas_scale_set falha | Registra erro, continua |
| `--strict` + erros | Exit code 2 |

### 14.4 Performance

- **Tempo típico:** ~1-3s por imagem (I/O + Atlas CLI)
- **Gargalo:** Chamada ao subprocess atlas_scale_set
- **Memória:** Carrega todas as temps do grupo para cálculo de window

### 14.5 Compatibilidade

- **Python:** 3.8+
- **SO:** Linux (Atlas SDK é compilado para Linux)
- **FLIR:** Testado com E75, T420, T530

---

## Changelog

### v1.0 (Fevereiro 2025)
- Documentação inicial
- Integração com Atlas SDK estável
- Relatórios JSON/CSV completos

---

## Referências

- [Atlas SDK](interno - projeto C)
- [exiftool](https://exiftool.org/)
- [piexif](https://github.com/hMatoba/Piexif)
- [flirimageextractor](https://github.com/nationaldronesau/FlirImageExtractor)

---

*Documentação gerada para handoff do projeto QE Solar.*
