#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Pipeline Otimizado para Análise de Imagens FLIR
Combina renderização limpa com detecção de anomalias térmicas
Ideal para inspeção de painéis elétricos e equipamentos industriais
"""

from pathlib import Path
import argparse
import numpy as np
import cv2
import json
from typing import Dict, List, Tuple, Optional
from datetime import datetime
import logging
from tqdm import tqdm

# --- helper para converter tipos NumPy em tipos nativos antes do JSON ---
def _np_encoder(o):
    import numpy as _np
    if isinstance(o, (_np.integer,)):  return int(o)
    if isinstance(o, (_np.floating,)): return float(o)
    if isinstance(o, _np.ndarray):     return o.tolist()
    return str(o)  # fallback seguro p/ objetos inesperados

# Configurar logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

try:
    from flirimageextractor import FlirImageExtractor
except ImportError:
    logger.error("flirimageextractor não instalado! Use: pip install flirimageextractor")
    FlirImageExtractor = None

class FLIRThermalAnalyzer:
    """
    Analisador térmico otimizado para equipamentos elétricos/industriais
    """
    
    def __init__(self, colormap: str = "inferno", clip_percent: float = 1.0):
        """
        Args:
            colormap: Nome do colormap (jet, turbo, magma, inferno, plasma)
            clip_percent: Percentual de recorte para evitar outliers
        """
        self.colormap = colormap
        self.clip_percent = clip_percent
        self.fie = None
        self.thermal_data = None
        self.metadata = None
        
        # Mapeamento de colormaps do OpenCV
        self.colormap_dict = {
            "jet": cv2.COLORMAP_JET,
            "turbo": cv2.COLORMAP_TURBO,
            "hot": cv2.COLORMAP_HOT,
            "magma": cv2.COLORMAP_MAGMA if hasattr(cv2, "COLORMAP_MAGMA") else cv2.COLORMAP_HOT,
            "inferno": cv2.COLORMAP_INFERNO if hasattr(cv2, "COLORMAP_INFERNO") else cv2.COLORMAP_HOT,
            "plasma": cv2.COLORMAP_PLASMA if hasattr(cv2, "COLORMAP_PLASMA") else cv2.COLORMAP_HOT,
        }
        
    def extract_thermal_data(self, image_path: Path) -> Optional[np.ndarray]:
        """
        Extrai dados térmicos da imagem FLIR
        
        Returns:
            Array numpy com temperaturas em Celsius
        """
        if FlirImageExtractor is None:
            return None
            
        try:
            self.fie = FlirImageExtractor(is_debug=False)
            self.fie.process_image(str(image_path))
            
            # Matriz térmica em °C
            self.thermal_data = self.fie.get_thermal_np()
            
            if self.thermal_data is None or self.thermal_data.size == 0:
                logger.warning(f"Sem dados radiométricos: {image_path.name}")
                return None
                
            # Tentar extrair metadados
            try:
                self.metadata = self.fie.get_metadata(str(image_path))
            except:
                self.metadata = {}
                
            return self.thermal_data
            
        except Exception as e:
            logger.error(f"Erro ao extrair {image_path.name}: {e}")
            return None
    
    def render_clean_thermal(self, 
                            thermal: np.ndarray,
                            vmin: Optional[float] = None,
                            vmax: Optional[float] = None) -> np.ndarray:
        """
        Renderiza imagem térmica limpa (sem UI/HUD)
        
        Args:
            thermal: Matriz de temperaturas
            vmin: Temperatura mínima para visualização
            vmax: Temperatura máxima para visualização
            
        Returns:
            Imagem RGB renderizada
        """
        th = thermal.copy()
        
        # Calcular faixa dinâmica usando percentis (robusto a outliers)
        if vmin is None or vmax is None:
            if self.clip_percent > 0:
                lo = np.percentile(th, self.clip_percent)
                hi = np.percentile(th, 100 - self.clip_percent)
            else:
                lo, hi = float(np.nanmin(th)), float(np.nanmax(th))
            
            if vmin is None: 
                vmin = lo
            if vmax is None: 
                vmax = hi
        
        # Normalizar para 0-255
        th_norm = np.clip((th - vmin) / max(1e-6, (vmax - vmin)), 0, 1)
        th_u8 = (th_norm * 255).astype(np.uint8)
        
        # Aplicar colormap
        cmap = self.colormap_dict.get(self.colormap.lower(), cv2.COLORMAP_JET)
        rgb = cv2.applyColorMap(th_u8, cmap)
        
        return rgb
    
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
        - threshold adaptativo: max(P95, mediana + delta_c, min_abs_c)
        - fallback pontual se Tmax > min_abs_c e nenhum blob detectado
        """
        hotspots = []

        # 1) cálculo de limiar adaptativo
        median = float(np.median(thermal))
        p95 = float(np.percentile(thermal, 95))
        thr = max(median + delta_c, p95, min_abs_c)

        # 2) máscara binária
        mask = (thermal > thr).astype(np.uint8) * 255

        # 3) morfologia leve
        kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
        mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)

        # 4) contornos
        contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        for contour in contours:
            area = cv2.contourArea(contour)
            if area < min_area_px:
                continue
            x, y, w, h = cv2.boundingRect(contour)
            roi = thermal[y:y+h, x:x+w]
            max_temp = float(np.max(roi))
            mean_temp = float(np.mean(roi))
            min_temp = float(np.min(roi))
            center = (int(x + w//2), int(y + h//2))
            severity_color = (0, 255, 0)
            severity = 'BAIXO'
            delta = max_temp - np.mean(thermal)
            if delta > 30:
                severity, severity_color = 'CRÍTICO', (0, 0, 255)
            elif delta > 20:
                severity, severity_color = 'ALTO', (0, 128, 255)
            elif delta > 10:
                severity, severity_color = 'MÉDIO', (0, 255, 255)

            hotspots.append({
                'bbox': (x, y, w, h),
                'center': center,
                'area': area,
                'max_temp': max_temp,
                'mean_temp': mean_temp,
                'min_temp': min_temp,
                'threshold_used': thr,
                'severity': severity,
                'color': severity_color
            })

        # 5) fallback se houver alerta térmico mas sem blob
        if not hotspots and np.max(thermal) > min_abs_c:
            max_idx = np.unravel_index(np.argmax(thermal), thermal.shape)
            cy, cx = max_idx
            hotspots.append({
                'bbox': (int(cx)-1, int(cy)-1, 3, 3),
                'center': (int(cx), int(cy)),
                'area': 1,
                'max_temp': float(np.max(thermal)),
                'mean_temp': float(np.max(thermal)),
                'min_temp': float(np.max(thermal)),
                'threshold_used': thr,
                'severity': 'PONTUAL',
                'color': (255, 0, 255)
            })

        # 6) ordenar e cortar top-K
        hotspots.sort(key=lambda x: x['max_temp'], reverse=True)
        if len(hotspots) > top_k:
            hotspots = hotspots[:top_k]

        return hotspots
    
    def analyze_electrical_panel(self, thermal: np.ndarray) -> Dict:
        """
        Análise específica para painéis elétricos
        
        Returns:
            Diagnóstico do painel
        """
        analysis = {
            'timestamp': datetime.now().isoformat(),
            'statistics': {
                'min_temp': float(np.min(thermal)),
                'max_temp': float(np.max(thermal)),
                'mean_temp': float(np.mean(thermal)),
                'std_temp': float(np.std(thermal)),
                'median_temp': float(np.median(thermal))
            },
            'alerts': [],
            'recommendations': []
        }
        
        # Detectar hotspots
        hotspots = self.detect_hotspots(thermal, threshold_method="statistical", threshold_value=2.0)
        analysis['hotspots'] = hotspots
        analysis['hotspot_count'] = len(hotspots)
        
        # Análise de risco
        max_temp = analysis['statistics']['max_temp']
        mean_temp = analysis['statistics']['mean_temp']
        
        # Alertas baseados em temperatura absoluta
        if max_temp > 70:
            analysis['alerts'].append({
                'level': 'CRÍTICO',
                'message': f'Temperatura crítica detectada: {max_temp:.1f}°C',
                'action': 'Inspeção imediata necessária'
            })
            analysis['recommendations'].append('Desligar equipamento e verificar conexões')
            
        elif max_temp > 60:
            analysis['alerts'].append({
                'level': 'ALTO',
                'message': f'Temperatura elevada: {max_temp:.1f}°C',
                'action': 'Agendar manutenção preventiva'
            })
            analysis['recommendations'].append('Verificar torque das conexões')
            
        elif max_temp > 50:
            analysis['alerts'].append({
                'level': 'MÉDIO',
                'message': f'Temperatura acima do normal: {max_temp:.1f}°C',
                'action': 'Monitorar evolução'
            })
            analysis['recommendations'].append('Realizar nova inspeção em 30 dias')
        
        # Análise de distribuição
        if len(hotspots) > 5:
            analysis['alerts'].append({
                'level': 'MÉDIO',
                'message': f'Múltiplos pontos quentes detectados ({len(hotspots)})',
                'action': 'Verificar balanceamento de cargas'
            })
        
        # Calcular delta máximo
        max_delta = max_temp - mean_temp
        analysis['max_delta'] = max_delta
        
        if max_delta > 25:
            analysis['recommendations'].append('Possível problema de conexão ou sobrecarga localizada')
        
        return analysis
    
    def visualize_analysis(self, 
                          thermal: np.ndarray,
                          rgb_clean: np.ndarray,
                          analysis: Dict,
                          output_path: Optional[Path] = None) -> np.ndarray:
        """
        Cria visualização completa da análise
        
        Returns:
            Imagem com visualização completa
        """
        h, w = thermal.shape
        
        # Criar canvas maior para incluir informações
        canvas_h = h + 100  # Espaço para texto
        canvas_w = w * 2  # Duas imagens lado a lado
        canvas = np.zeros((canvas_h, canvas_w, 3), dtype=np.uint8)
        
        # Imagem térmica limpa à esquerda
        canvas[0:h, 0:w] = rgb_clean
        
        # Imagem com anotações à direita
        annotated = rgb_clean.copy()
        
        # Desenhar hotspots
        for hs in analysis['hotspots']:
            x, y, w_box, h_box = hs['bbox']
            color = hs['color']
            
            # Retângulo
            cv2.rectangle(annotated, (x, y), (x+w_box, y+h_box), color, 2)
            
            # Texto com temperatura
            label = f"{hs['max_temp']:.1f}C"
            cv2.putText(annotated, label, (x, y-5), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)
            
            # Marcador no centro
            cx, cy = hs['center']
            cv2.circle(annotated, (cx, cy), 3, color, -1)
        
        canvas[0:h, w:w*2] = annotated
        
        # Adicionar informações de texto
        y_offset = h + 20
        font = cv2.FONT_HERSHEY_SIMPLEX
        
        # Estatísticas
        stats_text = [
            f"Min: {analysis['statistics']['min_temp']:.1f}C",
            f"Max: {analysis['statistics']['max_temp']:.1f}C",
            f"Med: {analysis['statistics']['mean_temp']:.1f}C",
            f"Hotspots: {analysis['hotspot_count']}"
        ]
        
        for i, text in enumerate(stats_text):
            cv2.putText(canvas, text, (10, y_offset + i*20),
                       font, 0.5, (255, 255, 255), 1)
        
        # Alertas
        if analysis['alerts']:
            alert = analysis['alerts'][0]
            alert_color = (0, 0, 255) if alert['level'] == 'CRÍTICO' else (0, 255, 255)
            cv2.putText(canvas, f"ALERTA: {alert['message']}", 
                       (w + 10, y_offset),
                       font, 0.5, alert_color, 2)
        
        if output_path:
            cv2.imwrite(str(output_path), canvas)
            
        return canvas


def process_batch(input_dir: Path, 
                  output_dir: Path,
                  colormap: str = "inferno",
                  save_npy: bool = False,
                  save_analysis: bool = True,
                  limit: int = 0) -> Dict:
    """
    Processa lote de imagens FLIR
    
    Returns:
        Resumo do processamento
    """
    analyzer = FLIRThermalAnalyzer(colormap=colormap)
    
    # Criar apenas o diretório de saída
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Listar imagens
    extensions = (".jpg", ".jpeg", ".png", ".tif", ".tiff")
    images = [p for p in input_dir.rglob("*") if p.suffix.lower() in extensions]
    images.sort()
    
    if limit > 0:
        images = images[:limit]
    
    logger.info(f"Processando {len(images)} imagens de {input_dir}")
    
    results = {
        'processed': 0,
        'failed': 0
    }
    
    for img_path in tqdm(images, total=len(images), desc="Processando", unit="img"):
        logger.info(f"Processando: {img_path.name}")
        
        # Extrair dados térmicos
        thermal = analyzer.extract_thermal_data(img_path)
        if thermal is None:
            results['failed'] += 1
            continue
        
        # Renderizar imagem limpa e salvar diretamente na pasta de saída
        rgb_clean = analyzer.render_clean_thermal(thermal)
        out_clean = output_dir / f"{img_path.stem}_clean.png"
        cv2.imwrite(str(out_clean), rgb_clean)
        
        results['processed'] += 1
    
    # Sem relatório/análise: apenas log de resumo
    logger.info(f"Processadas: {results['processed']}/{len(images)} | Falhas: {results['failed']}")
    
    return results


def main():
    parser = argparse.ArgumentParser(description="Pipeline otimizado para análise térmica FLIR")
    parser.add_argument("--input", required=True, help="Diretório com imagens FLIR")
    parser.add_argument("--output", required=True, help="Diretório de saída")
    parser.add_argument("--colormap", default="inferno", 
                       choices=["jet", "turbo", "hot", "magma", "inferno", "plasma"],
                       help="Colormap para renderização")
    parser.add_argument("--save-npy", action="store_true", help="Salvar matrizes térmicas em .npy")
    parser.add_argument("--no-analysis", action="store_true", help="Pular análise de hotspots")
    parser.add_argument("--limit", type=int, default=0, help="Limitar número de imagens")
    
    args = parser.parse_args()
    
    input_dir = Path(args.input)
    output_dir = Path(args.output)
    
    if not input_dir.exists():
        logger.error(f"Diretório não encontrado: {input_dir}")
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
    
    logger.info(f"\n✅ Processamento concluído! Resultados em: {output_dir}")


if __name__ == "__main__":
    main()

#  python 2-clean_ui.py --input /home/jean/qe_solar/images/all/crossover_edited --output /home/jean/qe_solar/images/all/all_edited/clean
#  python 2-clean_ui.py --input /home/jean/qe_solar/images/all/all_edited_new --output /home/jean/qe_solar/images/all/all_edited_new_treated/clean
# python 2-clean_ui.py --input /home/jean/qe_solar/images/all/all_edited_new --output /home/jean/qe_solar/images/all/all_edited_new_treated/clean

# python 2-clean_ui.py --input /home/jean/qe_solar/images/all/26_test_batch/raw --output /home/jean/qe_solar/images/all/26_test_batch/clean