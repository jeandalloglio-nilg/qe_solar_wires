# viz_exif_spots_on_clean.py

## Documentação Técnica

**Módulo:** `src.tools.viz_exif_spots_on_clean`

---

## 1. Descrição

O `viz_exif_spots_on_clean.py` é uma ferramenta de validação visual para:

- desenhar *spots* do EXIF FLIR (`MeasXParams`) em cima de uma imagem térmica limpa (`*_clean.png`)
- desenhar bboxes de um label YOLO (`labels/*.txt`) em cima de uma imagem (normalmente `*_clean.png`)
- comparar lado a lado a imagem **Edited** original e a imagem **clean** com labels desenhadas

Este script foi pensado para validar rapidamente:

- alinhamento de coordenadas
- domínio de treino (Edited vs Clean)
- consistência do dataset gerado

---

## 2. Requisitos

- Python com `opencv-python`
- `exiftool` instalado (somente para `--mode exif`)

---

## 3. Modos de uso

### 3.1 `--mode exif`

- Lê spots do EXIF (`MeasXParams`) a partir de `--exif-source`
- Desenha caixas (tamanho `--bbox-size`) e o centro do spot

Exemplo:

```bash
python -m src.tools.viz_exif_spots_on_clean \
  --mode exif \
  --exif-source /path/to/Edited/FLIR0001.jpg \
  --clean-image /path/to/thermal_clean/FLIR0001_clean.png \
  --bbox-size 10 \
  --draw-ids \
  --no-save
```

Observação: também é possível renderizar uma imagem clean em memória a partir da radiometria com `--render-clean-ui` (somente em `--mode exif`).

### 3.2 `--mode yolo`

- Lê um arquivo `.txt` no formato YOLO detect (`class cx cy w h`, normalizado)
- Desenha bboxes e o centro

Exemplo:

```bash
python -m src.tools.viz_exif_spots_on_clean \
  --mode yolo \
  --clean-image /path/to/dataset/images/FLIR0001_clean.png \
  --yolo-label /path/to/dataset/labels/FLIR0001_clean.txt \
  --draw-ids \
  --no-save
```

---

## 4. Comparação Edited vs Clean+labels (lado a lado)

O fluxo típico de validação do dataset `all_clean_bbox_10` é:

- esquerda: imagem `Edited/*.jpg`
- direita: imagem `data/.../images/*_clean.png` com labels desenhadas

Em ambientes com display, o script pode abrir uma janela com `--show`.

Em ambientes headless (sem `DISPLAY`), `--show` não abre janela e gera automaticamente um PNG temporário em `/tmp`.

Exemplo:

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

Saída esperada (headless):

```text
headless_show_written: /home/jean/qe_solar_wires/tmp/compare_FLIR3243.png
```

---

## 5. Flags principais

- `--mode`
  - `exif`: desenha spots do EXIF
  - `yolo`: desenha labels YOLO

- `--clean-image`
  - imagem base onde desenhar overlays

- `--edited-image`
  - (opcional) somente para comparação lado a lado no `--show`

- `--yolo-label`
  - label `.txt` YOLO (somente `--mode yolo`)

- `--bbox-size`
  - tamanho do bbox para o modo EXIF

- `--draw-ids`
  - desenha índices/ids ao lado de cada bbox

- `--show`
  - mostra o resultado
  - se não houver `DISPLAY`, gera PNG temporário e imprime o caminho

- `--no-save`
  - não grava o overlay da imagem “clean” com bboxes no disco

- `--tmp-output`
  - usado no modo headless com `--show` para escolher o caminho do PNG temporário

---

## 6. Troubleshooting

### 6.1 `qt.qpa.xcb: could not connect to display`

- Causa: ambiente headless sem `DISPLAY`.
- Solução: use `--show` normalmente e o script vai gerar um PNG temporário (fallback headless). Se quiser controlar o caminho, use `--tmp-output`.

### 6.2 Label aparece deslocada

Normalmente indica:

- mismatch de resolução entre coordenadas e imagem
- dataset com múltiplas resoluções térmicas misturadas

Sugestão:

- preparar dataset com `--only-thermal-320x240`
- validar múltiplas imagens com comparação lado a lado
