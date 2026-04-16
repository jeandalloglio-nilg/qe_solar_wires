# QE Solar - Pipeline de Inspeção Térmica

## Documentação Técnica Completa

**Versão:** 1.0  
**Última atualização:** Fevereiro 2025  
**Autor original:** Elton Gino Santos  
**Projeto:** QE Solar Thermal Inspection Pipeline

---

## Índice

1. [Visão Geral](#1-visão-geral)
2. [Arquitetura do Sistema](#2-arquitetura-do-sistema)
3. [Requisitos e Dependências](#3-requisitos-e-dependências)
4. [Estrutura de Diretórios](#4-estrutura-de-diretórios)
5. [Configuração Inicial](#5-configuração-inicial)
6. [Execução do Pipeline](#6-execução-do-pipeline)
7. [Detalhamento das Etapas](#7-detalhamento-das-etapas)
8. [Parâmetros de Configuração](#8-parâmetros-de-configuração)
9. [Estrutura de Saída](#9-estrutura-de-saída)
10. [Troubleshooting](#10-troubleshooting)
11. [Manutenção e Atualizações](#11-manutenção-e-atualizações)

---

## 1. Visão Geral

Dependências Mapeadas:
run_full_pipeline.py
    ├── subprocess → clean_ui.py
    ├── subprocess → tuning_thermal.py
    │                    └── subprocess → atlas_scale_set (C)
    │                    └── subprocess → exiftool
    ├── subprocess → wp4_wp5_integration_v4.py
    │                    ├── subprocess → exiftool
    │                    ├── subprocess → atlas_write_spots (C, compilado dinamicamente)
    │                    ├── API → OpenAI (GPT-4o Vision)
    │                    └── lib → ultralytics (YOLO)
    └── subprocess → wp6_repx_generator.py

### 1.1 Objetivo

O **QE Solar Pipeline** é um sistema automatizado para processamento de inspeções térmicas em instalações solares fotovoltaicas. O pipeline processa imagens térmicas FLIR para detectar anomalias (hot spots) em painéis solares, utilizando modelos de deep learning (YOLO) para detecção de keypoints e clustering para identificação de padrões anômalos.

### 1.2 Principais Funcionalidades

- **Extração de RGB** de imagens FLIR RAW
- **Limpeza de UI** das imagens térmicas (remoção de overlays da interface FLIR)
- **Sorting automático** por metadados EXIF (ImageDescription)
- **Thermal Range Tuning** usando Atlas SDK (ajuste P5-P95)
- **Detecção de keypoints** via YOLO (cantos de células/módulos)
- **Mapeamento térmico → RGB** com alinhamento geométrico
- **Clustering** para agrupamento de detecções
- **Análise de anomalias** com classificação de severidade
- **Geração de relatórios** em formato REPX

### 1.3 Modo de Operação

O pipeline opera em modo **RAW-Only**, processando exclusivamente imagens FLIR originais (formato radiométrico). Este modo garante:

- Acesso aos dados térmicos completos
- Extração precisa de temperaturas
- Consistência entre visualização e dados numéricos

---

## 2. Arquitetura do Sistema

### 2.1 Fluxo de Processamento

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          QE SOLAR PIPELINE                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  [RAW Images FLIR]                                                          │
│         │                                                                   │
│         ▼                                                                   │
│  ┌─────────────────┐                                                        │
│  │ ETAPA 0         │──────► 01_rgb_extracted/                               │
│  │ Extração RGB    │        (imagens RGB embutidas)                         │
│  └────────┬────────┘                                                        │
│           │                                                                 │
│           ▼                                                                 │
│  ┌─────────────────┐                                                        │
│  │ ETAPA 1         │──────► 02_thermal_clean/                               │
│  │ Clean UI        │        (térmicas sem overlay)                          │
│  └────────┬────────┘                                                        │
│           │                                                                 │
│           ▼                                                                 │
│  ┌─────────────────┐                                                        │
│  │ ETAPA 2 (WP1)   │──────► 03_wp1_sorted/                                  │
│  │ Sorting EXIF    │        (organizadas por grupo)                         │
│  └────────┬────────┘                                                        │
│           │                                                                 │
│           ▼                                                                 │
│  ┌─────────────────┐                                                        │
│  │ ETAPA 3 (WP2)   │──────► 04_wp2_thermal_tuned/                           │
│  │ Thermal Tuning  │        (scale range P5-P95)                            │
│  └────────┬────────┘                                                        │
│           │                                                                 │
│           ▼                                                                 │
│  ┌─────────────────┐                                                        │
│  │ ETAPA 4         │──────► 05_wp4_wp5_analysis/                            │
│  │ WP4+WP5         │        (keypoints, clusters, anomalias)                │
│  │ YOLO + Cluster  │                                                        │
│  └────────┬────────┘                                                        │
│           │                                                                 │
│           ▼                                                                 │
│  ┌─────────────────┐                                                        │
│  │ ETAPA 5 (WP6)   │──────► 06_report/                                      │
│  │ Relatório REPX  │        (report.repx, CSVs, JSONs)                      │
│  └─────────────────┘                                                        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Componentes Principais

| Componente | Arquivo | Função |
|------------|---------|--------|
| **Pipeline Principal** | `run_full_pipeline.py` | Orquestração completa |
| **Clean UI** | `clean_ui.py` | Remoção de overlays FLIR |
| **Thermal Tuning** | `tuning_thermal.py` | Ajuste de range térmico |
| **WP4+WP5 Integration** | `wp4_wp5_integration_v4.py` | Detecção e análise |
| **REPX Generator** | `wp6_repx_generator.py` | Geração de relatórios |

### 2.3 Dependências Externas

- **Atlas SDK**: Biblioteca C++ para manipulação de scale range em imagens FLIR
- **YOLO Model**: Modelo treinado para detecção de keypoints (cantos de células)
- **ExifTool**: Manipulação de metadados EXIF
- **OpenAI API**: Classificação de anomalias (opcional)

---

## 3. Requisitos e Dependências

### 3.1 Sistema Operacional

- Linux (Ubuntu 20.04+ recomendado)
- WSL2 no Windows (testado)

### 3.2 Python

```
Python 3.10+
```

### 3.3 Dependências Python

```bash
# Core
opencv-python>=4.8.0
numpy>=1.24.0
Pillow>=10.0.0

# FLIR Processing
flirimageextractor>=1.1.0

# Machine Learning
ultralytics>=8.0.0  # YOLO
torch>=2.0.0

# Utilities
tqdm>=4.65.0

# Optional (para análise com LLM)
openai>=1.0.0
```

### 3.4 Ferramentas Externas

```bash
# ExifTool (manipulação EXIF)
sudo apt-get install libimage-exiftool-perl

# Atlas SDK (compilado separadamente - ver seção 5.2)
```

### 3.5 Instalação

```bash
# Clonar repositório
git clone <repo_url> qe_solar_wires
cd qe_solar_wires

# Criar ambiente virtual
python -m venv .venv
source .venv/bin/activate

# Instalar dependências
pip install -r requirements.txt

# Verificar instalação
python -c "import cv2; import numpy; import ultralytics; print('OK')"
```

---

## 4. Estrutura de Diretórios

### 4.1 Estrutura do Projeto

```
qe_solar_wires/
├── src/
│   └── tools/
│       ├── run_full_pipeline.py      # Pipeline principal
│       ├── clean_ui.py               # Limpeza de UI
│       ├── tuning_thermal.py         # Thermal tuning
│       ├── wp4_wp5_integration_v4.py # Detecção + análise
│       ├── wp6_repx_generator.py     # Geração de relatórios
│       └── QE_SOLAR_logo.png         # Logo para relatórios
│
├── models/
│   └── best.pt                       # Modelo YOLO padrão
│
├── runs/
│   └── wp4_keypoints/
│       └── train_YYYYMMDD_HHMMSS/
│           └── weights/
│               └── best.pt           # Modelos treinados
│
├── atlas_sdk/
│   ├── build/
│   │   └── atlas_scale_set           # Executável compilado
│   └── src/                          # Código fonte
│
├── data/
│   └── <Cliente>/
│       └── <Site>/
│           └── <Ano>/
│               └── Raw Images/       # Imagens FLIR originais
│
├── output/                           # Saídas do pipeline
│
└── requirements.txt
```

### 4.2 Estrutura de Input Esperada

```
data/<Cliente>/<Site>/<Ano>/
└── Raw Images/          # OU: Raw/, RAW/, raw_images/, raw/
    ├── IR_54393.jpg     # Imagens FLIR radiométricas
    ├── IR_54394.jpg
    ├── IR_54395.jpg
    └── ...
```

**Nota:** O pipeline auto-detecta a pasta de imagens RAW procurando por:
- `Raw Images` (padrão)
- `Raw`, `RAW`, `raw_images`, `raw`

---

## 5. Configuração Inicial

### 5.1 Modelo YOLO

O pipeline utiliza um modelo YOLO treinado especificamente para detectar keypoints (cantos de células solares) em imagens térmicas.

**Localização padrão:**
```
models/best.pt
```

**Nota:** Se `models/best.pt` não existir, o pipeline tenta auto-detectar automaticamente o modelo mais recente em:
```
runs/wp4_keypoints/**/weights/best.pt
```

**Modelos treinados disponíveis:**
```
runs/wp4_keypoints/train_YYYYMMDD_HHMMSS/weights/best.pt
```

**Métricas do modelo atual (train_20251211_010508):**
- mAP50: 82.7%
- Confiança recomendada: 0.15 - 0.30

### 5.2 Atlas SDK

O Atlas SDK é necessário para o Thermal Tuning (WP2). É uma ferramenta C++ que manipula o scale range das imagens FLIR.

**Compilação:**
```bash
make -C atlas_sdk clean
make -C atlas_sdk

chmod +x atlas_sdk/build/atlas_scale_set atlas_sdk/build/check_settings
```

**Verificação:**
```bash
./atlas_sdk/build/atlas_scale_set --help
```

**Localização esperada:**
```
atlas_sdk/build/atlas_scale_set
```

**SDK oficial (repo):**
```
flir_atlas_c/
  include/
  lib/
```

### 5.3 Parâmetros de Alinhamento (Thermal → RGB)

Cada modelo de câmera FLIR tem parâmetros de alinhamento diferentes. Os valores abaixo foram calibrados empiricamente:

| Parâmetro | Descrição | Valor típico |
|-----------|-----------|--------------|
| `real2ir` | Fator de escala RGB→Thermal | 1.74 - 1.75 |
| `offset_x` | Deslocamento horizontal (px) | -10 |
| `offset_y` | Deslocamento vertical (px) | -13 |

**Nota:** Estes valores podem variar entre câmeras (E75, T420, T530). Em caso de desalinhamento visível, ajustar manualmente.

---

## 6. Execução do Pipeline

### 6.1 Comando Básico

```bash
cd ~/qe_solar_wires
source .venv/bin/activate

python -m src.tools.run_full_pipeline \
    --site-folder "data/<Cliente>/<Site>/<Ano>" \
    --output "output/<Nome_Output>"
```

### 6.2 Comando Completo (Recomendado)

```bash
python -m src.tools.run_full_pipeline \
    --site-folder "data/Syncarpha/Syncarpha/Syncarpha/Dodge 1/2025" \
    --output "output/Dodge_2025" \
    --atlas-sdk-dir "atlas_sdk" \
    --yolo-model "runs/wp4_keypoints/train_20251211_010508/weights/best.pt" \
    --yolo-conf 0.15 \
    --real2ir 1.75 \
    --offset-x -10 \
    --offset-y -13
```

### 6.2.1 Modo de Saída Minimal (visible/ + FLIR/)

Use este modo quando você quer **rodar o pipeline completo**, mas no final manter apenas os outputs essenciais, separados em duas pastas:

- `visible/`: imagens térmicas com keypoints desenhados (visualização rápida)
- `FLIR/`: imagens térmicas com spots/keypoints gravados no EXIF (para abrir no FLIR Tools)

```bash
python -m src.tools.run_full_pipeline \
    --site-folder "data/<Cliente>/<Site>/<Ano>" \
    --output "output/<Nome_Output>" \
    --minimal-output \
    --force-clean-output \
    --yolo-model "runs/wp4_keypoints/train_YYYYMMDD_HHMMSS/weights/best.pt" \
    --yolo-conf 0.15
```

**Arquivo adicional:** no modo `--minimal-output`, o pipeline também grava um `pipeline_log.json` no `--output` final, para permitir retry de erros.

### 6.3 Exemplos Práticos

**Exemplo 1: Site novo com configuração padrão**
```bash
python -m src.tools.run_full_pipeline \
    --site-folder "data/NovoCliente/SiteA/2025" \
    --output "output/NovoCliente_SiteA_2025"
```

**Exemplo 2: Reprocessamento apenas WP4+WP5**
```bash
python -m src.tools.run_full_pipeline \
    --site-folder "data/Cliente/Site/2025" \
    --output "output/Site_reprocess" \
    --skip-clean \
    --skip-wp1 \
    --skip-wp2
```

**Exemplo 2B: Reprocessar apenas imagens que deram erro (retry automático via log)**

Se uma execução anterior gerou `pipeline_log.json` (inclui status por imagem), você pode reprocessar **somente** as imagens com `status=error`:

```bash
python -m src.tools.run_full_pipeline \
    --site-folder "data/Cliente/Site/2025" \
    --output "output/Site_retry" \
    --minimal-output \
    --force-clean-output \
    --retry-errors-from "output/Site/pipeline_log.json"
```

**Exemplo 3: Apenas sorting e organização (sem análise)**
```bash
python -m src.tools.run_full_pipeline \
    --site-folder "data/Cliente/Site/2025" \
    --output "output/Site_sorted" \
    --skip-wp4-wp5 \
    --skip-wp6
```

**Exemplo 4: Threshold de anomalia mais sensível**
```bash
python -m src.tools.run_full_pipeline \
    --site-folder "data/Cliente/Site/2025" \
    --output "output/Site_sensitive" \
    --yolo-conf 0.10 \
    --delta-threshold 2.0
```

---

## 7. Detalhamento das Etapas

### 7.1 Etapa 0: Extração de RGB

**Objetivo:** Extrair a imagem RGB embutida nas imagens FLIR radiométricas.

**Processo:**
1. Lê imagem FLIR RAW
2. Extrai campo "EmbeddedImage" do EXIF
3. Salva como JPEG com qualidade 95%

**Output:** `01_rgb_extracted/<nome>_rgb.jpg`

**Fallback:** Se `flirimageextractor` falhar, usa `exiftool` diretamente.

### 7.2 Etapa 1: Clean Thermal (Limpeza de UI)

**Objetivo:** Remover overlays da interface FLIR (barra de temperatura, logo, etc.) para processamento YOLO limpo.

**Processo:**
1. Extrai dados térmicos puros
2. Aplica colormap (inferno por padrão)
3. Gera imagem limpa sem UI

**Output:** `02_thermal_clean/<nome>_clean.png`

**Módulo:** `src.tools.clean_ui`

### 7.3 Etapa 2: WP1 - Sorting

**Objetivo:** Organizar imagens por grupo/área baseado no campo EXIF `ImageDescription`.

**Processo:**
1. Lê `ImageDescription` de cada imagem
2. Agrupa imagens pelo valor do campo
3. Renomeia com padrão: `<idx>-<total>-<grupo_size>-INSIDE-<grupo_idx>-<grupo>_<original>`
4. Copia para subpastas organizadas

**Output:** `03_wp1_sorted/<GroupName>/<sorted_images>`

**Formato do nome:**
```
01-0036-15-INSIDE-01-Inverter1_IR_54393.jpg
│   │    │    │    │     │         └── Nome original
│   │    │    │    │     └── Nome do grupo (EXIF)
│   │    │    │    └── Índice do grupo
│   │    │    └── Posição (INSIDE/OUTSIDE)
│   │    └── Quantidade no grupo
│   └── Total de imagens
└── Índice global
```

### 7.4 Etapa 3: WP2 - Thermal Tuning

**Objetivo:** Ajustar scale range térmico para visualização otimizada (P5-P95).

**Processo:**
1. Analisa distribuição de temperaturas
2. Calcula percentis 5% e 95%
3. Aplica novo scale range via Atlas SDK
4. Preserva dados radiométricos

**Output:** `04_wp2_thermal_tuned/<GroupName>/<tuned_images>`

**Dependência:** Atlas SDK (`atlas_scale_set`)

**Fallback:** Se Atlas SDK não disponível, copia imagens sem modificação.

### 7.5 Etapa 4: WP4+WP5 - Análise de Keypoints e Anomalias

**Objetivo:** Detectar keypoints, mapear para RGB, clusterizar e identificar anomalias térmicas.

**Processo:**

1. **Detecção YOLO** na imagem térmica limpa
   - Detecta cantos de células/módulos
   - Filtra por confiança mínima

2. **Mapeamento Thermal → RGB**
   - Aplica transformação geométrica (real2ir, offsets)
   - Escala coordenadas para resolução RGB

3. **Clustering**
   - Agrupa keypoints próximos
   - Identifica células/módulos

4. **Extração de Temperaturas**
   - Lê temperatura em cada keypoint
   - Usa kernel 3x3 para média local

5. **Análise de Anomalias**
   - Calcula delta para mediana do cluster
   - Classifica severidade (high/medium/low)
   - Threshold padrão: 3.0°C

6. **Visualização Dual**
   - Gera overlay em imagem térmica
   - Gera overlay em imagem RGB

7. **Injeção EXIF**
   - Injeta spots detectados no EXIF da imagem tuned
   - Compatível com FLIR Tools

**Output:**
```
05_wp4_wp5_analysis/<sorted_name>/
├── thermal/
│   └── <name>_thermal_viz.png
├── rgb/
│   └── <name>_rgb_viz.png
├── keypoints_used.json
├── anomalies.json
└── <name>_edited.jpg  # Com spots injetados
```

**FLIR Tools (spots no EXIF):**
Quando `--inject-exif` está ativo, os arquivos com spots injetados são gravados em:
```
05_wp4_wp5_analysis/edited/
```

**Módulo:** `src.tools.wp4_wp5_integration_v4`

### 7.6 Etapa 5: WP6 - Relatório

**Objetivo:** Gerar relatório consolidado em formato REPX.

**Processo:**
1. Consolida anomalias de todas as imagens
2. Gera estatísticas por severidade
3. Cria arquivo REPX (formato FLIR Reports)
4. Exporta CSV e JSON de resumo

**Output:**
```
06_report/
├── report.repx
├── site_summary.json
└── anomalies_report.csv
```

**Módulo:** `src.tools.wp6_repx_generator`

---

## 8. Parâmetros de Configuração

### 8.1 Parâmetros Obrigatórios

| Parâmetro | Descrição |
|-----------|-----------|
| `--site-folder` | Caminho para pasta do site contendo RAW Images |
| `--output` / `-o` | Caminho para pasta de output |

### 8.2 Parâmetros de Modelo YOLO

| Parâmetro | Default | Descrição |
|-----------|---------|-----------|
| `--keypoint-source` | `yolo` | Fonte de keypoints (`yolo` ou `exif`) |
| `--yolo-model` | `models/best.pt` | Caminho para modelo YOLO |
| `--yolo-conf` | `0.30` | Confiança mínima (0.0 - 1.0) |

**Recomendação:** Para maior recall, usar `--yolo-conf 0.15`

### 8.3 Parâmetros de Alinhamento

| Parâmetro | Default | Descrição |
|-----------|---------|-----------|
| `--real2ir` | Auto | Fator de escala RGB→Thermal |
| `--offset-x` | Auto | Deslocamento X em pixels |
| `--offset-y` | Auto | Deslocamento Y em pixels |

**Valores calibrados para FLIR T-series:**
- `--real2ir 1.75 --offset-x -10 --offset-y -13`

### 8.4 Parâmetros de Análise

| Parâmetro | Default | Descrição |
|-----------|---------|-----------|
| `--delta-threshold` | `3.0` | Delta mínimo (°C) para anomalia |
| `--openai-model` | `gpt-4o` | Modelo para classificação (opcional) |

### 8.5 Flags de Skip

| Flag | Descrição |
|------|-----------|
| `--skip-clean` | Pular limpeza de UI |
| `--skip-wp1` | Pular sorting |
| `--skip-wp2` | Pular thermal tuning |
| `--skip-wp4-wp5` | Pular análise |
| `--skip-wp6` | Pular relatório |

### 8.6 Outros Parâmetros

| Parâmetro | Default | Descrição |
|-----------|---------|-----------|
| `--raw-subdir` | Auto | Nome da subpasta RAW |
| `--atlas-sdk-dir` | Auto | Diretório do Atlas SDK |
| `--minimal-output` | false | Exporta apenas outputs finais em `visible/` e `FLIR/` (sem manter intermediários) |
| `--force-clean-output` | false | No modo `--minimal-output`, apaga o conteúdo existente do `--output` antes de exportar |
| `--retry-errors-from` | None | Reprocessa apenas imagens com `status=error` a partir de um `pipeline_log.json` |

---

## 9. Estrutura de Saída

### 9.1 Estrutura Completa

```
output/<Nome_Output>/
│
├── 01_rgb_extracted/
│   ├── IR_54393_rgb.jpg
│   ├── IR_54394_rgb.jpg
│   └── ...
│
├── 02_thermal_clean/
│   ├── IR_54393_clean.png
│   ├── IR_54394_clean.png
│   └── ...
│
├── 03_wp1_sorted/
│   ├── Inverter1/
│   │   ├── 01-0036-15-INSIDE-01-Inverter1_IR_54393.jpg
│   │   └── ...
│   ├── Inverter2/
│   │   └── ...
│   └── Outdoor/
│       └── ...
│
├── 04_wp2_thermal_tuned/
│   ├── Inverter1/
│   │   ├── 01-0036-15-INSIDE-01-Inverter1_IR_54393.jpg
│   │   └── ...
│   └── ...
│
├── 05_wp4_wp5_analysis/
│   ├── 01-0036-15-INSIDE-01-Inverter1_IR_54393/
│   │   ├── thermal/
│   │   │   └── *_thermal_viz.png
│   │   ├── rgb/
│   │   │   └── *_rgb_viz.png
│   │   ├── keypoints_used.json
│   │   ├── anomalies.json
│   │   └── *_edited.jpg
│   └── ...
│
├── 06_report/
│   ├── report.repx
│   ├── site_summary.json
│   └── anomalies_report.csv
│
└── pipeline_log.json
```

### 9.1.1 Estrutura de Saída no modo `--minimal-output`

Neste modo o pipeline roda internamente em uma pasta temporária e, ao final, mantém apenas:

```
output/<Nome_Output>/
├── visible/
│   ├── <prefix>_thermal_visible.png
│   └── ...
├── FLIR/
│   ├── <prefix>_thermal_FLIR.jpg
│   └── ...
└── pipeline_log.json
```

### 9.2 Arquivos de Saída Importantes

**`pipeline_log.json`**
```json
{
  "site_name": "2025",
  "start_time": "2025-02-12T10:30:00",
  "end_time": "2025-02-12T10:45:30",
  "duration_seconds": 930.5,
  "total_images": 36,
  "processed_images": 35,
  "failed_images": 1,
  "total_anomalies": 12,
  "high_severity": 3,
  "medium_severity": 5,
  "low_severity": 4
}
```

**`anomalies.json` (por imagem)**
```json
{
  "total_anomalies": 2,
  "anomalies": [
    {
      "keypoint_id": 15,
      "component_type": "cell",
      "temperature_c": 58.3,
      "delta_to_median_c": 5.2,
      "severity": "high",
      "position": {"x": 245, "y": 312}
    }
  ]
}
```

**`site_summary.json`**
```json
{
  "site_name": "Dodge 1",
  "generated_at": "2025-02-12T10:45:30",
  "mode": "RAW-only (Tuned)",
  "total_images": 36,
  "processed": 35,
  "anomalies": {
    "total": 12,
    "high": 3,
    "medium": 5,
    "low": 4
  }
}
```

---

## 10. Troubleshooting

### 10.1 Erros Comuns

#### "RAW Images não encontrado"

**Causa:** Pasta de imagens RAW não detectada.

**Solução:**
```bash
# Verificar estrutura
ls -la "data/Cliente/Site/Ano/"

# Especificar subpasta manualmente
--raw-subdir "Raw Images"
```

#### "RGB not found"

**Causa:** Extração de RGB falhou para alguma imagem.

**Solução:**
1. Verificar se imagem é FLIR válida
2. Verificar instalação do `flirimageextractor`
3. Verificar se `exiftool` está instalado

```bash
exiftool -b -EmbeddedImage imagem.jpg > teste.jpg
```

#### "Clean thermal não encontrado"

**Causa:** Etapa de limpeza não gerou output.

**Solução:**
```bash
# Rodar clean_ui manualmente
python -m src.tools.clean_ui \
    --input "data/Site/Raw Images" \
    --output "output/test_clean"
```

#### "Atlas SDK não encontrado"

**Causa:** Atlas SDK não compilado ou path incorreto.

**Solução:**
```bash
# Verificar existência
ls atlas_sdk/build/atlas_scale_set

# Compilar se necessário
cd atlas_sdk && mkdir build && cd build && cmake .. && make
```

#### "Timeout (300s)"

**Causa:** Processamento de imagem muito lento.

**Solução:**
- Verificar se GPU está sendo usada (YOLO)
- Reduzir resolução de entrada
- Aumentar timeout no código

#### Falha ocasional no clustering (OpenAI) com JSON vazio / output_text vazio

**Sintoma:** erro parecido com:

```
ValueError: OpenAI response output_text is empty
ValueError: Failed to parse OpenAI clustering JSON. output_text preview: <empty>
```

**Causa:** A API do OpenAI pode retornar uma resposta vazia/truncada de forma intermitente.

**Comportamento atual:** o pipeline trata isso como **best-effort** e aplica um fallback determinístico (cluster único `unassigned`), para que a imagem ainda gere:

- outputs em `visible/`
- imagem em `FLIR/` (EXIF spots)

E o motivo é registrado em `run_metadata.json` (campo `openai_clustering_error`).

### 10.2 Problemas de Alinhamento

Se as visualizações mostrarem desalinhamento entre thermal e RGB:

1. **Verificar modelo da câmera**
   ```bash
   exiftool -Model imagem.jpg
   ```

2. **Ajustar parâmetros manualmente**
   ```bash
   --real2ir 1.70 --offset-x -8 --offset-y -15
   ```

3. **Testar incrementalmente**
   - Começar com `real2ir` (escala)
   - Depois ajustar offsets

### 10.3 Logs e Debug

```bash
# Ver log completo
cat output/Site/pipeline_log.json | python -m json.tool

# Verificar erros específicos
grep -r "error" output/Site/05_wp4_wp5_analysis/*/anomalies.json
```

---

## 11. Manutenção e Atualizações

### 11.1 Retreinamento do Modelo YOLO

Se a performance do modelo degradar ou novos tipos de painéis forem adicionados:

1. **Preparar dataset YOLO a partir de spots no EXIF (recomendado)**

   Este fluxo alinha o domínio de treino com o domínio de inferência do pipeline (que roda YOLO sobre `thermal_clean`).

   ```bash
   python -m src.tools.prepare_yolo_dataset_from_exif_spots \
     --input-images /home/jean/qe_solar_wires/dataset2train/Edited \
     --output-dataset /home/jean/qe_solar_wires/data/all_clean_bbox_10 \
     --render-clean-ui \
     --bbox-size 10 \
     --only-thermal-320x240 \
     --skip-exiftool-errors \
     --skip-render-errors
   ```

   Referência: `src/tools/docs/prepare_yolo_dataset_from_exif_spots.md`.

2. **Validar visualmente algumas amostras (Edited vs Clean + labels)**

   Em ambiente headless, o `--show` gera um PNG temporário em `/tmp` (ou no caminho indicado por `--tmp-output`).

   ```bash
   python -m src.tools.viz_exif_spots_on_clean \
     --mode yolo \
     --edited-image /home/jean/qe_solar_wires/dataset2train/Edited/FLIR3243.jpg \
     --clean-image  /home/jean/qe_solar_wires/data/all_clean_bbox_10/images/FLIR3243_clean.png \
     --yolo-label   /home/jean/qe_solar_wires/data/all_clean_bbox_10/labels/FLIR3243_clean.txt \
     --draw-ids \
     --show \
     --no-save \
     --tmp-output /home/jean/qe_solar_wires/tmp/compare_FLIR3243.png
   ```

   Referência: `src/tools/docs/viz_exif_spots_on_clean.md`.

3. **Treinar um novo modelo YOLO**

   ```bash
   python -m src.tools.train_yolo_wp4 \
     --data-dir /home/jean/qe_solar_wires/data/all_clean_bbox_10 \
     --epochs 300 \
     --batch 8 \
     --imgsz 640 \
     --model yolov8n.pt
   ```

   Referência: `src/tools/docs/train_yolo_wp4.md`.

4. **Atualizar o path do modelo no pipeline**

   Após o treino, use o `best.pt` gerado em:

   ```text
   runs/wp4_keypoints/train_YYYYMMDD_HHMMSS/weights/best.pt
   ```

   E atualize:

   ```bash
   --yolo-model "runs/wp4_keypoints/train_YYYYMMDD_HHMMSS/weights/best.pt"
   ```

### 11.2 Atualização de Dependências

```bash
# Atualizar requirements
pip install --upgrade -r requirements.txt

# Testar após atualização
python -m src.tools.run_full_pipeline \
    --site-folder "data/test_site" \
    --output "output/test" \
    --skip-wp6
```

### 11.3 Backup de Modelos

Manter backup dos modelos treinados:
```bash
cp -r runs/wp4_keypoints/train_YYYYMMDD_HHMMSS /backup/models/
```

---

## Apêndice A: Referência Rápida

### Comando Padrão de Execução

```bash
cd ~/qe_solar_wires
source .venv/bin/activate

python -m src.tools.run_full_pipeline \
    --site-folder "data/<CLIENTE>/<SITE>/<ANO>" \
    --output "output/<NOME_OUTPUT>" \
    --atlas-sdk-dir "atlas_sdk" \
    --yolo-model "runs/wp4_keypoints/train_20251211_010508/weights/best.pt" \
    --yolo-conf 0.15 \
    --real2ir 1.75 \
    --offset-x -10 \
    --offset-y -13
```

### Checklist Pré-Execução

- [ ] Ambiente virtual ativado (`.venv`)
- [ ] Imagens RAW na pasta correta
- [ ] Modelo YOLO disponível
- [ ] Atlas SDK compilado (opcional, para WP2)
- [ ] Espaço em disco suficiente (~3x tamanho das RAW)

### OpenAI (opcional)

Se você for usar clustering via OpenAI, coloque a chave em um arquivo `.env` na raiz do repositório:
```
OPENAI_API_KEY=...
```

**Nota:** `.env` é ignorado pelo `.gitignore`.

