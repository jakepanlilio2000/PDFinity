#!/usr/bin/env python3
"""
PDFinity - Studio UI Screenshot Showcase Generator
Publisher: StackSync Solutions (https://www.stacksyncsolutions.com/)
Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)
License: Mozilla Public License Version 2.0 (MPL-2.0)
"""

import os
from PIL import Image, ImageDraw, ImageFont

def get_font(name="segoeui.ttf", size=14, bold=False):
    if bold:
        candidates = ["segoeuib.ttf", "arialbd.ttf", "segoeui.ttf"]
    else:
        candidates = [name, "arial.ttf"]
    for c in candidates:
        try:
            return ImageFont.truetype(c, size)
        except Exception:
            continue
    return ImageFont.load_default()

def draw_pill(draw, box, radius, fill, outline=None, width=1):
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)

def generate_studio_home():
    w, h = 1280, 800
    img = Image.new('RGB', (w, h), '#18181B')
    draw = ImageDraw.Draw(img)

    f_title = get_font(size=22, bold=True)
    f_h2 = get_font(size=16, bold=True)
    f_body = get_font(size=13, bold=False)
    f_body_b = get_font(size=13, bold=True)
    f_small = get_font(size=11, bold=False)
    f_icon = get_font(size=18, bold=True)

    # 1. Top Window Title Bar (Studio Dark: #18181B)
    draw.rectangle([0, 0, w, 36], fill='#18181B')
    draw.text((16, 8), "PDFinity Studio 1.1.0 — Enterprise Offline PDF Suite", fill='#F4F4F5', font=f_small)
    draw.text((w - 240, 8), "🔒 100% Offline • Zero Telemetry", fill='#10B981', font=f_small)

    # 2. Studio Tab Bar
    draw.rectangle([0, 36, w, 76], fill='#1E2024')
    draw.line([(0, 76), (w, 76)], fill='#34373E', width=1)

    # Pinned Home Tab (Active)
    draw.rectangle([0, 36, 110, 76], fill='#27292E')
    draw.line([(0, 74), (110, 74)], fill='#DC2626', width=3) # Crimson active underline
    draw.text((20, 48), "🏠 Home", fill='#F4F4F5', font=f_body_b)

    # Inactive tab
    draw.rectangle([114, 40, 340, 72], fill='#1E2024')
    draw.text((126, 48), "📄 annual_audit_report.pdf", fill='#A1A1AA', font=f_body)
    draw.text((320, 48), "✕", fill='#71717A', font=f_small)

    # Tab corner tools
    draw.text((w - 140, 48), "➕  ▾   🔍   🌓", fill='#A1A1AA', font=f_body)

    # 3. Central Pasteboard / Dashboard Backdrop (#111214)
    draw.rectangle([0, 77, w, h - 28], fill='#111214')

    # Hero Banner
    draw.text((80, 105), "PDFinity Document Studio", fill='#F4F4F5', font=f_title)
    draw.text((80, 138), "Enterprise offline-first PDF suite with high-assurance cryptographic security and dual-engine architecture.", fill='#A1A1AA', font=f_body)

    # 4. Quick Action Cards Grid (2 rows x 4 columns)
    cards = [
        ("📑", "Organize Pages", "Assembly, drag reorder, split & merge"),
        ("✏️", "Edit Content", "Native text editing, vector shapes & fonts"),
        ("💬", "Annotate & Review", "Highlights, sticky notes, callouts, ink"),
        ("📋", "Form Inspector", "AcroForms, calculated fields & flattening"),
        ("🔒", "Protect & Redact", "Deep glyph excision & XMP sanitization"),
        ("🔍", "OCR Studio", "Local Tesseract searchable text layer"),
        ("🧠", "Intelligence", "Structural xref tree & metadata inspector"),
        ("⚙️", "Batch Automation", "Multi-file batch profiles & CLI automation"),
    ]

    card_w, card_h = 260, 105
    start_x, start_y = 80, 180
    gap_x, gap_y = 24, 20

    for i, (icon, title, desc) in enumerate(cards):
        row = i // 4
        col = i % 4
        cx = start_x + col * (card_w + gap_x)
        cy = start_y + row * (card_h + gap_y)

        # Card container (#27292E surface, #34373E hairline border)
        draw_pill(draw, [cx, cy, cx + card_w, cy + card_h], 8, '#27292E', '#34373E', 1)
        # Accent left indicator on card 0
        if i == 0:
            draw.line([(cx + 2, cy + 12), (cx + 2, cy + card_h - 12)], fill='#DC2626', width=3)

        draw.text((cx + 18, cy + 16), icon, fill='#DC2626', font=f_icon)
        draw.text((cx + 52, cy + 18), title, fill='#F4F4F5', font=f_h2)
        draw.text((cx + 18, cy + 54), desc, fill='#A1A1AA', font=f_small)

    # 5. Recent Documents Section
    draw.text((80, 440), "Recent Documents & Projects", fill='#F4F4F5', font=f_h2)
    draw_pill(draw, [w - 240, 435, w - 80, 465], 6, '#DC2626')
    draw.text((w - 225, 442), "📂 Open PDF File...", fill='#FFFFFF', font=f_body_b)

    recents = [
        ("contract_master_services_2026.pdf", "16 pages • 2.4 MB", "Today, 18:42", "✓ Signed (PAdES)"),
        ("annual_financial_audit_q3.pdf", "42 pages • 8.1 MB", "Yesterday, 14:15", "✓ PDF/A-1b Compliant"),
        ("sanitized_merger_records.pdf", "8 pages • 1.1 MB", "Nov 12, 2026", "🛡️ Irreversible Redactions Applied"),
        ("product_technical_schematic.pdf", "28 pages • 12.3 MB", "Nov 10, 2026", "Layers • Form Fields"),
    ]

    ry = 480
    for title, meta, date, badge in recents:
        draw_pill(draw, [80, ry, w - 80, ry + 46], 6, '#1E2024', '#34373E', 1)
        draw.text((100, ry + 14), "📄", fill='#DC2626', font=f_body)
        draw.text((128, ry + 14), title, fill='#F4F4F5', font=f_body_b)
        draw.text((450, ry + 15), meta, fill='#71717A', font=f_small)
        draw.text((680, ry + 15), date, fill='#71717A', font=f_small)

        # Status badge
        badge_fill = '#052E16' if '✓' in badge else ('#450A0A' if '🛡️' in badge else '#18181B')
        badge_text = '#4ADE80' if '✓' in badge else ('#F87171' if '🛡️' in badge else '#94A3B8')
        draw_pill(draw, [w - 360, ry + 10, w - 100, ry + 36], 4, badge_fill)
        draw.text((w - 350, ry + 14), badge, fill=badge_text, font=f_small)
        ry += 54

    # 6. Status Bar
    draw.rectangle([0, h - 28, w, h], fill='#18181B')
    draw.line([(0, h - 28), (w, h - 28)], fill='#34373E', width=1)
    draw.text((16, h - 20), "Ready  |  Dual-Engine: Google PDFium 6.8 & QPDF 11.9  |  MPL-2.0  |  StackSync Solutions", fill='#71717A', font=f_small)
    draw.text((w - 220, h - 20), "Lead Programmer: Jake Ashley Panlilio", fill='#A1A1AA', font=f_small)

    out_path = os.path.join(os.path.dirname(__file__), '..', 'docs', 'screenshots', 'studio-home.png')
    img.save(out_path, format='PNG')
    print(f"[+] Generated: {out_path}")

def generate_studio_workspace_dark():
    w, h = 1280, 800
    img = Image.new('RGB', (w, h), '#18181B')
    draw = ImageDraw.Draw(img)

    f_h1 = get_font(size=18, bold=True)
    f_h2 = get_font(size=14, bold=True)
    f_body = get_font(size=12, bold=False)
    f_body_b = get_font(size=12, bold=True)
    f_small = get_font(size=10, bold=False)
    f_icon = get_font(size=14, bold=True)

    # 1. Title bar
    draw.rectangle([0, 0, w, 32], fill='#18181B')
    draw.text((16, 8), "PDFinity Studio 1.1.0 — contract_master_services_2026.pdf", fill='#F4F4F5', font=f_small)
    draw.text((w - 180, 8), "🔒 100% Offline Core", fill='#10B981', font=f_small)

    # 2. Studio Tab Bar
    draw.rectangle([0, 32, w, 68], fill='#1E2024')
    draw.line([(0, 68), (w, 68)], fill='#34373E', width=1)
    draw.text((18, 44), "🏠 Home", fill='#A1A1AA', font=f_body)
    # Active tab
    draw.rectangle([80, 32, 380, 68], fill='#222428')
    draw.line([(80, 66), (380, 66)], fill='#DC2626', width=3)
    draw.text((95, 44), "● contract_master_services_2026.pdf (16 p.)", fill='#F4F4F5', font=f_body_b)
    draw.text((360, 44), "✕", fill='#71717A', font=f_small)
    draw.text((w - 130, 44), "➕  ▾   🔍   🌓", fill='#A1A1AA', font=f_body)

    # 3. 2-Tier Contextual Workflow Ribbon
    # Tier 1: Mode Tabs
    draw.rectangle([0, 69, w, 99], fill='#222428')
    draw.line([(0, 99), (w, 99)], fill='#34373E', width=1)
    modes = ["All Tools", "View", "Comment", "Edit", "Organize", "Fill & Sign", "Protect & Redact", "Convert & OCR"]
    mx = 20
    for m in modes:
        is_active = (m == "Comment")
        if is_active:
            draw_pill(draw, [mx - 6, 73, mx + len(m)*8 + 6, 95], 4, '#DC2626')
            draw.text((mx, 77), m, fill='#FFFFFF', font=f_body_b)
        else:
            draw.text((mx, 77), m, fill='#A1A1AA', font=f_body)
        mx += len(m)*8 + 24

    # Tier 2: Tool Strip
    draw.rectangle([0, 100, w, 142], fill='#27292E')
    draw.line([(0, 142), (w, 142)], fill='#34373E', width=1)
    tools = [
        ("V", "Select"), ("H", "Hand"), "|",
        ("🖍", "Highlight"), ("̲U", "Underline"), ("̶S", "Strikeout"),
        ("📝", "Note"), ("✏", "Ink Draw"), ("▢", "Rectangle"), ("💬", "Callout"), ("⭐", "Stamp"),
        "|", ("🎨", "Color: #DC2626"), ("─", "Stroke: 3.0pt"), ("◐", "Opacity: 85%")
    ]
    tx = 20
    for item in tools:
        if item == "|":
            draw.line([(tx + 4, 108), (tx + 4, 134)], fill='#34373E', width=1)
            tx += 14
        else:
            icon, lbl = item
            is_sel = (lbl == "Highlight")
            bg = '#DC2626' if is_sel else '#1E2024'
            fg = '#FFFFFF' if is_sel else '#F4F4F5'
            draw_pill(draw, [tx, 106, tx + len(lbl)*7 + 26, 136], 4, bg, '#34373E', 1)
            draw.text((tx + 6, 114), icon, fill=fg, font=f_small)
            draw.text((tx + 22, 114), lbl, fill=fg, font=f_small)
            tx += len(lbl)*7 + 34

    # 4. 3-Pane Body: Left Rail (44px) + Drawer (200px) | Canvas | Right Inspector (280px)
    rail_w = 44
    drawer_w = 180
    insp_w = 280

    # Left Icon Rail (#1E2024)
    draw.rectangle([0, 143, rail_w, h - 28], fill='#1E2024')
    draw.line([(rail_w, 143), (rail_w, h - 28)], fill='#34373E', width=1)
    rail_icons = ["📑", "🔖", "📎", "🥞", "💬", "✍", "⚖"]
    ry = 160
    for idx, ic in enumerate(rail_icons):
        if idx == 0:
            draw_pill(draw, [4, ry - 4, rail_w - 4, ry + 28], 4, '#DC2626')
            draw.text((14, ry + 2), ic, fill='#FFFFFF', font=f_icon)
        else:
            draw.text((14, ry + 2), ic, fill='#A1A1AA', font=f_icon)
        ry += 42

    # Left Drawer (#222428) — Page Thumbnails
    draw.rectangle([rail_w, 143, rail_w + drawer_w, h - 28], fill='#222428')
    draw.line([(rail_w + drawer_w, 143), (rail_w + drawer_w, h - 28)], fill='#34373E', width=1)
    draw.text((rail_w + 14, 155), "PAGE THUMBNAILS (16)", fill='#A1A1AA', font=f_small)

    # 3 Thumbnails
    ty = 180
    for p_num in range(1, 4):
        is_cur = (p_num == 3)
        outline_c = '#DC2626' if is_cur else '#34373E'
        draw_pill(draw, [rail_w + 16, ty, rail_w + drawer_w - 16, ty + 150], 4, '#18181B', outline_c, 2 if is_cur else 1)
        # White sheet miniature
        draw.rectangle([rail_w + 26, ty + 10, rail_w + drawer_w - 26, ty + 130], fill='#FFFFFF')
        draw.text((rail_w + 32, ty + 14), f"Page {p_num}", fill='#18181B', font=f_small)
        for ly in range(ty + 34, ty + 120, 12):
            draw.rectangle([rail_w + 32, ly, rail_w + drawer_w - 32, ly + 4], fill='#E5E7EB')
        draw.text((rail_w + drawer_w // 2 - 8, ty + 134), f"{p_num}", fill='#DC2626' if is_cur else '#A1A1AA', font=f_small)
        ty += 166

    # Right Inspector Panel (#1E2024)
    insp_left = w - insp_w
    draw.rectangle([insp_left, 143, w, h - 28], fill='#1E2024')
    draw.line([(insp_left, 143), (insp_left, h - 28)], fill='#34373E', width=1)

    draw.text((insp_left + 16, 155), "PROPERTIES & INSPECTOR", fill='#F4F4F5', font=f_h2)
    draw_pill(draw, [insp_left + 16, 185, w - 16, 320], 6, '#27292E', '#34373E', 1)
    draw.text((insp_left + 28, 198), "Highlight Properties", fill='#DC2626', font=f_body_b)
    draw.text((insp_left + 28, 226), "Color:  Crimson Accent (#DC2626)", fill='#F4F4F5', font=f_small)
    draw.text((insp_left + 28, 250), "Opacity:  85% (Multiply)", fill='#F4F4F5', font=f_small)
    draw.text((insp_left + 28, 274), "Author:  Jake Ashley Panlilio", fill='#A1A1AA', font=f_small)
    draw.text((insp_left + 28, 296), "Date:  2026-11-14 18:42", fill='#71717A', font=f_small)

    draw_pill(draw, [insp_left + 16, 335, w - 16, 520], 6, '#27292E', '#34373E', 1)
    draw.text((insp_left + 28, 348), "Preflight & Compliance Check", fill='#10B981', font=f_body_b)
    draw.text((insp_left + 28, 376), "✓ PDF/A-1b Standard Valid", fill='#F4F4F5', font=f_small)
    draw.text((insp_left + 28, 400), "✓ All Fonts Embedded (CID/TrueType)", fill='#F4F4F5', font=f_small)
    draw.text((insp_left + 28, 424), "✓ Device-Independent sRGB Profile", fill='#F4F4F5', font=f_small)
    draw.text((insp_left + 28, 448), "✓ PAdES-BES Digital Signature Valid", fill='#F4F4F5', font=f_small)
    draw.text((insp_left + 28, 472), "✓ 0 Unsanitized Attachments", fill='#F4F4F5', font=f_small)
    draw.text((insp_left + 28, 494), "Verified by StackSync Engine", fill='#71717A', font=f_small)

    # 5. Center Viewport Canvas (#111214) with Elevated Sheet Rendering
    canvas_x1 = rail_w + drawer_w
    canvas_x2 = insp_left
    canvas_cx = canvas_x1 + (canvas_x2 - canvas_x1) // 2

    draw.rectangle([canvas_x1, 143, canvas_x2, h - 28], fill='#111214')

    # Top & Left Rulers
    draw.rectangle([canvas_x1, 143, canvas_x2, 158], fill='#1E2024')
    draw.line([(canvas_x1, 158), (canvas_x2, 158)], fill='#34373E', width=1)
    for rx in range(canvas_x1 + 20, canvas_x2, 25):
        draw.line([(rx, 150), (rx, 158)], fill='#71717A', width=1)

    draw.rectangle([canvas_x1, 159, canvas_x1 + 16, h - 28], fill='#1E2024')
    draw.line([(canvas_x1 + 16, 159), (canvas_x1 + 16, h - 28)], fill='#34373E', width=1)
    for ry_tick in range(170, h - 30, 25):
        draw.line([(canvas_x1 + 8, ry_tick), (canvas_x1 + 16, ry_tick)], fill='#71717A', width=1)

    # Elevated Document Sheet (Centered)
    doc_w = 440
    doc_h = 560
    doc_left = canvas_cx - doc_w // 2
    doc_top = 180

    # Multi-layered soft drop shadow
    draw.rectangle([doc_left + 8, doc_top + 8, doc_left + doc_w + 8, doc_top + doc_h + 8], fill='#070708')
    draw.rectangle([doc_left + 4, doc_top + 4, doc_left + doc_w + 4, doc_top + doc_h + 4], fill='#0B0C0E')
    # White page sheet
    draw.rectangle([doc_left, doc_top, doc_left + doc_w, doc_top + doc_h], fill='#FFFFFF', outline='#34373E', width=1)

    # Document contents
    draw.text((doc_left + 36, doc_top + 36), "MASTER PROFESSIONAL SERVICES AGREEMENT", fill='#0F172A', font=f_h2)
    draw.text((doc_left + 36, doc_top + 60), "StackSync Solutions — Enterprise Software Licensing Division", fill='#64748B', font=f_small)
    draw.line([(doc_left + 36, doc_top + 76), (doc_left + doc_w - 36, doc_top + 76)], fill='#E2E8F0', width=1)

    # Paragraph lines
    py = doc_top + 96
    for _ in range(4):
        draw.rectangle([doc_left + 36, py, doc_left + doc_w - 36, py + 8], fill='#F1F5F9')
        py += 18

    # Highlighted region with touch/resize handles
    hl_box = [doc_left + 36, py, doc_left + doc_w - 60, py + 22]
    draw.rectangle(hl_box, fill='#FECACA')
    draw.text((doc_left + 42, py + 4), "All document computation operates 100% offline.", fill='#991B1B', font=f_small)
    # 4 corner handles
    for hx, hy in [(hl_box[0], hl_box[1]), (hl_box[2], hl_box[1]), (hl_box[0], hl_box[3]), (hl_box[2], hl_box[3])]:
        draw.rectangle([hx - 3, hy - 3, hx + 3, hy + 3], fill='#DC2626')
    py += 34

    for _ in range(8):
        draw.rectangle([doc_left + 36, py, doc_left + doc_w - 36, py + 8], fill='#F1F5F9')
        py += 18

    # Cryptographic signature box in document
    draw_pill(draw, [doc_left + 36, py + 10, doc_left + doc_w - 36, py + 75], 4, '#F0FDF4', '#86EFAC', 1)
    draw.text((doc_left + 48, py + 20), "✓ PAdES-BES Cryptographic Signature Verified", fill='#166534', font=f_small)
    draw.text((doc_left + 48, py + 38), "Signer: Jake Ashley Panlilio (StackSync Solutions)", fill='#15803D', font=f_small)
    draw.text((doc_left + 48, py + 54), "SHA-256 Digest Match • Zero Modifications", fill='#166534', font=f_small)

    # 6. Floating Canvas HUD Overlay (Translucent pill anchored bottom-center)
    hud_w = 480
    hud_h = 42
    hud_left = canvas_cx - hud_w // 2
    hud_top = h - 85
    draw_pill(draw, [hud_left, hud_top, hud_left + hud_w, hud_top + hud_h], 21, '#18181BE6', '#34373E', 1)
    hud_content = "▲   3 / 16   ▼   |   [V]   [H]   |   −   125%   +   |   ≡   ⚏   |   ↷"
    draw.text((hud_left + 45, hud_top + 13), hud_content, fill='#F4F4F5', font=f_body_b)

    # 7. Status bar
    draw.rectangle([0, h - 28, w, h], fill='#18181B')
    draw.line([(0, h - 28), (w, h - 28)], fill='#34373E', width=1)
    draw.text((16, h - 20), "Page 3 of 16  |  Zoom: 125%  |  ISO 32000-1  |  Google PDFium Sub-Pixel Engine", fill='#71717A', font=f_small)
    draw.text((w - 240, h - 20), "📡 Remote Companion: Inactive", fill='#71717A', font=f_small)

    out_path = os.path.join(os.path.dirname(__file__), '..', 'docs', 'screenshots', 'studio-workspace-dark.png')
    img.save(out_path, format='PNG')
    print(f"[+] Generated: {out_path}")

def generate_studio_tools_inspector():
    w, h = 1280, 800
    img = Image.new('RGB', (w, h), '#18181B')
    draw = ImageDraw.Draw(img)

    f_h1 = get_font(size=18, bold=True)
    f_h2 = get_font(size=14, bold=True)
    f_body = get_font(size=12, bold=False)
    f_body_b = get_font(size=12, bold=True)
    f_small = get_font(size=10, bold=False)
    f_icon = get_font(size=16, bold=True)

    # Header
    draw.rectangle([0, 0, w, 36], fill='#18181B')
    draw.text((20, 10), "PDFinity Studio — Tool & Property Inspector Diagnostics", fill='#F4F4F5', font=f_body_b)
    draw.text((w - 220, 10), "ISO 32000-1 / ISO 19005-1", fill='#10B981', font=f_small)

    # Split: Document View on Left (820px) and Full Inspector Palette on Right (460px)
    left_w = 820
    draw.rectangle([0, 36, left_w, h], fill='#111214')
    draw.rectangle([left_w, 36, w, h], fill='#1E2024')
    draw.line([(left_w, 36), (left_w, h)], fill='#34373E', width=2)

    # Document on left
    draw_pill(draw, [80, 60, left_w - 80, h - 50], 4, '#FFFFFF', '#34373E', 1)
    draw.text((120, 100), "ANNUAL REGULATORY & COMPLIANCE FILING", fill='#0F172A', font=f_h1)
    draw.text((120, 130), "StackSync Solutions — Global Compliance Division", fill='#64748B', font=f_small)

    py = 170
    for _ in range(5):
        draw.rectangle([120, py, left_w - 120, py + 10], fill='#F1F5F9')
        py += 22

    # Annotation in document
    draw.rectangle([120, py, left_w - 200, py + 26], fill='#FECACA')
    draw.text((130, py + 6), "Audit requirement: All user documents must execute in memory without cloud transmission.", fill='#991B1B', font=f_small)
    py += 45

    for _ in range(12):
        draw.rectangle([120, py, left_w - 120, py + 10], fill='#F1F5F9')
        py += 22

    # Right Inspector Panel (#1E2024)
    ix = left_w + 24
    draw.text((ix, 55), "CONTEXTUAL TOOL & PROPERTY INSPECTOR", fill='#F4F4F5', font=f_h2)
    draw.line([(ix, 80), (w - 24, 80)], fill='#34373E', width=1)

    # Card 1: Annotation Properties
    draw_pill(draw, [ix, 95, w - 24, 280], 6, '#27292E', '#34373E', 1)
    draw.text((ix + 16, 110), "Annotation Properties (Highlight)", fill='#DC2626', font=f_body_b)

    # Color swatches
    draw.text((ix + 16, 140), "Color Swatch:", fill='#A1A1AA', font=f_small)
    swatches = ['#DC2626', '#F59E0B', '#10B981', '#0284C7', '#8B5CF6']
    for idx, c in enumerate(swatches):
        draw.ellipse([ix + 100 + idx*32, 136, ix + 124 + idx*32, 160], fill=c)
        if idx == 0:
            draw.ellipse([ix + 98, 134, ix + 126, 162], outline='#FFFFFF', width=2)

    draw.text((ix + 16, 175), "Stroke Width:  3.0 pt", fill='#F4F4F5', font=f_small)
    draw_pill(draw, [ix + 120, 180, w - 40, 184], 2, '#34373E')
    draw.ellipse([ix + 200, 175, ix + 214, 189], fill='#DC2626')

    draw.text((ix + 16, 205), "Opacity:  85%", fill='#F4F4F5', font=f_small)
    draw_pill(draw, [ix + 120, 210, w - 40, 214], 2, '#34373E')
    draw.ellipse([ix + 280, 205, ix + 294, 219], fill='#DC2626')

    draw.text((ix + 16, 235), "Author:  Jake Ashley Panlilio", fill='#A1A1AA', font=f_small)
    draw.text((ix + 16, 255), "Subject:  Security Invariant Check", fill='#71717A', font=f_small)

    # Card 2: PKCS#7 Digital Signature Validation
    draw_pill(draw, [ix, 295, w - 24, 490], 6, '#27292E', '#34373E', 1)
    draw.text((ix + 16, 310), "Digital Signature Details (PAdES)", fill='#10B981', font=f_body_b)
    sig_info = [
        ("Status:", "✓ Cryptographically Valid"),
        ("Signer Name:", "Jake Ashley Panlilio"),
        ("Organization:", "StackSync Solutions"),
        ("Certificate Type:", "X.509 v3 RSA-2048 / SHA-256"),
        ("Timestamp:", "2026-11-14 18:42:10 UTC"),
        ("Byte Range:", "Entire Document Protected")
    ]
    sy = 338
    for lbl, val in sig_info:
        draw.text((ix + 16, sy), lbl, fill='#A1A1AA', font=f_small)
        draw.text((ix + 130, sy), val, fill='#4ADE80' if '✓' in val else '#F4F4F5', font=f_small)
        sy += 24

    # Card 3: Preflight & Standards Validation
    draw_pill(draw, [ix, 505, w - 24, 710], 6, '#27292E', '#34373E', 1)
    draw.text((ix + 16, 520), "Preflight & ISO Standards Compliance", fill='#38BDF8', font=f_body_b)
    checks = [
        ("• ISO 32000-1 (PDF 1.7):", "100% Conforming syntax"),
        ("• ISO 19005-1 (PDF/A-1b):", "Passed — Long-Term Archive"),
        ("• Font Embedding:", "All 12 typefaces embedded"),
        ("• Color Management:", "Device-independent RGB space"),
        ("• Metadata Consistency:", "XMP and Info dict match"),
        ("• Executable Javascript:", "0 disallowed streams detected"),
        ("• Structural Hierarchy:", "Valid Tagged PDF Document")
    ]
    cy = 548
    for lbl, val in checks:
        draw.text((ix + 16, cy), lbl, fill='#F4F4F5', font=f_small)
        draw.text((ix + 190, cy), val, fill='#86EFAC' if 'Pass' in val or '100%' in val or 'All' in val else '#94A3B8', font=f_small)
        cy += 22

    # Bottom action button
    draw_pill(draw, [ix, h - 60, w - 24, h - 20], 6, '#DC2626')
    draw.text((ix + 80, h - 45), "⚡ GENERATE COMPLIANCE REPORT (PDF/A)", fill='#FFFFFF', font=f_body_b)

    out_path = os.path.join(os.path.dirname(__file__), '..', 'docs', 'screenshots', 'studio-tools-inspector.png')
    img.save(out_path, format='PNG')
    print(f"[+] Generated: {out_path}")

def generate_studio_mobile():
    w, h = 720, 1280
    img = Image.new('RGB', (w, h), '#111214')
    draw = ImageDraw.Draw(img)

    f_title = get_font(size=20, bold=True)
    f_h2 = get_font(size=16, bold=True)
    f_body = get_font(size=14, bold=False)
    f_body_b = get_font(size=14, bold=True)
    f_small = get_font(size=12, bold=False)
    f_icon = get_font(size=22, bold=True)

    # 1. Bezel Status Bar
    draw.rectangle([0, 0, w, 40], fill='#000000')
    draw.text((28, 12), "10:30", fill='#FFFFFF', font=f_small)
    draw.text((w - 180, 12), "5G  100% 🔋", fill='#FFFFFF', font=f_small)

    # 2. Mobile Top App Bar (52px height)
    draw.rectangle([0, 40, w, 92], fill='#1E2024')
    draw.line([(0, 92), (w, 92)], fill='#34373E', width=1)
    draw.text((24, 54), "←", fill='#F4F4F5', font=f_icon)
    draw.text((70, 56), "contract_enterprise.pdf", fill='#F4F4F5', font=f_h2)
    draw.text((w - 150, 56), "🔍   🌓   ⋮", fill='#A1A1AA', font=f_body_b)

    # 3. Canvas with Elevated Document Sheet
    doc_x = 40
    doc_y = 110
    doc_w = w - 80
    doc_h = h - 200

    draw.rectangle([doc_x + 6, doc_y + 6, doc_x + doc_w + 6, doc_y + doc_h + 6], fill='#070708')
    draw.rectangle([doc_x, doc_y, doc_x + doc_w, doc_y + doc_h], fill='#FFFFFF', outline='#34373E', width=1)

    draw.text((doc_x + 36, doc_y + 36), "ENTERPRISE AGREEMENT", fill='#0F172A', font=f_title)
    draw.text((doc_x + 36, doc_y + 68), "StackSync Solutions — Touch Optimization", fill='#64748B', font=f_small)
    draw.line([(doc_x + 36, doc_y + 90), (doc_x + doc_w - 36, doc_y + 90)], fill='#E2E8F0', width=1)

    py = doc_y + 114
    for _ in range(8):
        draw.rectangle([doc_x + 36, py, doc_x + doc_w - 36, py + 14], fill='#F1F5F9')
        py += 26

    # Touch highlight with 12dp handles
    hl_box = [doc_x + 36, py, doc_x + doc_w - 50, py + 30]
    draw.rectangle(hl_box, fill='#FECACA')
    draw.text((doc_x + 42, py + 6), "Responsive breakpoint layout with 48dp touch targets.", fill='#991B1B', font=f_small)
    for hx, hy in [(hl_box[0], hl_box[1]), (hl_box[2], hl_box[1]), (hl_box[0], hl_box[3]), (hl_box[2], hl_box[3])]:
        draw.rectangle([hx - 6, hy - 6, hx + 6, hy + 6], fill='#DC2626')
    py += 48

    for _ in range(10):
        draw.rectangle([doc_x + 36, py, doc_x + doc_w - 36, py + 14], fill='#F1F5F9')
        py += 26

    # 4. Mobile Bottom Sheet (Sliding up from bottom, height 440px)
    sheet_h = 460
    sheet_top = h - 68 - sheet_h

    # Scrim / dim overlay
    draw.rectangle([0, 92, w, sheet_top], fill='#00000088')

    # Sheet container (#1E2024 with rounded top corners)
    draw_pill(draw, [0, sheet_top, w, h - 68], 16, '#1E2024', '#34373E', 1)

    # Top drag handle
    draw_pill(draw, [w // 2 - 24, sheet_top + 10, w // 2 + 24, sheet_top + 15], 2, '#71717A')

    # Sheet header
    draw.text((32, sheet_top + 28), "Document Tools Palette", fill='#F4F4F5', font=f_h2)
    draw_pill(draw, [w - 68, sheet_top + 22, w - 24, sheet_top + 66], 6, '#27292E')
    draw.text((w - 52, sheet_top + 34), "✕", fill='#A1A1AA', font=f_body_b)

    # Grid of tool buttons (48dp touch targets)
    mobile_tools = [
        ("📑", "Organize Pages"), ("🗂", "Merge PDF"),
        ("✂", "Split PDF"), ("✏", "Edit Content"),
        ("📋", "Form Inspector"), ("🔍", "OCR Studio"),
        ("⚡", "Optimizer"), ("🧠", "Intelligence")
    ]
    card_w, card_h = 300, 64
    for idx, (ic, title) in enumerate(mobile_tools):
        r = idx // 2
        c = idx % 2
        bx = 36 + c * (card_w + 24)
        by = sheet_top + 80 + r * (card_h + 16)
        draw_pill(draw, [bx, by, bx + card_w, by + card_h], 8, '#27292E', '#34373E', 1)
        draw.text((bx + 20, by + 18), ic, fill='#DC2626', font=f_icon)
        draw.text((bx + 64, by + 24), title, fill='#F4F4F5', font=f_body_b)

    # 5. Mobile Bottom Navigation Bar (56px height)
    draw.rectangle([0, h - 68, w, h], fill='#222428')
    draw.line([(0, h - 68), (w, h - 68)], fill='#34373E', width=1)
    navs = [
        ("⚒", "Tools", True),
        ("💬", "Comment", False),
        ("📄", "Pages", False),
        ("✍", "Sign", False),
        ("⋯", "More", False)
    ]
    nx = 30
    for ic, lbl, is_active in navs:
        fg = '#DC2626' if is_active else '#A1A1AA'
        draw.text((nx + 18, h - 56), ic, fill=fg, font=f_body)
        draw.text((nx + 8, h - 34), lbl, fill=fg, font=f_small)
        nx += 140

    out_path = os.path.join(os.path.dirname(__file__), '..', 'docs', 'screenshots', 'studio-mobile.png')
    img.save(out_path, format='PNG')
    print(f"[+] Generated: {out_path}")

def main():
    print("Generating refreshed Studio UI showcase screenshots...")
    generate_studio_home()
    generate_studio_workspace_dark()
    generate_studio_tools_inspector()
    generate_studio_mobile()
    print("All Studio UI screenshots successfully generated!")

if __name__ == '__main__':
    main()
