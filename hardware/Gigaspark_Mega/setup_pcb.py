#!/usr/bin/env python3
"""
Gigaspark Mega - Script de Setup PCB para KiCad
Ejecutar desde el Board Editor de KiCad:
  File -> Scripting Console -> exec(open('setup_pcb.py').read())

Crea el board outline, mounting holes, y configura DRC rules.
Compatible con KiCad 8/9/10.
"""

import pcbnew

# ============================================================
# CONSTANTES
# ============================================================

BOARD_W_MM = 100
BOARD_H_MM = 80
MOUNT_HOLE_DIA = 3.2  # mm, M3
MOUNT_HOLE_OFFSET = 5  # mm desde las esquinas
PCB_FILE = "Gigaspark_Mega.kicad_pcb"

# ============================================================
# FUNCIONES AUXILIARES
# ============================================================

def mm_to_nm(mm):
    """Convertir milimetros a nanometros (unidad interna de KiCad)."""
    return int(mm * 1e6)


def set_design_rules(board):
    """Configurar reglas de diseno (DRC)."""
    board.GetDesignSettings().SetMinClearance(mm_to_nm(0.2))
    board.GetDesignSettings().SetMinTrackWidth(mm_to_nm(0.15))
    board.GetDesignSettings().SetMinViaDiameter(mm_to_nm(0.5))
    board.GetDesignSettings().SetMinViaDrill(mm_to_nm(0.2))
    board.GetDesignSettings().SetMinHoleClearance(mm_to_nm(0.2))
    board.GetDesignSettings().SetDefaultLineWidth(mm_to_nm(0.25))

    print("[OK] Reglas de diseno configuradas:")
    print(f"     Track min: 0.15mm, Clearance: 0.2mm")
    print(f"     Via min: 0.5mm, Via drill: 0.2mm")


def create_board_outline(board):
    """Crear borde rectangular del PCB en Edge.Cuts."""
    layer_id = pcbnew.Edge_Cuts

    # Rectangulo de 100mm x 80mm
    outline = pcbnew.PCB_SHAPE(board)
    outline.SetShape(pcbnew.SHAPE_T_SEGMENT)
    outline.SetLayer(layer_id)
    outline.SetWidth(mm_to_nm(0.15))

    # Esquina superior izquierda -> superior derecha
    outline.SetStart(pcbnew.VECTOR2I(0, 0))
    outline.SetEnd(pcbnew.VECTOR2I(mm_to_nm(BOARD_W_MM), 0))
    board.Add(outline)

    # Superior derecha -> inferior derecha
    outline2 = pcbnew.PCB_SHAPE(board)
    outline2.SetShape(pcbnew.SHAPE_T_SEGMENT)
    outline2.SetLayer(layer_id)
    outline2.SetWidth(mm_to_nm(0.15))
    outline2.SetStart(pcbnew.VECTOR2I(mm_to_nm(BOARD_W_MM), 0))
    outline2.SetEnd(pcbnew.VECTOR2I(mm_to_nm(BOARD_W_MM), mm_to_nm(BOARD_H_MM)))
    board.Add(outline2)

    # Inferior derecha -> inferior izquierda
    outline3 = pcbnew.PCB_SHAPE(board)
    outline3.SetShape(pcbnew.SHAPE_T_SEGMENT)
    outline3.SetLayer(layer_id)
    outline3.SetWidth(mm_to_nm(0.15))
    outline3.SetStart(pcbnew.VECTOR2I(mm_to_nm(BOARD_W_MM), mm_to_nm(BOARD_H_MM)))
    outline3.SetEnd(pcbnew.VECTOR2I(0, mm_to_nm(BOARD_H_MM)))
    board.Add(outline3)

    # Inferior izquierda -> superior izquierda
    outline4 = pcbnew.PCB_SHAPE(board)
    outline4.SetShape(pcbnew.SHAPE_T_SEGMENT)
    outline4.SetLayer(layer_id)
    outline4.SetWidth(mm_to_nm(0.15))
    outline4.SetStart(pcbnew.VECTOR2I(0, mm_to_nm(BOARD_H_MM)))
    outline4.SetEnd(pcbnew.VECTOR2I(0, 0))
    board.Add(outline4)

    print(f"[OK] Board outline: {BOARD_W_MM}mm x {BOARD_H_MM}mm")


def add_mounting_holes(board):
    """Añadir 4 agujeros de montaje M3 en las esquinas."""
    positions = [
        (MOUNT_HOLE_OFFSET, MOUNT_HOLE_OFFSET),
        (BOARD_W_MM - MOUNT_HOLE_OFFSET, MOUNT_HOLE_OFFSET),
        (MOUNT_HOLE_OFFSET, BOARD_H_MM - MOUNT_HOLE_OFFSET),
        (BOARD_W_MM - MOUNT_HOLE_OFFSET, BOARD_H_MM - MOUNT_HOLE_OFFSET),
    ]

    for i, (x, y) in enumerate(positions, 1):
        # Crear footprint simple con pad thru-hole
        pad = pcbnew.PAD(board)
        pad.SetShape(pcbnew.PAD_SHAPE_CIRCLE)
        pad.SetAttribute(pcbnew.PAD_ATTRIB_NPTH)  # Non-plated
        pad.SetDrillSize(pcbnew.VECTOR2I(mm_to_nm(MOUNT_HOLE_DIA), mm_to_nm(MOUNT_HOLE_DIA)))
        pad.SetSize(pcbnew.VECTOR2I(mm_to_nm(MOUNT_HOLE_DIA), mm_to_nm(MOUNT_HOLE_DIA)))
        pad.SetPosition(pcbnew.VECTOR2I(mm_to_nm(x), mm_to_nm(y)))
        pad.SetLayerSet(pad.NPTHMask())
        board.Add(pad)

    print(f"[OK] 4x mounting holes M3 ({MOUNT_HOLE_DIA}mm) en esquinas")


def add_silkscreen_labels(board):
    """Añadir textos de referencia en la silkscreen."""
    texts = [
        ("GIGASPARK MEGA v1.0", BOARD_W_MM / 2, BOARD_H_MM / 2, 2.0),
        ("100mm x 80mm 4-Layer", BOARD_W_MM / 2, BOARD_H_MM / 2 + 4, 1.5),
        ("F.Cu/In1.Cu/In2.Cu/B.Cu", BOARD_W_MM / 2, BOARD_H_MM / 2 + 8, 1.0),
    ]

    for text_str, x, y, size in texts:
        txt = pcbnew.PCB_TEXT(board)
        txt.SetText(text_str)
        txt.SetPosition(pcbnew.VECTOR2I(mm_to_nm(x), mm_to_nm(y)))
        txt.SetLayer(pcbnew.F_SilkS)
        txt.SetTextSize(pcbnew.VECTOR2I(mm_to_nm(size), mm_to_nm(size)))
        txt.SetTextThickness(mm_to_nm(0.15))
        board.Add(txt)

    print("[OK] Textos de silkscreen añadidos")


def add_zone_defines(board):
    """Añadir definiciones de zonas (ground pour en In1.Cu)."""
    # Ground pour en In1.Cu (se activara despues del netlist)
    zone = pcbnew.ZONE(board)
    zone.SetLayer(pcbnew.In1_Cu)
    zone.SetIsRuleArea(False)
    zone.SetDoNotAllowTracks(False)
    zone.SetDoNotAllowVias(False)
    zone.SetDoNotAllowPads(False)
    zone.SetDoNotAllowFootprints(False)
    zone.SetDoNotAllowCopperPour(False)

    # Outline de la zona = board outline
    outline = zone.Outline()
    outline.NewOutline()
    outline.Append(0, 0)
    outline.Append(mm_to_nm(BOARD_W_MM), 0)
    outline.Append(mm_to_nm(BOARD_W_MM), mm_to_nm(BOARD_H_MM))
    outline.Append(0, mm_to_nm(BOARD_H_MM))

    zone.SetNetCode(0)  # Sin net asignado aun (esperando netlist)
    board.Add(zone)

    # Power pour en In2.Cu (se activara despues del netlist)
    zone2 = pcbnew.ZONE(board)
    zone2.SetLayer(pcbnew.In2_Cu)
    zone2.SetIsRuleArea(False)
    outline2 = zone2.Outline()
    outline2.NewOutline()
    outline2.Append(0, 0)
    outline2.Append(mm_to_nm(BOARD_W_MM), 0)
    outline2.Append(mm_to_nm(BOARD_W_MM), mm_to_nm(BOARD_H_MM))
    outline2.Append(0, mm_to_nm(BOARD_H_MM))
    zone2.SetNetCode(0)
    board.Add(zone2)

    print("[OK] Zonas definidas: GND pour (In1.Cu) + Power pour (In2.Cu)")


# ============================================================
# MAIN
# ============================================================

def main():
    print("=" * 50)
    print("  GIGASPARK MEGA - PCB Setup Script")
    print("  KiCad pcbnew automation")
    print("=" * 50)
    print()

    board = pcbnew.GetBoard()
    if board is None:
        print("[ERROR] No hay un board abierto. Abre un PCB vacio primero.")
        return

    # Ejecutar configuracion
    set_design_rules(board)
    create_board_outline(board)
    add_mounting_holes(board)
    add_silkscreen_labels(board)
    add_zone_defines(board)

    print()
    print("=" * 50)
    print("  [OK] PCB configurado exitosamente!")
    print("  Siguiente paso: importar netlist del esquematico")
    print("  Luego: colocar componentes segun PCB_LAYOUT_GUIDE.md")
    print("=" * 50)

    # Actualizar vista
    pcbnew.Refresh()


if __name__ == "__main__":
    main()
