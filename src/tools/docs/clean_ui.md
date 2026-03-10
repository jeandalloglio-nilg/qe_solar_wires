# clean_ui.py

## Documentação Técnica

**Módulo:** `src.tools.clean_ui`  
**Versão:** 1.0  
**Autor:** Elton Gino / QE Solar  
**Última atualização:** Fevereiro 2025

---

## Sumário

1. [Descrição](#1-descrição)
2. [Localização e Execução](#2-localização-e-execução)
3. [Dependências](#3-dependências)
4. [Classe FLIRThermalAnalyzer](#4-classe-flirthermalanalyzer)
5. [Funções de Processamento](#5-funções-de-processamento)
6. [Argumentos CLI](#6-argumentos-cli)
7. [Fluxo de Execução](#7-fluxo-de-execução)
8. [Formatos de Saída](#8-formatos-de-saída)
9. [Exemplos de Uso](#9-exemplos-de-uso)
10. [Integração com Pipeline](#10-integração-com-pipeline)
11. [Notas de Implementação](#11-notas-de-implementação)

---

## 1. Descrição

### 1.1 Propósito

O `clean_ui.py` é responsável por **extrair e renderizar imagens térmicas limpas** a partir de arquivos FLIR radiométricos. Remove overlays de interface (UI/HUD) como barras de temperatura, logos e informações de escala, gerando imagens térmicas puras ideais para processamento por modelos de machine learning (YOLO).

### 1.2 Funcionalidades Principais

| Funcionalidade | Descrição |
|----------------|-----------|
| **Extração Térmica** | Extrai matriz de temperaturas em °C de imagens FLIR |
| **Renderização Limpa** | Aplica colormap sem elementos de UI |
| **Detecção de Hotspots** | Identifica pontos quentes com limiar adaptativo |
| **Análise de Painéis** | Diagnóstico específico para equipamentos elétricos |
| **Processamento em Lote** | Processa múltiplas imagens automaticamente |

### 1.3 Contexto no Pipeline

```
Pipeline QE Solar
      │
      ├── Etapa 0: Extração RGB
      │
      ├── Etapa 1: Clean UI  ◄─── ESTE MÓDULO
      │       │
      │       └── Gera: 02_thermal_clean/<stem>_clean.png
      │
      ├── Etapa 2: WP1 Sorting
      │
      └── ...
```

O módulo é chamado pelo `run_full_pipeline.py` na **Etapa 1** para gerar imagens térmicas limpas que serão usadas pelo modelo YOLO na detecção de keypoints.

---

## 2. Localização e Execução

### 2.1 Caminho do Arquivo

```
qe_solar_wires/src/tools/clean_ui.py
```

**Nota:** O arquivo também pode ser encontrado como `2-clean_ui.py` em alguns ambientes.

### 2.2 Execução como Módulo

```bash
python -m src.tools.clean_ui --input <dir> --output <dir>
```

### 2.3 Execução Direta

```bash
python src/tools/clean_ui.py --input <dir> --output <dir>
```

---

## 3. Dependências

### 3.1 Bibliotecas Python

```python
# Standard Library
from pathlib import Path
import argparse
import json
from typing import Dict, List, Tuple, Optional
from datetime import datetime
import logging

# Third-party (obrigatórias)
import numpy as np          # Manipulação de arrays
import cv2                  # Processamento de imagem
from tqdm import tqdm       # Barra de progresso

# Third-party (opcional com fallback)
from flirimageextractor import FlirImageExtractor
```

### 3.2 Verificação de Dependências

```python
try:
    from flirimageextractor import FlirImageExtractor
except ImportError:
    logger.error("flirimageextractor não instalado!")
    FlirImageExtractor = None
```

### 3.3 Instalação

```bash
pip install numpy opencv-python tqdm flirimageextractor
```

### 3.4 Requisitos de Sistema

- Python 3.8+
- OpenCV com suporte a colormaps (cv2.COLORMAP_*)

---

## 4. Classe FLIRThermalAnalyzer

### 4.1 Visão Geral

Classe principal que encapsula toda a lógica de análise térmica.

```python
class FLIRThermalAnalyzer:
    """
    Analisador térmico otimizado para equipamentos elétricos/industriais
    """
```

### 4.2 Construtor

```python
def __init__(self, colormap: str = "inferno", clip_percent: float = 1.0):
    """
    Args:
        colormap: Nome do colormap para visualização
        clip_percent: Percentual de recorte para evitar outliers
    """
```

**Parâmetros:**

| Parâmetro | Tipo | Default | Descrição |
|-----------|------|---------|-----------|
| `colormap` | str | `"inferno"` | Colormap OpenCV |
| `clip_percent` | float | `1.0` | Percentil de clipping (ex: 1.0 = P1-P99) |

**Colormaps Suportados:**

| Nome | Constante OpenCV | Aparência |
|------|------------------|-----------|
| `jet` | `cv2.COLORMAP_JET` | Arco-íris (azul→vermelho) |
| `turbo` | `cv2.COLORMAP_TURBO` | Similar a jet, mais suave |
| `hot` | `cv2.COLORMAP_HOT` | Preto→vermelho→amarelo→branco |
| `magma` | `cv2.COLORMAP_MAGMA` | Preto→roxo→laranja→amarelo |
| `inferno` | `cv2.COLORMAP_INFERNO` | Preto→roxo→laranja→amarelo |
| `plasma` | `cv2.COLORMAP_PLASMA` | Azul→roxo→laranja→amarelo |

**Atributos de Instância:**

```python
self.colormap       # Nome do colormap
self.clip_percent   # Percentual de clipping
self.fie            # Instância FlirImageExtractor
self.thermal_data   # Última matriz térmica extraída
self.metadata       # Metadados EXIF da última imagem
self.colormap_dict  # Mapeamento nome → constante OpenCV
```

---

### 4.3 Método: `extract_thermal_data`

Extrai dados térmicos de uma imagem FLIR.

```python
def extract_thermal_data(self, image_path: Path) -> Optional[np.ndarray]:
    """
    Extrai dados térmicos da imagem FLIR
    
    Args:
        image_path: Caminho para imagem FLIR
        
    Returns:
        Array numpy com temperaturas em Celsius (float32)
        None se falhar
    """
```

**Processo interno:**

1. Cria instância `FlirImageExtractor(is_debug=False)`
2. Processa imagem com `process_image()`
3. Extrai matriz térmica com `get_thermal_np()`
4. Tenta extrair metadados com `get_metadata()`
5. Retorna matriz ou None se falhar

**Formato de retorno:**

```python
# Shape: (height, width)
# Dtype: float32 ou float64
# Valores: temperaturas em °C

thermal_data = analyzer.extract_thermal_data(path)
# Ex: thermal_data[100, 200] = 45.3  # 45.3°C no pixel (200, 100)
```

**Tratamento de erros:**

- Retorna `None` se `FlirImageExtractor` não disponível
- Retorna `None` se imagem não tem dados radiométricos
- Loga warning/error conforme o caso

---

### 4.4 Método: `render_clean_thermal`

Renderiza imagem térmica limpa com colormap.

```python
def render_clean_thermal(self, 
                        thermal: np.ndarray,
                        vmin: Optional[float] = None,
                        vmax: Optional[float] = None) -> np.ndarray:
    """
    Renderiza imagem térmica limpa (sem UI/HUD)
    
    Args:
        thermal: Matriz de temperaturas (HxW)
        vmin: Temperatura mínima para visualização
        vmax: Temperatura máxima para visualização
        
    Returns:
        Imagem BGR renderizada (HxWx3, uint8)
    """
```

**Processo interno:**

```
1. Cópia da matriz térmica
         │
         ▼
2. Cálculo de limites (se não fornecidos)
   ├── clip_percent > 0: usa percentis
   │     lo = np.percentile(th, clip_percent)
   │     hi = np.percentile(th, 100 - clip_percent)
   └── clip_percent = 0: usa min/max absolutos
         │
         ▼
3. Normalização para 0-1
   th_norm = (th - vmin) / (vmax - vmin)
         │
         ▼
4. Conversão para uint8 (0-255)
         │
         ▼
5. Aplicação de colormap
   rgb = cv2.applyColorMap(th_u8, colormap)
         │
         ▼
6. Retorna imagem BGR
```

**Exemplo de uso:**

```python
thermal = analyzer.extract_thermal_data(img_path)
rgb = analyzer.render_clean_thermal(thermal)
cv2.imwrite("clean.png", rgb)

# Com limites customizados
rgb = analyzer.render_clean_thermal(thermal, vmin=20.0, vmax=60.0)
```

---

### 4.5 Método: `detect_hotspots`

Detecta pontos quentes na imagem térmica.

```python
def detect_hotspots(self, 
                    thermal: np.ndarray,
                    threshold_method: str = "adaptive",
                    threshold_value: float = None,
                    delta_c: float = 5.0,
                    min_abs_c: float = 45.0,
                    min_area_px: int = 8,
                    top_k: int = 50) -> List[Dict]:
    """
    Detecta hotspots com limiar adaptativo robusto.
    
    Args:
        thermal: Matriz de temperaturas
        threshold_method: Método de limiar (não usado atualmente)
        threshold_value: Valor de limiar (não usado atualmente)
        delta_c: Delta acima da mediana (°C)
        min_abs_c: Temperatura mínima absoluta (°C)
        min_area_px: Área mínima do hotspot (pixels)
        top_k: Máximo de hotspots a retornar
        
    Returns:
        Lista de dicionários com informações dos hotspots
    """
```

**Algoritmo de detecção:**

```
1. Cálculo do limiar adaptativo
   thr = max(mediana + delta_c, P95, min_abs_c)
         │
         ▼
2. Criação de máscara binária
   mask = (thermal > thr)
         │
         ▼
3. Operação morfológica (closing)
   kernel 3x3 elíptico
         │
         ▼
4. Detecção de contornos
   cv2.findContours(RETR_EXTERNAL)
         │
         ▼
5. Filtragem por área mínima
         │
         ▼
6. Extração de estatísticas por ROI
         │
         ▼
7. Classificação de severidade
         │
         ▼
8. Fallback pontual (se nenhum blob mas Tmax > min_abs_c)
         │
         ▼
9. Ordenação por temperatura e corte top-k
```

**Estrutura de retorno:**

```python
[
    {
        'bbox': (x, y, w, h),        # Bounding box
        'center': (cx, cy),          # Centro do hotspot
        'area': int,                 # Área em pixels
        'max_temp': float,           # Temperatura máxima
        'mean_temp': float,          # Temperatura média
        'min_temp': float,           # Temperatura mínima
        'threshold_used': float,     # Limiar utilizado
        'severity': str,             # 'CRÍTICO'|'ALTO'|'MÉDIO'|'BAIXO'|'PONTUAL'
        'color': (B, G, R)           # Cor para visualização
    },
    ...
]
```

**Classificação de severidade:**

| Severidade | Delta (Tmax - Tmean) | Cor (BGR) |
|------------|----------------------|-----------|
| CRÍTICO | > 30°C | (0, 0, 255) Vermelho |
| ALTO | > 20°C | (0, 128, 255) Laranja |
| MÉDIO | > 10°C | (0, 255, 255) Amarelo |
| BAIXO | ≤ 10°C | (0, 255, 0) Verde |
| PONTUAL | Fallback | (255, 0, 255) Magenta |

---

### 4.6 Método: `analyze_electrical_panel`

Análise específica para painéis elétricos.

```python
def analyze_electrical_panel(self, thermal: np.ndarray) -> Dict:
    """
    Análise específica para painéis elétricos
    
    Args:
        thermal: Matriz de temperaturas
        
    Returns:
        Dicionário com diagnóstico completo
    """
```

**Estrutura de retorno:**

```python
{
    'timestamp': str,              # ISO timestamp
    'statistics': {
        'min_temp': float,
        'max_temp': float,
        'mean_temp': float,
        'std_temp': float,
        'median_temp': float
    },
    'hotspots': List[Dict],        # Lista de hotspots
    'hotspot_count': int,
    'max_delta': float,            # Tmax - Tmean
    'alerts': [
        {
            'level': str,          # 'CRÍTICO'|'ALTO'|'MÉDIO'
            'message': str,
            'action': str
        }
    ],
    'recommendations': List[str]
}
```

**Regras de alerta:**

| Tmax | Nível | Ação |
|------|-------|------|
| > 70°C | CRÍTICO | Inspeção imediata |
| > 60°C | ALTO | Manutenção preventiva |
| > 50°C | MÉDIO | Monitorar evolução |
| > 5 hotspots | MÉDIO | Verificar balanceamento |

---

### 4.7 Método: `visualize_analysis`

Cria visualização completa da análise.

```python
def visualize_analysis(self, 
                      thermal: np.ndarray,
                      rgb_clean: np.ndarray,
                      analysis: Dict,
                      output_path: Optional[Path] = None) -> np.ndarray:
    """
    Cria visualização completa da análise
    
    Args:
        thermal: Matriz de temperaturas
        rgb_clean: Imagem térmica renderizada
        analysis: Resultado de analyze_electrical_panel()
        output_path: Caminho para salvar (opcional)
        
    Returns:
        Imagem de visualização (canvas)
    """
```

**Layout do canvas:**

```
┌─────────────────────┬─────────────────────┐
│                     │                     │
│   Imagem Térmica    │  Imagem Anotada     │
│      Limpa          │   (com hotspots)    │
│                     │                     │
├─────────────────────┴─────────────────────┤
│                                           │
│  Min: XX.X°C  Max: XX.X°C  Hotspots: N    │
│  ALERTA: Mensagem de alerta               │
│                                           │
└───────────────────────────────────────────┘
```

**Dimensões:**
- Largura: `width * 2`
- Altura: `height + 100`

---

## 5. Funções de Processamento

### 5.1 Função Auxiliar: `_np_encoder`

Encoder JSON para tipos NumPy.

```python
def _np_encoder(o):
    """
    Converte tipos NumPy em tipos nativos Python para JSON
    
    Args:
        o: Objeto a converter
        
    Returns:
        Versão serializável do objeto
    """
    import numpy as _np
    if isinstance(o, (_np.integer,)):  return int(o)
    if isinstance(o, (_np.floating,)): return float(o)
    if isinstance(o, _np.ndarray):     return o.tolist()
    return str(o)  # fallback
```

**Uso:**
```python
json.dumps(data, default=_np_encoder)
```

---

### 5.2 Função: `process_batch`

Processa lote de imagens FLIR.

```python
def process_batch(input_dir: Path, 
                  output_dir: Path,
                  colormap: str = "inferno",
                  save_npy: bool = False,
                  save_analysis: bool = True,
                  limit: int = 0) -> Dict:
    """
    Processa lote de imagens FLIR
    
    Args:
        input_dir: Diretório de entrada
        output_dir: Diretório de saída
        colormap: Colormap para renderização
        save_npy: Salvar matrizes .npy (não implementado)
        save_analysis: Realizar análise de hotspots (não usado)
        limit: Limitar número de imagens (0 = sem limite)
        
    Returns:
        Dicionário com estatísticas de processamento
    """
```

**Processo:**

```
1. Criar instância FLIRThermalAnalyzer
         │
         ▼
2. Criar diretório de saída
         │
         ▼
3. Listar imagens (recursivo)
   Extensões: .jpg, .jpeg, .png, .tif, .tiff
         │
         ▼
4. Aplicar limite (se especificado)
         │
         ▼
5. Para cada imagem:
   ├── Extrair dados térmicos
   ├── Se falhar: incrementar 'failed', continuar
   ├── Renderizar imagem limpa
   ├── Salvar como <stem>_clean.png
   └── Incrementar 'processed'
         │
         ▼
6. Retornar estatísticas
```

**Retorno:**

```python
{
    'processed': int,  # Imagens processadas com sucesso
    'failed': int      # Imagens que falharam
}
```

**Nomenclatura de saída:**

```
input:  Raw Images/IR_54393.jpg
output: output_dir/IR_54393_clean.png
```

---

### 5.3 Função: `main`

Entry point com parsing de argumentos.

```python
def main():
    parser = argparse.ArgumentParser(...)
    args = parser.parse_args()
    
    # Validação
    if not input_dir.exists():
        logger.error(f"Diretório não encontrado")
        return
    
    # Processar
    results = process_batch(
        input_dir,
        output_dir,
        colormap=args.colormap,
        save_npy=args.save_npy,
        save_analysis=not args.no_analysis,
        limit=args.limit
    )
    
    logger.info(f"✅ Processamento concluído!")
```

---

## 6. Argumentos CLI

### 6.1 Tabela de Argumentos

| Argumento | Obrigatório | Default | Descrição |
|-----------|-------------|---------|-----------|
| `--input` | ✅ | - | Diretório com imagens FLIR |
| `--output` | ✅ | - | Diretório de saída |
| `--colormap` | ❌ | `"inferno"` | Colormap para renderização |
| `--save-npy` | ❌ | `False` | Salvar matrizes .npy |
| `--no-analysis` | ❌ | `False` | Pular análise de hotspots |
| `--limit` | ❌ | `0` | Limitar número de imagens |

### 6.2 Valores de Colormap

```bash
--colormap jet      # Arco-íris clássico
--colormap turbo    # Arco-íris suave
--colormap hot      # Preto→vermelho→amarelo→branco
--colormap magma    # Escuro para claro (perceptualmente uniforme)
--colormap inferno  # Similar ao magma (PADRÃO)
--colormap plasma   # Azul→roxo→laranja
```

---

## 7. Fluxo de Execução

```
main()
  │
  ├── argparse: parsing de argumentos
  │
  ├── Validação: input_dir existe?
  │
  └── process_batch()
        │
        ├── FLIRThermalAnalyzer(colormap)
        │
        ├── output_dir.mkdir()
        │
        ├── Listar imagens (rglob)
        │
        └── Para cada imagem (com tqdm):
              │
              ├── extract_thermal_data()
              │     │
              │     ├── FlirImageExtractor.process_image()
              │     │
              │     └── FlirImageExtractor.get_thermal_np()
              │
              ├── render_clean_thermal()
              │     │
              │     ├── Calcular percentis
              │     │
              │     ├── Normalizar para 0-255
              │     │
              │     └── cv2.applyColorMap()
              │
              └── cv2.imwrite(<stem>_clean.png)
```

---

## 8. Formatos de Saída

### 8.1 Imagem Limpa

**Nome:** `<stem>_clean.png`

**Formato:**
- PNG 8-bit
- 3 canais (BGR)
- Mesma resolução da matriz térmica original

**Exemplo:**
```
IR_54393.jpg  →  IR_54393_clean.png
```

### 8.2 Estrutura de Diretório

```
output_dir/
├── IR_54393_clean.png
├── IR_54394_clean.png
├── IR_54395_clean.png
└── ...
```

**Nota:** Não há subdiretórios - todas as imagens ficam na raiz do output.

---

## 9. Exemplos de Uso

### 9.1 Uso Básico

```bash
python -m src.tools.clean_ui \
    --input "data/Site/Raw Images" \
    --output "output/02_thermal_clean"
```

### 9.2 Com Colormap Diferente

```bash
python -m src.tools.clean_ui \
    --input "data/Site/Raw Images" \
    --output "output/clean_jet" \
    --colormap jet
```

### 9.3 Processamento Limitado (teste)

```bash
python -m src.tools.clean_ui \
    --input "data/Site/Raw Images" \
    --output "output/test_clean" \
    --limit 5
```

### 9.4 Uso Programático

```python
from pathlib import Path
from src.tools.clean_ui import FLIRThermalAnalyzer

analyzer = FLIRThermalAnalyzer(colormap="inferno", clip_percent=1.0)

# Extrair dados térmicos
thermal = analyzer.extract_thermal_data(Path("image.jpg"))

if thermal is not None:
    # Renderizar imagem limpa
    rgb = analyzer.render_clean_thermal(thermal)
    
    # Salvar
    import cv2
    cv2.imwrite("clean.png", rgb)
    
    # Detectar hotspots
    hotspots = analyzer.detect_hotspots(thermal)
    
    # Análise completa
    analysis = analyzer.analyze_electrical_panel(thermal)
```

---

## 10. Integração com Pipeline

### 10.1 Chamada pelo run_full_pipeline.py

```python
# Em step_1_clean_thermal()
cmd = [
    sys.executable, "-m", "src.tools.clean_ui",
    "--input", str(config.raw_images_dir),
    "--output", str(clean_dir),
    "--colormap", "inferno",
]

proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
```

### 10.2 Fluxo de Dados

```
Raw Images/           clean_ui.py        02_thermal_clean/
     │                    │                    │
     ├── IR_54393.jpg ────┼───────────────►    ├── IR_54393_clean.png
     ├── IR_54394.jpg ────┼───────────────►    ├── IR_54394_clean.png
     └── IR_54395.jpg ────┴───────────────►    └── IR_54395_clean.png
```

### 10.3 Consumidores

| Etapa | Módulo | Uso |
|-------|--------|-----|
| WP4+WP5 | `wp4_wp5_integration_v4.py` | Input para YOLO (`--yolo-image`) |
| WP4+WP5 | `wp4_wp5_integration_v4.py` | Input thermal clean (`--thermal-clean-image`) |

---

## 11. Notas de Implementação

### 11.1 Design Decisions

1. **Colormap Inferno como padrão**
   - Perceptualmente uniforme
   - Bom contraste para detecção de anomalias
   - Compatível com daltonismo

2. **Clipping por percentis**
   - Evita que outliers dominem a visualização
   - P1-P99 padrão (1% de clipping)
   - Melhora contraste em imagens com ruído

3. **Fallback pontual em hotspots**
   - Se nenhum blob detectado mas Tmax > min_abs_c
   - Garante que anomalias pontuais não sejam perdidas

### 11.2 Limitações Conhecidas

1. **Parâmetros não utilizados**
   - `save_npy`: Flag existe mas não está implementada
   - `save_analysis`: Flag existe mas análise não é salva
   - `threshold_method`/`threshold_value`: Não usados em detect_hotspots

2. **Estrutura de saída plana**
   - Todas as imagens no mesmo diretório
   - Pode causar conflitos se stems duplicados

3. **Sem preservação de metadados**
   - Imagem de saída não contém EXIF original
   - Metadados são extraídos mas não salvos

### 11.3 Performance

- **Tempo típico:** ~0.5-1.5s por imagem (dependendo do tamanho)
- **Memória:** Carrega uma imagem por vez
- **I/O:** Escrita PNG não comprimida

### 11.4 Tratamento de Erros

| Situação | Comportamento |
|----------|---------------|
| FlirImageExtractor não instalado | Log error, retorna None |
| Imagem sem dados radiométricos | Log warning, continua |
| Erro de leitura | Log error, incrementa 'failed' |
| Diretório de entrada não existe | Log error, aborta |

### 11.5 Compatibilidade de Colormaps

Alguns colormaps podem não estar disponíveis em versões antigas do OpenCV:

```python
# Fallback implementado
"magma": cv2.COLORMAP_MAGMA if hasattr(cv2, "COLORMAP_MAGMA") else cv2.COLORMAP_HOT,
"inferno": cv2.COLORMAP_INFERNO if hasattr(cv2, "COLORMAP_INFERNO") else cv2.COLORMAP_HOT,
"plasma": cv2.COLORMAP_PLASMA if hasattr(cv2, "COLORMAP_PLASMA") else cv2.COLORMAP_HOT,
```

---

## Changelog

### v1.0 (Fevereiro 2025)
- Documentação inicial
- Renderização limpa estável
- Detecção de hotspots com limiar adaptativo

---

## Referências

- [flirimageextractor](https://github.com/nationaldronesau/FlirImageExtractor)
- [OpenCV Colormaps](https://docs.opencv.org/4.x/d3/d50/group__imgproc__colormap.html)
- [FLIR Radiometric JPEG](https://flir.custhelp.com/app/answers/detail/a_id/2127)

---

*Documentação gerada para handoff do projeto QE Solar.*
