# wp6_repx_generator.py

## Documentação Técnica

**Módulo:** `src.tools.wp6_repx_generator`  
**Versão:** v2.1  
**Autor:** Elton Gino / QE Solar  
**Última atualização:** Fevereiro 2025

---

## Sumário

1. [Descrição](#1-descrição)
2. [Localização e Execução](#2-localização-e-execução)
3. [Dependências](#3-dependências)
4. [Formato REPX](#4-formato-repx)
5. [Funções Utilitárias](#5-funções-utilitárias)
6. [Função Principal](#6-função-principal)
7. [Argumentos CLI](#7-argumentos-cli)
8. [Fluxo de Execução](#8-fluxo-de-execução)
9. [Estrutura do REPX](#9-estrutura-do-repx)
10. [Exemplos de Uso](#10-exemplos-de-uso)
11. [Integração com Pipeline](#11-integração-com-pipeline)
12. [Notas de Implementação](#12-notas-de-implementação)

---

## 1. Descrição

### 1.1 Propósito

O `wp6_repx_generator.py` implementa o **WP6 (Work Package 6)** do pipeline QE Solar: geração de relatórios em formato `.repx` compatíveis com **FLIR Tools**.

### 1.2 O que é REPX?

O formato `.repx` é o formato nativo de relatórios do FLIR Tools. É essencialmente um arquivo ZIP com estrutura específica contendo:

- Imagens térmicas FLIR (com spots de medição)
- Metadados XML
- Arquivos de relacionamento (.rels)
- Logo e previews

### 1.3 Funcionalidades

| Funcionalidade | Descrição |
|----------------|-----------|
| **Geração de REPX** | Cria arquivo ZIP com estrutura FLIR |
| **Múltiplas imagens** | Suporta qualquer número de imagens |
| **Logo customizado** | PNG ou JPG (converte automaticamente) |
| **Compatibilidade FLIR** | XML em UTF-16, .rels em UTF-8 com BOM |

### 1.4 Contexto no Pipeline

```
Pipeline QE Solar
      │
      ├── Etapa 0-3: Preparação
      │
      ├── Etapa 4: WP4+WP5 Analysis
      │       │
      │       └── Output: 05_wp4_wp5_analysis/edited/
      │             └── *_with_spots.jpg
      │
      └── Etapa 5: WP6 Report  ◄─── ESTE MÓDULO
              │
              └── Output: 06_report/
                    └── report.repx
```

---

## 2. Localização e Execução

### 2.1 Caminho do Arquivo

```
qe_solar_wires/src/tools/wp6_repx_generator.py
```

### 2.2 Execução como Módulo

```bash
python -m src.tools.wp6_repx_generator \
    --edited-dir "05_wp4_wp5_analysis/edited" \
    --output "06_report/report.repx"
```

### 2.3 Execução Direta

```bash
python src/tools/wp6_repx_generator.py \
    --edited-dir "05_wp4_wp5_analysis/edited" \
    --output "06_report/report.repx"
```

---

## 3. Dependências

### 3.1 Bibliotecas Python

```python
# Standard Library (todas incluídas no Python)
import argparse
import shutil
import uuid
import zipfile
import tempfile
import sys
from pathlib import Path
from typing import List, Optional

# Third-party (opcional)
from PIL import Image  # Apenas para conversão PNG→JPG
```

### 3.2 Dependência Opcional

| Biblioteca | Necessário | Uso |
|------------|------------|-----|
| `Pillow` | ⚠️ Opcional | Conversão de logo PNG para JPG |

Se Pillow não estiver instalado e o logo for PNG, a conversão falhará e será usado um logo placeholder.

### 3.3 Instalação

```bash
# Mínimo (sem conversão de logo PNG)
# Nenhuma instalação necessária

# Completo (com suporte a logo PNG)
pip install Pillow
```

### 3.4 Ferramentas Externas

Nenhuma ferramenta externa é necessária. O script é completamente autocontido.

---

## 4. Formato REPX

### 4.1 Visão Geral

O formato REPX é baseado no padrão **Open Packaging Conventions (OPC)**, similar a .docx, .xlsx, etc.

```
report.repx (arquivo ZIP)
│
├── [Content_Types].xml      # Tipos MIME
├── _rels/
│   └── .rels               # Relacionamento raiz
│
└── Report/
    ├── report.xml          # Documento principal
    ├── _rels/
    │   └── report.xml.rels # Relacionamentos do report
    ├── Media/
    │   ├── Logo.jpg        # Logo do relatório
    │   ├── PagePreview_0   # Preview (JPEG sem extensão)
    │   └── <uuid>.jpg      # Imagens térmicas
    │
    └── Sections/
        ├── section1.xml    # Seção 1
        ├── section2.xml    # Seção 2
        └── _rels/
            ├── section1.xml.rels
            └── section2.xml.rels
```

### 4.2 Tipos de Relationship

| Tipo | URI |
|------|-----|
| Documento raiz | `http://flir.com/reportdocument` |
| Seção | `http://flir.com/reportdocument/section` |
| Imagem | `http://schemas.openxmlformats.org/officeDocument/2006/relationships/image` |

### 4.3 Codificações

| Arquivo | Codificação | BOM |
|---------|-------------|-----|
| `*.xml` (conteúdo) | UTF-16 LE | ✅ Sim |
| `*.rels` | UTF-8 | ✅ Sim |
| `[Content_Types].xml` | UTF-8 | ✅ Sim |

### 4.4 Formato de IDs

Os IDs seguem o padrão FLIR:
```
R + 16 caracteres hexadecimais
Exemplo: R1a2b3c4d5e6f7890
```

---

## 5. Funções Utilitárias

### 5.1 `generate_rid() -> str`

Gera um ID no formato FLIR.

```python
def generate_rid() -> str:
    """Gera um ID no formato FLIR: R + 16 hex chars."""
    return f"R{uuid.uuid4().hex[:16]}"

# Exemplo: "R1a2b3c4d5e6f7890"
```

### 5.2 `write_utf16_xml(path: Path, content: str) -> None`

Escreve XML em UTF-16 LE com BOM (formato FLIR para conteúdo).

```python
def write_utf16_xml(path: Path, content: str) -> None:
    """Escreve XML em UTF-16 LE com BOM (formato FLIR)."""
    content = content.strip()  # Minificar
    path.write_bytes(content.encode('utf-16'))
```

**Nota:** `encode('utf-16')` em Python automaticamente adiciona BOM.

### 5.3 `write_utf8_xml(path: Path, content: str) -> None`

Escreve XML em UTF-8 com BOM (formato FLIR para .rels).

```python
def write_utf8_xml(path: Path, content: str) -> None:
    """Escreve XML em UTF-8 com BOM (formato FLIR para .rels)."""
    content = content.strip()  # Minificar
    bom = b'\xef\xbb\xbf'  # UTF-8 BOM
    path.write_bytes(bom + content.encode('utf-8'))
```

### 5.4 `create_default_logo(output_path: Path) -> None`

Cria um logo placeholder (1x1 pixel branco JPEG).

```python
def create_default_logo(output_path: Path) -> None:
    """Cria um logo placeholder (1x1 pixel branco JPEG)."""
    minimal_jpeg = bytes([
        0xFF, 0xD8, 0xFF, 0xE0, ...  # JPEG mínimo válido
    ])
    output_path.write_bytes(minimal_jpeg)
```

**Uso:** Quando nenhum logo é fornecido ou a conversão falha.

### 5.5 `copy_logo_as_jpg(logo_path: Path, dest_path: Path) -> bool`

Copia logo para destino, convertendo PNG para JPG se necessário.

```python
def copy_logo_as_jpg(logo_path: Path, dest_path: Path) -> bool:
    """
    Copia logo para destino, convertendo PNG para JPG se necessário.
    Returns True se sucesso.
    """
    if logo_path.suffix.lower() == ".png":
        # Converter PNG para JPG via Pillow
        from PIL import Image as PILImage
        with PILImage.open(logo_path) as img:
            # Remover canal alpha se presente
            if img.mode in ('RGBA', 'LA', 'P'):
                background = PILImage.new('RGB', img.size, (255, 255, 255))
                # ... composição ...
            img.save(dest_path, 'JPEG', quality=95)
        return True
    else:
        # Copiar diretamente
        shutil.copy2(logo_path, dest_path)
        return True
```

**Tratamento de alpha:**
- RGBA → Composto sobre fundo branco
- LA (grayscale + alpha) → Composto sobre fundo branco
- P (paleta) → Convertido para RGBA primeiro

---

## 6. Função Principal

### 6.1 `generate_repx(...) -> bool`

Função principal que gera o arquivo REPX.

```python
def generate_repx(
    edited_dir: Path,                              # Pasta com imagens
    output_path: Path,                             # Caminho do .repx
    site_name: str = "Thermal Inspection Report",  # Nome do relatório
    logo_path: Optional[Path] = None,              # Logo customizado
) -> bool:
```

**Retorno:** `True` se sucesso, `False` se falha.

### 6.2 Processo Interno

```
generate_repx()
  │
  ├── Buscar imagens (*_with_spots.jpg ou *.jpg)
  │
  ├── Criar diretório temporário
  │
  ├── Criar estrutura de pastas
  │     ├── Report/Sections/_rels/
  │     ├── Report/Media/
  │     ├── Report/_rels/
  │     └── _rels/
  │
  ├── Copiar/criar logo
  │     ├── Logo fornecido (PNG→JPG se necessário)
  │     └── Logo placeholder (1x1 branco)
  │
  ├── Criar preview placeholder
  │
  ├── Para cada imagem:
  │     │
  │     ├── Gerar IDs (section, ir_image, ir_table, visual_image, visual_table)
  │     │
  │     ├── Copiar imagem para Media/<uuid>.jpg
  │     │
  │     ├── Criar sectionN.xml
  │     │     └── 4 SectionContentItems (IR, table, Visual, table)
  │     │
  │     ├── Criar sectionN.xml.rels
  │     │
  │     └── Acumular referências para report.xml
  │
  ├── Criar report.xml
  │     └── Referências a todas as seções
  │
  ├── Criar report.xml.rels
  │     └── Relationships para logo, preview, seções
  │
  ├── Criar _rels/.rels
  │     └── Relationship raiz para report.xml
  │
  ├── Criar [Content_Types].xml
  │
  └── Criar ZIP final (.repx)
```

---

## 7. Argumentos CLI

### 7.1 Tabela de Argumentos

| Argumento | Obrigatório | Default | Descrição |
|-----------|-------------|---------|-----------|
| `--edited-dir` / `-e` | ✅ | - | Pasta com imagens `*_with_spots.jpg` |
| `--output` / `-o` | ✅ | - | Caminho do arquivo `.repx` |
| `--site-name` / `-s` | ❌ | `"Thermal Inspection Report"` | Nome do site/relatório |
| `--logo` / `-l` | ❌ | None | Logo customizado (JPG ou PNG) |

### 7.2 Busca de Imagens

O script busca imagens na seguinte ordem:

1. `*_with_spots.jpg` (padrão do wp4_wp5)
2. `*.jpg` (fallback)

---

## 8. Fluxo de Execução

```
main()
  │
  ├── argparse: parsing de argumentos
  │
  └── generate_repx()
        │
        ├── Listar imagens
        │     └── glob("*_with_spots.jpg") ou glob("*.jpg")
        │
        ├── Validar (pelo menos 1 imagem)
        │
        ├── tempfile.TemporaryDirectory()
        │
        ├── Criar estrutura de pastas
        │
        ├── Processar logo
        │     ├── copy_logo_as_jpg() [se fornecido]
        │     └── create_default_logo() [fallback]
        │
        ├── Para cada imagem (loop):
        │     │
        │     ├── generate_rid() × 5 (IDs)
        │     │
        │     ├── uuid.uuid4() para nome no Media
        │     │
        │     ├── shutil.copy2() para Media/
        │     │
        │     ├── write_utf16_xml(sectionN.xml)
        │     │
        │     └── write_utf8_xml(sectionN.xml.rels)
        │
        ├── write_utf16_xml(report.xml)
        │
        ├── write_utf8_xml(report.xml.rels)
        │
        ├── write_utf8_xml(_rels/.rels)
        │
        ├── write_utf8_xml([Content_Types].xml)
        │
        └── zipfile.ZipFile() → .repx
```

---

## 9. Estrutura do REPX

### 9.1 [Content_Types].xml

Define os tipos MIME para cada extensão/arquivo.

```xml
<?xml version="1.0" encoding="utf-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
    <Default Extension="xml" ContentType="text/xml" />
    <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml" />
    <Default Extension="jpg" ContentType="image/jpeg" />
    <Override PartName="/Report/Media/PagePreview_0" ContentType="image/jpeg" />
</Types>
```

### 9.2 _rels/.rels

Relacionamento raiz apontando para o documento principal.

```xml
<?xml version="1.0" encoding="utf-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
    <Relationship 
        Type="http://flir.com/reportdocument" 
        Target="/Report/report.xml" 
        Id="R1234567890abcdef" />
</Relationships>
```

### 9.3 Report/report.xml

Documento principal do relatório.

```xml
<?xml version="1.0" encoding="utf-16" standalone="yes" ?>
<Report 
    xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" 
    Assembly="Flir.Model.Report, Version=1.0.10349.1000, Culture=neutral, PublicKeyToken=caa391fd8e07c76b" 
    ShowAllIRImageParameters="false" 
    xmlns="http://flir.com/reportml/main">
    
    <Previews>
        <Preview r:ID="Rpreview123456789" />
    </Previews>
    
    <DocumentData PageSize="8" PageOrientation="2" />
    
    <Logo r:ID="Rlogo1234567890ab" />
    
    <Section r:ID="Rsection1_id12345" />
    <Section r:ID="Rsection2_id67890" />
    <!-- ... mais seções ... -->
    
</Report>
```

### 9.4 Report/Sections/sectionN.xml

Define o layout de uma página/seção.

```xml
<?xml version="1.0" encoding="utf-16" standalone="yes" ?>
<Section 
    xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" 
    xmlns="http://flir.com/reportml/main">
    
    <!-- Imagem IR (canto superior direito) -->
    <SectionContentItem X="220" Y="0" Height="343" Width="457" IsFilled="false">
        <Image 
            r:ID="Rir_image_id12345" 
            ImageType="IRFusionImage" 
            IsInformationVisible="true" 
            IsScaleImageVisible="true" 
            ImageFileName="original_name.jpg" />
    </SectionContentItem>
    
    <!-- Tabela de resultados IR (canto superior esquerdo) -->
    <SectionContentItem X="0" Y="0" Height="553" Width="200" IsFilled="false">
        <ResultTable r:ID="Rir_table_id12345" />
    </SectionContentItem>
    
    <!-- Imagem Visual (canto inferior direito) -->
    <SectionContentItem X="220" Y="363" Height="343" Width="457" IsFilled="false">
        <Image 
            r:ID="Rvisual_image_id12" 
            ImageType="FusionVisualImage" 
            IsInformationVisible="true" 
            IsScaleImageVisible="true" 
            ImageFileName="original_name.jpg" />
    </SectionContentItem>
    
    <!-- Tabela de resultados Visual (canto inferior esquerdo) -->
    <SectionContentItem X="0" Y="573" Height="184" Width="200" IsFilled="false">
        <ResultTable r:ID="Rvisual_table_id12" />
    </SectionContentItem>
    
</Section>
```

**Layout da seção:**

```
┌────────────────┬─────────────────────────┐
│                │                         │
│   ResultTable  │    IR Fusion Image      │
│   (IR data)    │    (ImageType=IR...)    │
│                │                         │
│    200×553     │       457×343           │
│                │                         │
├────────────────┼─────────────────────────┤
│                │                         │
│   ResultTable  │   Visual Fusion Image   │
│  (Visual data) │  (ImageType=FusionVis..)│
│                │                         │
│    200×184     │       457×343           │
│                │                         │
└────────────────┴─────────────────────────┘
```

### 9.5 Report/Sections/_rels/sectionN.xml.rels

Relacionamentos da seção para as imagens.

```xml
<?xml version="1.0" encoding="utf-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
    <Relationship 
        Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" 
        Target="/Report/Media/uuid-da-imagem.jpg" 
        Id="Rir_image_id12345" />
    <Relationship 
        Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" 
        Target="/Report/Media/uuid-da-imagem.jpg" 
        Id="Rir_table_id12345" />
    <Relationship 
        Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" 
        Target="/Report/Media/uuid-da-imagem.jpg" 
        Id="Rvisual_image_id12" />
    <Relationship 
        Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" 
        Target="/Report/Media/uuid-da-imagem.jpg" 
        Id="Rvisual_table_id12" />
</Relationships>
```

**Nota:** Todos os 4 IDs apontam para a mesma imagem física. O FLIR Tools extrai IR e Visual da mesma imagem FLIR.

---

## 10. Exemplos de Uso

### 10.1 Uso Básico

```bash
python -m src.tools.wp6_repx_generator \
    --edited-dir "output/05_wp4_wp5_analysis/edited" \
    --output "output/06_report/inspection_report.repx"
```

### 10.2 Com Logo Customizado

```bash
python -m src.tools.wp6_repx_generator \
    --edited-dir "output/edited" \
    --output "output/report.repx" \
    --site-name "Syncarpha - Dodge 1 - 2025" \
    --logo "assets/company_logo.png"
```

### 10.3 Uso Programático

```python
from pathlib import Path
from src.tools.wp6_repx_generator import generate_repx

success = generate_repx(
    edited_dir=Path("output/edited"),
    output_path=Path("output/report.repx"),
    site_name="Site ABC - Inspeção Térmica",
    logo_path=Path("assets/logo.jpg"),
)

if success:
    print("Relatório gerado com sucesso!")
```

---

## 11. Integração com Pipeline

### 11.1 Chamada pelo run_full_pipeline.py

```python
# Em step_5_wp6_report()
cmd = [
    sys.executable, "-m", "src.tools.wp6_repx_generator",
    "--edited-dir", str(analysis_dir / "edited"),
    "--output", str(report_dir / "report.repx"),
    "--site-name", pipeline_result.site_name,
]

# Logo opcional
for logo_path in logo_candidates:
    if logo_path.exists():
        cmd.extend(["--logo", str(logo_path)])
        break

subprocess.run(cmd, capture_output=True, text=True, timeout=300)
```

### 11.2 Fluxo de Dados

```
05_wp4_wp5_analysis/           wp6_repx_generator.py        06_report/
     │                                │                          │
     └── edited/                      │                          │
           │                          │                          │
           ├── img1_with_spots.jpg ───┼──────────────────────►   ├── report.repx
           ├── img2_with_spots.jpg ───┤                          │     └── (ZIP contendo:)
           └── img3_with_spots.jpg ───┘                          │           ├── Report/
                                                                 │           │   ├── Media/
                                                                 │           │   └── Sections/
                                                                 │           └── ...
                                                                 │
                                                                 ├── site_summary.json
                                                                 └── anomalies_report.csv
```

### 11.3 Localização do Logo

O script procura logo em:

```python
logo_candidates = [
    Path(__file__).parent / "QE_SOLAR_logo.png",
    Path("/home/elton/qe_solar_wires/src/tools/QE_SOLAR_logo.png"),
]
```

---

## 12. Notas de Implementação

### 12.1 Design Decisions

1. **XML Minificado**
   - FLIR Tools espera XML em uma única linha
   - Sem indentação ou quebras de linha

2. **UTF-16 para conteúdo, UTF-8 para .rels**
   - Compatibilidade com FLIR Tools
   - BOM obrigatório em ambos

3. **Diretório temporário**
   - Toda a estrutura é montada em `/tmp`
   - Compactada para ZIP no final
   - Limpo automaticamente

4. **Logo placeholder**
   - JPEG mínimo válido (1x1 branco)
   - Evita erros se logo não fornecido

### 12.2 Correções na v2

| Problema | Correção |
|----------|----------|
| Tipo de relationship errado | Alterado para `http://flir.com/reportdocument/section` |
| XML formatado | Minificado (uma linha) |
| Falta de BOM nos .rels | Adicionado BOM UTF-8 |

### 12.3 Limitações Conhecidas

1. **Layout fixo**
   - Posições X, Y, Width, Height são hardcoded
   - Adequado para a maioria dos casos

2. **Sem suporte a anotações customizadas**
   - Apenas IR Image + Visual Image + Tables
   - Comentários/notas não suportados

3. **Uma imagem por seção**
   - Cada imagem gera uma seção completa
   - Não há agrupamento

### 12.4 Compatibilidade

- **FLIR Tools:** ✅ Testado e funcionando
- **FLIR Report Studio:** ✅ Compatível
- **FLIR Thermal Studio:** ⚠️ Não testado

### 12.5 Performance

| Operação | Tempo típico |
|----------|--------------|
| Processar 1 imagem | ~0.1s |
| Processar 50 imagens | ~2-3s |
| Compactação ZIP | ~0.5s |

O gargalo é I/O (cópia de imagens).

### 12.6 Tamanho do Output

```
Tamanho REPX ≈ Σ(tamanho das imagens) + ~50KB (metadados)
```

Imagens FLIR típicas: 200-500KB cada.

---

## Changelog

### v2.1 (Fevereiro 2025)
- Suporte a logo PNG (conversão automática para JPG)
- Tratamento de canal alpha

### v2.0 (Fevereiro 2025)
- Correção do tipo de relationship para seções
- XML minificado (uma linha)
- BOM UTF-8 nos arquivos .rels

### v1.0 (anterior)
- Versão inicial

---

## Referências

- [Open Packaging Conventions (OPC)](https://en.wikipedia.org/wiki/Open_Packaging_Conventions)
- [FLIR Tools Documentation](https://www.flir.com/products/flir-tools/)
- [Office Open XML](https://docs.microsoft.com/en-us/office/open-xml/open-xml-sdk)

---

*Documentação gerada para handoff do projeto QE Solar.*
