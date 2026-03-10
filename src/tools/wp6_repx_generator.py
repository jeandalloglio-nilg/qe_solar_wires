#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
WP6 - REPX Report Generator (v2.1)
===================================

Gera relatórios .repx para FLIR Tools a partir das imagens com spots injetados.

CORREÇÕES v2:
- Tipo de relationship corrigido para "http://flir.com/reportdocument/section"
- XML em uma única linha (minificado) como o FLIR Tools espera
- BOM UTF-8 nos arquivos .rels

v2.1:
- Suporte a logo PNG (converte automaticamente para JPG)

Author: QE Solar / Elton Gino
"""

from __future__ import annotations

import argparse
import shutil
import uuid
import zipfile
from pathlib import Path
from typing import List, Optional
import sys


# =============================================================================
# Utility Functions
# =============================================================================

def generate_rid() -> str:
    """Gera um ID no formato FLIR: R + 16 hex chars."""
    return f"R{uuid.uuid4().hex[:16]}"


def write_utf16_xml(path: Path, content: str) -> None:
    """Escreve XML em UTF-16 LE com BOM (formato FLIR)."""
    # Minificar (uma linha)
    content = content.strip()
    # UTF-16 LE com BOM
    path.write_bytes(content.encode('utf-16'))


def write_utf8_xml(path: Path, content: str) -> None:
    """Escreve XML em UTF-8 com BOM (formato FLIR para .rels)."""
    # Minificar (uma linha, sem espaços extras)
    content = content.strip()
    # UTF-8 com BOM
    bom = b'\xef\xbb\xbf'
    path.write_bytes(bom + content.encode('utf-8'))


def create_default_logo(output_path: Path) -> None:
    """Cria um logo placeholder (1x1 pixel branco JPEG)."""
    minimal_jpeg = bytes([
        0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01,
        0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43,
        0x00, 0x08, 0x06, 0x06, 0x07, 0x06, 0x05, 0x08, 0x07, 0x07, 0x07, 0x09,
        0x09, 0x08, 0x0A, 0x0C, 0x14, 0x0D, 0x0C, 0x0B, 0x0B, 0x0C, 0x19, 0x12,
        0x13, 0x0F, 0x14, 0x1D, 0x1A, 0x1F, 0x1E, 0x1D, 0x1A, 0x1C, 0x1C, 0x20,
        0x24, 0x2E, 0x27, 0x20, 0x22, 0x2C, 0x23, 0x1C, 0x1C, 0x28, 0x37, 0x29,
        0x2C, 0x30, 0x31, 0x34, 0x34, 0x34, 0x1F, 0x27, 0x39, 0x3D, 0x38, 0x32,
        0x3C, 0x2E, 0x33, 0x34, 0x32, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x01,
        0x00, 0x01, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0x1F, 0x00, 0x00,
        0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0xFF, 0xC4, 0x00, 0xB5, 0x10, 0x00, 0x02, 0x01, 0x03,
        0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D,
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06,
        0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
        0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72,
        0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45,
        0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75,
        0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3,
        0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
        0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
        0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
        0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4,
        0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01,
        0x00, 0x00, 0x3F, 0x00, 0xFB, 0xD5, 0xDB, 0x20, 0xA8, 0xF1, 0x45, 0x00,
        0x14, 0x50, 0x01, 0x40, 0x05, 0xFF, 0xD9
    ])
    output_path.write_bytes(minimal_jpeg)


def copy_logo_as_jpg(logo_path: Path, dest_path: Path) -> bool:
    """
    Copia logo para destino, convertendo PNG para JPG se necessário.
    Returns True se sucesso.
    """
    if not logo_path.exists():
        return False
    
    if logo_path.suffix.lower() == ".png":
        # Converter PNG para JPG
        try:
            from PIL import Image as PILImage
            with PILImage.open(logo_path) as img:
                # Converter para RGB (remover alpha se houver)
                if img.mode in ('RGBA', 'LA', 'P'):
                    background = PILImage.new('RGB', img.size, (255, 255, 255))
                    if img.mode == 'P':
                        img = img.convert('RGBA')
                    if img.mode == 'RGBA':
                        background.paste(img, mask=img.split()[3])
                    else:
                        background.paste(img)
                    img = background
                elif img.mode != 'RGB':
                    img = img.convert('RGB')
                img.save(dest_path, 'JPEG', quality=95)
            print(f"   ✅ Logo convertido (PNG→JPG): {logo_path.name}")
            return True
        except Exception as e:
            print(f"   ⚠️  Erro ao converter PNG: {e}")
            return False
    else:
        # Copiar diretamente (JPG ou outro)
        shutil.copy2(logo_path, dest_path)
        print(f"   ✅ Logo: {logo_path.name}")
        return True


# =============================================================================
# REPX Generator
# =============================================================================

def generate_repx(
    edited_dir: Path,
    output_path: Path,
    site_name: str = "Thermal Inspection Report",
    logo_path: Optional[Path] = None,
) -> bool:
    """
    Gera arquivo .repx a partir das imagens na pasta edited/.
    """
    print("\n" + "=" * 70)
    print("WP6 - REPX Report Generator (v2.1)")
    print("=" * 70)
    
    # Encontrar imagens
    images = sorted(edited_dir.glob("*_with_spots.jpg"))
    if not images:
        images = sorted(edited_dir.glob("*.jpg"))
    
    if not images:
        print(f"❌ Nenhuma imagem encontrada em: {edited_dir}")
        return False
    
    print(f"\n📂 Pasta edited: {edited_dir}")
    print(f"📷 Imagens encontradas: {len(images)}")
    print(f"📋 Site: {site_name}")
    
    import tempfile
    with tempfile.TemporaryDirectory() as tmpdir:
        work_dir = Path(tmpdir)
        
        # Criar estrutura
        (work_dir / "Report" / "Sections" / "_rels").mkdir(parents=True)
        (work_dir / "Report" / "Media").mkdir(parents=True)
        (work_dir / "Report" / "_rels").mkdir(parents=True)
        (work_dir / "_rels").mkdir(parents=True)
        
        # IDs fixos
        logo_id = generate_rid()
        preview_id = generate_rid()
        
        # Copiar/criar logo
        logo_dest = work_dir / "Report" / "Media" / "Logo.jpg"
        if logo_path and logo_path.exists():
            if not copy_logo_as_jpg(logo_path, logo_dest):
                create_default_logo(logo_dest)
        else:
            create_default_logo(logo_dest)
            print("   ⚠️  Usando logo placeholder")
        
        # Preview placeholder
        preview_dest = work_dir / "Report" / "Media" / "PagePreview_0"
        create_default_logo(preview_dest)
        
        # Processar imagens
        sections_xml = []
        section_rels_parts = []
        
        for idx, img_path in enumerate(images, start=1):
            print(f"   [{idx}/{len(images)}] {img_path.name}")
            
            # IDs
            section_id = generate_rid()
            ir_image_id = generate_rid()
            ir_table_id = generate_rid()
            visual_image_id = generate_rid()
            visual_table_id = generate_rid()
            
            # Nome na Media
            media_uuid = str(uuid.uuid4())
            media_filename = f"{media_uuid}.jpg"
            
            # Copiar imagem
            shutil.copy2(img_path, work_dir / "Report" / "Media" / media_filename)
            
            # Nome original
            original_name = img_path.stem.replace("_with_spots", "") + ".jpg"
            
            # Section XML (minificado)
            section_xml = (
                f'<?xml version="1.0" encoding="utf-16" standalone="yes" ?>'
                f'<Section xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns="http://flir.com/reportml/main">'
                f'<SectionContentItem X="220" Y="0" Height="343" Width="457" IsFilled="false">'
                f'<Image r:ID="{ir_image_id}" ImageType="IRFusionImage" IsInformationVisible="true" IsScaleImageVisible="true" ImageFileName="{original_name}" />'
                f'</SectionContentItem>'
                f'<SectionContentItem X="0" Y="0" Height="553" Width="200" IsFilled="false">'
                f'<ResultTable r:ID="{ir_table_id}" />'
                f'</SectionContentItem>'
                f'<SectionContentItem X="220" Y="363" Height="343" Width="457" IsFilled="false">'
                f'<Image r:ID="{visual_image_id}" ImageType="FusionVisualImage" IsInformationVisible="true" IsScaleImageVisible="true" ImageFileName="{original_name}" />'
                f'</SectionContentItem>'
                f'<SectionContentItem X="0" Y="573" Height="184" Width="200" IsFilled="false">'
                f'<ResultTable r:ID="{visual_table_id}" />'
                f'</SectionContentItem>'
                f'</Section>'
            )
            write_utf16_xml(work_dir / "Report" / "Sections" / f"section{idx}.xml", section_xml)
            
            # Section rels (minificado, UTF-8 com BOM)
            section_rels = (
                f'<?xml version="1.0" encoding="utf-8"?>'
                f'<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
                f'<Relationship Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="/Report/Media/{media_filename}" Id="{ir_image_id}" />'
                f'<Relationship Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="/Report/Media/{media_filename}" Id="{ir_table_id}" />'
                f'<Relationship Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="/Report/Media/{media_filename}" Id="{visual_image_id}" />'
                f'<Relationship Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="/Report/Media/{media_filename}" Id="{visual_table_id}" />'
                f'</Relationships>'
            )
            write_utf8_xml(work_dir / "Report" / "Sections" / "_rels" / f"section{idx}.xml.rels", section_rels)
            
            # Adicionar ao report.xml
            sections_xml.append(f'<Section r:ID="{section_id}" />')
            
            # Adicionar ao report.xml.rels (TIPO CORRETO: http://flir.com/reportdocument/section)
            section_rels_parts.append(
                f'<Relationship Type="http://flir.com/reportdocument/section" Target="/Report/Sections/section{idx}.xml" Id="{section_id}" />'
            )
        
        # report.xml (minificado)
        report_xml = (
            f'<?xml version="1.0" encoding="utf-16" standalone="yes" ?>'
            f'<Report xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" '
            f'Assembly="Flir.Model.Report, Version=1.0.10349.1000, Culture=neutral, PublicKeyToken=caa391fd8e07c76b" '
            f'ShowAllIRImageParameters="false" xmlns="http://flir.com/reportml/main">'
            f'<Previews><Preview r:ID="{preview_id}" /></Previews>'
            f'<DocumentData PageSize="8" PageOrientation="2" />'
            f'<Logo r:ID="{logo_id}" />'
            + ''.join(sections_xml) +
            f'</Report>'
        )
        write_utf16_xml(work_dir / "Report" / "report.xml", report_xml)
        
        # report.xml.rels (minificado, UTF-8 com BOM)
        report_rels = (
            f'<?xml version="1.0" encoding="utf-8"?>'
            f'<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
            f'<Relationship Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="/Report/Media/PagePreview_0" Id="{preview_id}" />'
            f'<Relationship Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="/Report/Media/Logo.jpg" Id="{logo_id}" />'
            + ''.join(section_rels_parts) +
            f'</Relationships>'
        )
        write_utf8_xml(work_dir / "Report" / "_rels" / "report.xml.rels", report_rels)
        
        # _rels/.rels (TIPO CORRETO: http://flir.com/reportdocument)
        root_rels_id = generate_rid()
        root_rels = (
            '<?xml version="1.0" encoding="utf-8"?>'
            '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
            f'<Relationship Type="http://flir.com/reportdocument" Target="/Report/report.xml" Id="{root_rels_id}" />'
            '</Relationships>'
        )
        write_utf8_xml(work_dir / "_rels" / ".rels", root_rels)
        
        # [Content_Types].xml (TIPO CORRETO: text/xml, com Override para PagePreview)
        content_types = (
            '<?xml version="1.0" encoding="utf-8"?>'
            '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
            '<Default Extension="xml" ContentType="text/xml" />'
            '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml" />'
            '<Default Extension="jpg" ContentType="image/jpeg" />'
            '<Override PartName="/Report/Media/PagePreview_0" ContentType="image/jpeg" />'
            '</Types>'
        )
        write_utf8_xml(work_dir / "[Content_Types].xml", content_types)
        
        # Criar ZIP
        output_path.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(output_path, 'w', zipfile.ZIP_DEFLATED) as zf:
            for file_path in work_dir.rglob("*"):
                if file_path.is_file():
                    arcname = file_path.relative_to(work_dir).as_posix()
                    zf.write(file_path, arcname)
        
        print(f"\n✅ REPX gerado: {output_path}")
        print(f"   Imagens: {len(images)}")
        
        return True


def main():
    parser = argparse.ArgumentParser(description="WP6 - Generate .repx for FLIR Tools")
    parser.add_argument("--edited-dir", "-e", required=True, help="Pasta com imagens *_with_spots.jpg")
    parser.add_argument("--output", "-o", required=True, help="Caminho do arquivo .repx")
    parser.add_argument("--site-name", "-s", default="Thermal Inspection Report", help="Nome do site")
    parser.add_argument("--logo", "-l", default=None, help="Logo customizado (JPG ou PNG)")
    
    args = parser.parse_args()
    
    success = generate_repx(
        edited_dir=Path(args.edited_dir),
        output_path=Path(args.output),
        site_name=args.site_name,
        logo_path=Path(args.logo) if args.logo else None,
    )
    
    if not success:
        sys.exit(1)


if __name__ == "__main__":
    main()