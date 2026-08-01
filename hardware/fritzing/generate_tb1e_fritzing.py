#!/usr/bin/env python3
"""Generate TB-1E Fritzing parts (ESP32 38-pin, CW-022 4CH) and wiring sketch (.fzz)."""

import html
import os
import uuid
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PARTS_DIR = ROOT / "parts"
SVG_DIR = PARTS_DIR / "svg"
OUT_FZZ = ROOT / "TB-1E_ESP32_CW022_Wiring.fzz"

# ESP32 DevKit V1 style 38-pin labels (left column top→bottom, right column top→bottom)
ESP32_LEFT = [
    "3V3", "EN", "VP", "VN", "GPIO34", "GPIO35", "GPIO32", "GPIO33",
    "GPIO25", "GPIO26", "GPIO27", "GPIO14", "GPIO12", "GPIO13", "GND", "VIN",
    "NC1", "NC2", "NC3",
]
ESP32_RIGHT = [
    "GND", "GPIO23", "GPIO22", "TX0", "RX0", "GPIO21", "GPIO19", "GPIO18",
    "GPIO5", "GPIO17", "GPIO16", "GPIO4", "GPIO0", "GPIO2", "GPIO15", "GND",
    "NC4", "NC5", "NC6",
]
ESP32_PINS = ESP32_LEFT + ESP32_RIGHT

CW022_PINS = ["GND", "IN1", "IN2", "IN3", "IN4", "VCC"]

# TB-1E motor relay wiring (ESP32 GPIO name -> CW-022 input)
MOTOR_WIRES = [
    ("VIN", "VCC", "#cc0000"),
    ("GND", "GND", "#000000"),
    ("GPIO26", "IN1", "#3366cc"),
    ("GPIO27", "IN2", "#3366cc"),
    ("GPIO14", "IN3", "#009900"),
]


def pin_id(index: int) -> str:
    return f"connector{index}"


def make_rect_svg(view: str, width: float, height: float, pins, left_side: bool) -> str:
    """Build breadboard/schematic SVG with connector pins."""
    lines = [
        '<?xml version="1.0" encoding="UTF-8" standalone="no"?>',
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}in" height="{height}in" '
        f'viewBox="0 0 {width * 100} {height * 100}" id="svg">',
        f'<g id="{view}">',
    ]
    if view == "breadboard":
        lines.append(
            f'<rect x="5" y="5" width="{width*100-10}" height="{height*100-10}" '
            'rx="4" fill="#006600" stroke="#003300" stroke-width="2"/>'
        )
    else:
        lines.append(
            f'<rect x="10" y="10" width="{width*100-20}" height="{height*100-20}" '
            'fill="none" stroke="#000000" stroke-width="2"/>'
        )

    n = len(pins)
    for i, name in enumerate(pins):
        if left_side:
            row = i
            cx, cy = 8, 20 + row * (height * 100 - 40) / max(n - 1, 1)
            tx = 18
        else:
            # right side pins for dual-row parts handled by caller
            row = i
            cx, cy = width * 100 - 8, 20 + row * (height * 100 - 40) / max(n - 1, 1)
            tx = width * 100 - 55

        pin_suffix = "pin" if view == "breadboard" else "pin"
        term_suffix = "terminal" if view == "schematic" else ""
        cid = pin_id(i if left_side else i)

        if view == "breadboard":
            lines.append(
                f'<rect id="{cid}pin" x="{cx-4}" y="{cy-3}" width="8" height="6" '
                f'fill="#cccccc" stroke="#888888"/>'
            )
        else:
            lines.append(f'<circle id="{cid}pin" cx="{cx}" cy="{cy}" r="3" fill="none" stroke="#000"/>')
            lines.append(f'<rect id="{cid}terminal" x="{cx-2}" y="{cy-2}" width="4" height="4" fill="none"/>')

        label = html.escape(name)
        lines.append(
            f'<text x="{tx}" y="{cy+3}" font-size="7" font-family="Arial" fill="#ffffff">{label}</text>'
            if view == "breadboard"
            else f'<text x="{tx}" y="{cy+3}" font-size="7" font-family="Arial">{label}</text>'
        )

    lines.append("</g></svg>")
    return "\n".join(lines)


def make_esp32_breadboard_svg() -> str:
    w, h = 2.6, 4.8
    lines = [
        '<?xml version="1.0" encoding="UTF-8" standalone="no"?>',
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}in" height="{h}in" '
        f'viewBox="0 0 {w*100} {h*100}">',
        '<g id="breadboard">',
        f'<rect x="15" y="10" width="{w*100-30}" height="{h*100-20}" rx="6" fill="#00979d" stroke="#004446" stroke-width="2"/>',
        f'<text x="{w*50}" y="28" text-anchor="middle" font-size="10" fill="#fff" font-family="Arial">ESP32 DevKit</text>',
        f'<text x="{w*50}" y="40" text-anchor="middle" font-size="7" fill="#fff" font-family="Arial">38 Pin — TB-1E</text>',
    ]
    n = 19
    for i in range(n):
        y = 55 + i * ((h * 100 - 70) / (n - 1))
        # left pin
        cid_l = pin_id(i)
        lines.append(f'<rect id="{cid_l}pin" x="17" y="{y-3}" width="8" height="6" fill="#ccc" stroke="#888"/>')
        lines.append(f'<text x="28" y="{y+3}" font-size="6" fill="#fff" font-family="Arial">{html.escape(ESP32_LEFT[i])}</text>')
        # right pin
        cid_r = pin_id(i + 19)
        lines.append(f'<rect id="{cid_r}pin" x="{w*100-25}" y="{y-3}" width="8" height="6" fill="#ccc" stroke="#888"/>')
        lines.append(
            f'<text x="{w*100-72}" y="{y+3}" font-size="6" fill="#fff" text-anchor="end" '
            f'font-family="Arial">{html.escape(ESP32_RIGHT[i])}</text>'
        )
    lines.append("</g></svg>")
    return "\n".join(lines)


def make_cw022_breadboard_svg() -> str:
    w, h = 2.0, 1.6
    lines = [
        '<?xml version="1.0" encoding="UTF-8" standalone="no"?>',
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}in" height="{h}in" viewBox="0 0 {w*100} {h*100}">',
        '<g id="breadboard">',
        f'<rect x="10" y="10" width="{w*100-20}" height="{h*100-20}" rx="4" fill="#1a1a2e" stroke="#000" stroke-width="2"/>',
        f'<text x="{w*50}" y="28" text-anchor="middle" font-size="9" fill="#fff" font-family="Arial">CW-022</text>',
        f'<text x="{w*50}" y="40" text-anchor="middle" font-size="7" fill="#aaa" font-family="Arial">4CH 5V Relay</text>',
    ]
    for i, name in enumerate(CW022_PINS):
        y = 55 + i * 18
        cid = pin_id(i)
        lines.append(f'<rect id="{cid}pin" x="12" y="{y-3}" width="8" height="6" fill="#ccc" stroke="#888"/>')
        lines.append(f'<text x="24" y="{y+3}" font-size="7" fill="#fff" font-family="Arial">{html.escape(name)}</text>')
    lines.append("</g></svg>")
    return "\n".join(lines)


def make_simple_schematic_svg(title: str, pins) -> str:
    w = 2.5
    h = 0.4 + len(pins) * 0.25
    lines = [
        '<?xml version="1.0" encoding="UTF-8" standalone="no"?>',
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}in" height="{h}in" viewBox="0 0 {w*100} {h*100}">',
        '<g id="schematic">',
        f'<text x="50" y="15" font-size="9" font-family="Arial">{html.escape(title)}</text>',
    ]
    for i, name in enumerate(pins):
        y = 25 + i * 20
        cid = pin_id(i)
        lines.append(f'<line x1="10" y1="{y}" x2="30" y2="{y}" stroke="#000"/>')
        lines.append(f'<circle id="{cid}pin" cx="30" cy="{y}" r="3" fill="none" stroke="#000"/>')
        lines.append(f'<rect id="{cid}terminal" x="28" y="{y-2}" width="4" height="4" fill="none"/>')
        lines.append(f'<text x="38" y="{y+3}" font-size="7" font-family="Arial">{html.escape(name)}</text>')
    lines.append("</g></svg>")
    return "\n".join(lines)


def connector_block(start_index: int, names, pin_type="female") -> str:
    chunks = []
    for i, name in enumerate(names):
        idx = start_index + i
        cid = pin_id(idx)
        chunks.append(f"""<connector id="{cid}" type="{pin_type}" name="{html.escape(name)}">
  <description>{html.escape(name)}</description>
  <views>
    <breadboardView><p layer="breadboard" svgId="{cid}pin"/></breadboardView>
    <schematicView><p layer="schematic" svgId="{cid}pin" terminalId="{cid}terminal"/></schematicView>
    <pcbView><p layer="copper0" svgId="{cid}pin"/><p layer="copper1" svgId="{cid}pin"/></pcbView>
  </views>
</connector>""")
    return "\n".join(chunks)


def write_part(module_id: str, title: str, description: str, tags, pin_names, breadboard_svg: str, schematic_svg: str):
    part_dir = PARTS_DIR / module_id
    bb_dir = part_dir / "svg" / "breadboard"
    sc_dir = part_dir / "svg" / "schematic"
    ic_dir = part_dir / "svg" / "icon"
    for d in (bb_dir, sc_dir, ic_dir):
        d.mkdir(parents=True, exist_ok=True)

    bb_file = f"{module_id}_breadboard.svg"
    sc_file = f"{module_id}_schematic.svg"
    (bb_dir / bb_file).write_text(breadboard_svg, encoding="utf-8")
    (sc_dir / sc_file).write_text(schematic_svg, encoding="utf-8")
    (ic_dir / bb_file).write_text(breadboard_svg, encoding="utf-8")

    tag_xml = "".join(f"<tag>{html.escape(t)}</tag>" for t in tags)
    connectors = connector_block(0, pin_names)

    fzp = f"""<?xml version="1.0" encoding="UTF-8"?>
<module moduleId="{module_id}" fritzingVersion="0.9.3">
  <version>1</version>
  <author>TB-1E</author>
  <title>{html.escape(title)}</title>
  <description>{html.escape(description)}</description>
  <tags>{tag_xml}</tags>
  <properties>
    <property name="family">TB-1E</property>
    <property name="variant">{html.escape(title)}</property>
  </properties>
  <views>
    <breadboardView>
      <layers image="svg/breadboard/{bb_file}">
        <layer layerId="breadboard"/>
      </layers>
    </breadboardView>
    <schematicView>
      <layers image="svg/schematic/{sc_file}">
        <layer layerId="schematic"/>
      </layers>
    </schematicView>
    <pcbView>
      <layers image="svg/breadboard/{bb_file}">
        <layer layerId="copper0"/>
        <layer layerId="silkscreen"/>
      </layers>
    </pcbView>
    <iconView>
      <layers image="svg/breadboard/{bb_file}">
        <layer layerId="icon"/>
      </layers>
    </iconView>
  </views>
  <connectors>
{connectors}
  </connectors>
</module>
"""
    (part_dir / f"{module_id}.fzp").write_text(fzp, encoding="utf-8")
    return part_dir


def esp32_pin_index(name: str) -> int:
    if name == "GND":
        return ESP32_PINS.index("GND")
    if name.startswith("GPIO"):
        num = name.replace("GPIO", "")
        for i, p in enumerate(ESP32_PINS):
            if p == name or p == f"GPIO{num}":
                return i
    for i, p in enumerate(ESP32_PINS):
        if p == name:
            return i
    raise KeyError(name)


def cw022_pin_index(name: str) -> int:
    return CW022_PINS.index(name)


def build_sketch_fz() -> str:
    esp32_id = "TB1E_ESP32_38pin"
    cw_id = "TB1E_CW022_4ch"
    esp32_mi = 1001
    cw_mi = 1002
    wire_base = 2000

    wire_xml = []
    for i, (esp_pin, cw_pin, color) in enumerate(MOTOR_WIRES):
        w_mi = wire_base + i
        e_cid = pin_id(esp32_pin_index(esp_pin))
        c_cid = pin_id(cw022_pin_index(cw_pin))
        x1, y1 = -200 + i * 5, 150 + i * 8
        x2, y2 = 60, 180 + cw022_pin_index(cw_pin) * 12
        wire_xml.append(f"""    <instance moduleIdRef="WireModuleID" modelIndex="{w_mi}" path=":/resources/parts/core/wire.fzp">
      <title>Wire_{esp_pin}_{cw_pin}</title>
      <views>
        <breadboardView layer="breadboardWire">
          <geometry z="5" x="{x1}" y="{y1}" x1="0" y1="0" x2="{x2-x1}" y2="{y2-y1}" wireFlags="0"/>
          <wireExtras mils="22.2222" color="{color}" opacity="1" banded="0"/>
          <connectors>
            <connector connectorId="connector0" layer="breadboardWire">
              <connects>
                <connect connectorId="{e_cid}" modelIndex="{esp32_mi}" layer="breadboard"/>
              </connects>
            </connector>
            <connector connectorId="connector1" layer="breadboardWire">
              <connects>
                <connect connectorId="{c_cid}" modelIndex="{cw_mi}" layer="breadboard"/>
              </connects>
            </connector>
          </connectors>
        </breadboardView>
        <schematicView layer="schematicTrace">
          <geometry z="5" x="{x1}" y="{y1}" x1="0" y1="0" x2="{x2-x1}" y2="{y2-y1}" wireFlags="0"/>
          <wireExtras mils="9.7222" color="{color}" opacity="1" banded="0"/>
          <connectors>
            <connector connectorId="connector0" layer="schematicTrace">
              <connects>
                <connect connectorId="{e_cid}" modelIndex="{esp32_mi}" layer="schematic"/>
              </connects>
            </connector>
            <connector connectorId="connector1" layer="schematicTrace">
              <connects>
                <connect connectorId="{c_cid}" modelIndex="{cw_mi}" layer="schematic"/>
              </connects>
            </connector>
          </connectors>
        </schematicView>
        <pcbView layer="copper0trace">
          <geometry z="5" x="{x1}" y="{y1}" x1="0" y1="0" x2="{x2-x1}" y2="{y2-y1}" wireFlags="0"/>
          <wireExtras mils="11.1111" color="{color}" opacity="1" banded="0"/>
        </pcbView>
      </views>
    </instance>""")

    esp_instance = build_connected_part(esp32_id, esp32_mi, "ESP32", "-400", "50", esp32_id, wire_base, is_cw=False)
    cw_instance = build_connected_part(cw_id, cw_mi, "CW-022", "80", "120", cw_id, wire_base, is_cw=True)

    note = """TB-1E motor wiring (CW-022 active LOW):
Red=VIN to VCC | Black=GND | Blue=GPIO26 to IN1 | Blue=GPIO27 to IN2 | Green=GPIO14 to IN3
Panel: GPIO32 mode, GPIO18/19 limits, GPIO33 hold, GPIO21/22/23 buttons, GPIO4 AUTO-DOWN, GPIO25 BLE"""

    return f"""<?xml version="1.0" encoding="UTF-8"?>
<module fritzingVersion="0.9.3b">
  <views>
    <view name="breadboardView" backgroundColor="#ffffff" gridSize="0.1in" showGrid="1" alignToGrid="1" viewFromBelow="0"/>
    <view name="schematicView" backgroundColor="#ffffff" gridSize="0.1in" showGrid="1" alignToGrid="1" viewFromBelow="0"/>
    <view name="pcbView" backgroundColor="#333333" gridSize="0.05in" showGrid="1" alignToGrid="1" viewFromBelow="0"/>
  </views>
  <instances>
{esp_instance}
{cw_instance}
{chr(10).join(wire_xml)}
  </instances>
</module>
"""


def build_connected_part(module_id, model_index, title, x, y, path_suffix, wire_base, is_cw=False):
    esp32_mi = 1001
    cw_mi = 1002
    connects_xml = []
    for i, (esp_pin, cw_pin, _) in enumerate(MOTOR_WIRES):
        if is_cw:
            pin_idx = cw022_pin_index(cw_pin)
        else:
            pin_idx = esp32_pin_index(esp_pin)
        cid = pin_id(pin_idx)
        w_mi = wire_base + i
        end = "connector1" if is_cw else "connector0"
        connects_xml.append(f"""            <connector connectorId="{cid}" layer="breadboard">
              <connects>
                <connect connectorId="{end}" modelIndex="{w_mi}" layer="breadboardWire"/>
              </connects>
            </connector>""")

    return f"""    <instance moduleIdRef="{module_id}" modelIndex="{model_index}" path="{path_suffix}/{module_id}.fzp">
      <title>{html.escape(title)}</title>
      <views>
        <breadboardView layer="breadboard">
          <geometry z="2" x="{x}" y="{y}"/>
          <connectors>
{chr(10).join(connects_xml)}
          </connectors>
        </breadboardView>
        <schematicView layer="schematic">
          <geometry z="2" x="{x}" y="{y}"/>
        </schematicView>
        <pcbView layer="copper0">
          <geometry z="2" x="{x}" y="{y}"/>
        </pcbView>
      </views>
    </instance>"""


def package_fzz():
    esp32_id = "TB1E_ESP32_38pin"
    cw_id = "TB1E_CW022_4ch"

    write_part(
        esp32_id,
        "ESP32 DevKit 38 Pin",
        "ESP32-WROOM-32 DevKit module, 38 pins, for TB-1E controller.",
        ["ESP32", "TB-1E", "38pin"],
        ESP32_PINS,
        make_esp32_breadboard_svg(),
        make_simple_schematic_svg("ESP32 38pin", ESP32_PINS),
    )
    write_part(
        cw_id,
        "CW-022 4CH Relay",
        "CW-022 4-channel 5V relay module. Active LOW inputs.",
        ["relay", "CW-022", "TB-1E", "4ch"],
        CW022_PINS,
        make_cw022_breadboard_svg(),
        make_simple_schematic_svg("CW-022", CW022_PINS),
    )

    fz_content = build_sketch_fz()
    fz_name = "TB-1E_ESP32_CW022_Wiring.fz"

    with zipfile.ZipFile(OUT_FZZ, "w", zipfile.ZIP_STORED) as zf:
        zf.writestr(fz_name, fz_content)
        for part_id in (esp32_id, cw_id):
            part_root = PARTS_DIR / part_id
            for file in part_root.rglob("*"):
                if file.is_file():
                    arc = f"{part_id}/{file.relative_to(part_root).as_posix()}"
                    zf.write(file, arc)

    print(f"Created: {OUT_FZZ}")

    # Fritzing 0.9.3 QuaZip fails on Windows paths with non-ASCII characters.
    ascii_dir = Path("C:/TB1E_Fritzing")
    ascii_dir.mkdir(parents=True, exist_ok=True)
    ascii_fzz = ascii_dir / OUT_FZZ.name
    ascii_fzz.write_bytes(OUT_FZZ.read_bytes())
    print(f"Copied to: {ascii_fzz} (open this path in Fritzing)")


if __name__ == "__main__":
    package_fzz()
