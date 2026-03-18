# prepare_yolo_dataset_from_exif_spots.py

## Documentação Técnica

**Módulo:** `src.tools.prepare_yolo_dataset_from_exif_spots`

---

## 1. Descrição

O `prepare_yolo_dataset_from_exif_spots.py` prepara um dataset no formato **YOLO detect** a partir de *spots* existentes no **EXIF de imagens FLIR** (`FLIR:MeasXParams`).

O script suporta dois modos principais:

- Usar a imagem de entrada diretamente (cópia/sem cópia)
- Renderizar uma imagem térmica **clean UI** a partir do radiométrico (`--render-clean-ui`) e usar essa renderização como imagem de treino (PNG)

Esse segundo modo é o recomendado quando o pipeline faz inferência em `thermal_clean` (domínio alinhado treino vs inferência).

---

## 2. Estrutura do Dataset Gerado

O output em `--output-dataset` segue o padrão esperado pelo Ultralytics:

```text
<output-dataset>/
  images/
    <stem>_clean.png
  labels/
    <stem>_clean.txt
  splits/
    train.txt
    val.txt
  dataset.yaml            (opcional, pode ser gerado pelo train_yolo_wp4.py)
  prepare_stats.json
```

- `labels/*.txt` seguem o formato YOLO detect: `class cx cy w h` (normalizado 0-1)
- `splits/*.txt` contêm caminhos absolutos para as imagens

---

## 3. Requisitos

- `exiftool` instalado e acessível no PATH
- Python com:
  - `opencv-python`
  - `flirimageextractor` (necessário para `--render-clean-ui`)

---

## 4. Uso

### 4.1 Preparar dataset usando imagens Clean UI (recomendado)

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

### 4.2 Preparar dataset sem render (copiando a imagem de entrada)

```bash
python -m src.tools.prepare_yolo_dataset_from_exif_spots \
  --input-images /path/to/images \
  --output-dataset /path/to/out_dataset \
  --bbox-size 10
```

---

## 5. Argumentos CLI (principais)

- `--input-images`
  - Diretório com imagens FLIR (ex: `Edited/`) contendo spots no EXIF

- `--output-dataset`
  - Diretório do dataset de saída

- `--bbox-size`
  - Tamanho (px) do quadrado gerado em volta do spot

- `--render-clean-ui`
  - Renderiza `*_clean.png` via radiometria e aplica colormap

- `--render-colormap`
  - Colormap do OpenCV aplicado na renderização (`inferno` default)

- `--render-clip-percent`
  - Clipping por percentil antes de normalizar e colorizar

- `--skip-exiftool-errors`
  - Em caso de falha no `exiftool`, pula a imagem ao invés de abortar

- `--skip-render-errors`
  - Em caso de falha no render radiométrico (`flirimageextractor`/PIL), pula a imagem

- `--only-thermal-320x240`
  - Filtra o dataset para somente imagens com `RawThermalImageWidth/Height` == `320x240` ou `240x320`

---

## 6. Observações importantes

### 6.1 Coordenadas e escala

- Os spots (`MeasXParams`) estão no espaço do **RawThermalImageWidth/Height**.
- O script faz *rescale* para o tamanho real da imagem (especialmente relevante quando a imagem está em 240x320 vs 320x240, etc.).

### 6.2 Por que usar `--only-thermal-320x240`

Ao treinar com imagens de várias resoluções térmicas (ex: 160x120 misturado), é comum ocorrer:

- inconsistência visual
- labels “aparentemente erradas”
- pior generalização

Filtrar para um domínio único ajuda a estabilizar o dataset.

### 6.3 Render failures

Erros como `PIL.UnidentifiedImageError` podem acontecer em alguns JPEGs “Edited” que não possuem stream térmico radiométrico compatível. Nesses casos, use `--skip-render-errors` e acompanhe o contador `skipped_render_errors` no `prepare_stats.json`.

---

## 7. Saída e métricas

O script sempre grava `prepare_stats.json` com contagens como:

- `images_with_spots`
- `skipped_render_errors`
- `skipped_bad_thermal_size`
- `train_images` / `val_images`

Esse arquivo é útil para validar se o dataset ficou “grande o suficiente” antes do treino.
