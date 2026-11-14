#!/usr/bin/env python3
"""
PDFinity - Visual Assets & UI Showcase Generator
Publisher: StackSync Solutions (https://www.stacksyncsolutions.com/)
Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)
License: Mozilla Public License Version 2.0 (MPL-2.0)
"""

import os
import math
from PIL import Image, ImageDraw, ImageFont

def create_app_icon():
    size = 512
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # 1. Base squircle with crimson gradient approximation
    # Draw shadow
    draw.rounded_rectangle([28, 36, 484, 492], radius=100, fill=(0, 0, 0, 60))
    # Draw primary background
    draw.rounded_rectangle([32, 32, 480, 480], radius=96, fill='#B91C1C')
    # Inner gradient layer
    draw.rounded_rectangle([36, 36, 476, 476], radius=92, fill='#DC2626')

    # 2. Folded Page Corner Accent in top right
    corner = [(360, 32), (480, 152), (360, 152)]
    draw.polygon(corner, fill='#991B1B')
    draw.line([(360, 32), (360, 152), (480, 152)], fill='#7F1D1D', width=3)

    # 3. Draw Lemniscate of Bernoulli (Infinity Symbol)
    # Parametric: x = a*cos(t)/(1+sin^2(t)), y = b*sin(t)*cos(t)/(1+sin^2(t))
    pts = []
    steps = 400
    cx, cy = 256, 280
    a = 150
    b = 100
    for i in range(steps + 1):
        t = i * (2 * math.pi / steps)
        denom = 1 + math.sin(t) ** 2
        x = cx + (a * math.cos(t)) / denom
        y = cy + (b * math.sin(t) * math.cos(t)) / denom
        pts.append((x, y))

    # Draw thick infinity stroke
    for i in range(len(pts) - 1):
        draw.line([pts[i], pts[i+1]], fill='#FFFFFF', width=36)
        # Smooth joints
        draw.ellipse([pts[i][0]-18, pts[i][1]-18, pts[i][0]+18, pts[i][1]+18], fill='#FFFFFF')

    # Center intersection highlight notch
    draw.line([(256, 245), (256, 315)], fill='#F3F4F6', width=8)

    # Save outputs
    out_dir = os.path.join(os.path.dirname(__file__), '..', 'assets', 'icons')
    os.makedirs(out_dir, exist_ok=True)

    png_path = os.path.join(out_dir, 'pdfinity.png')
    img.save(png_path, format='PNG')
    print(f"[+] Generated: {png_path}")

    # Generate ICO
    ico_path = os.path.join(out_dir, 'pdfinity.ico')
    img.save(ico_path, format='ICO', sizes=[(256, 256), (128, 128), (64, 64), (48, 48), (32, 32), (16, 16)])
    print(f"[+] Generated: {ico_path}")

    # Generate ICNS
    icns_path = os.path.join(out_dir, 'pdfinity.icns')
    img.save(icns_path, format='ICNS')
    print(f"[+] Generated: {icns_path}")

def create_desktop_workspace_screenshot():
    w, h = 1280, 800
    img = Image.new('RGB', (w, h), '#0F172A')
    draw = ImageDraw.Draw(img)

    # 1. Title bar
    draw.rectangle([0, 0, w, 40], fill='#1E293B')
    # Window controls
    draw.ellipse([16, 14, 28, 26], fill='#EF4444')
    draw.ellipse([36, 14, 48, 26], fill='#F59E0B')
    draw.ellipse([56, 14, 68, 26], fill='#10B981')
    draw.text((90, 12), "PDFinity 1.0 Stable — contract_enterprise_2026.pdf", fill='#E2E8F0')
    draw.text((w - 200, 12), "🔒 100% Offline Core", fill='#10B981')

    # 2. Tab Bar
    draw.rectangle([0, 40, w, 75], fill='#0F172A')
    # Active tab
    draw.rounded_rectangle([10, 46, 260, 75], radius=6, fill='#1E293B')
    draw.text((25, 52), "📄 contract_enterprise_2026.pdf  ✕", fill='#F8FAFC')
    # Inactive tab
    draw.rounded_rectangle([270, 46, 480, 75], radius=6, fill='#1E293B')
    draw.text((285, 52), "📄 quarterly_report.pdf  ✕", fill='#94A3B8')

    # 3. Ribbon / Toolbar
    draw.rectangle([0, 75, w, 125], fill='#1E293B')
    draw.line([(0, 125), (w, 125)], fill='#334155', width=1)
    tools = [
        "📂 Open", "💾 Save As", "|", "🔍 Zoom (125%)", "↺ Rotate", "|",
        "✏️ Annotate", "📝 Forms", "✂️ Edit Content", "🛡️ Redact", "🔏 Sign", "|",
        "⚙️ Batch Automation", "🤖 MCP Stdio", "📱 Remote Pair"
    ]
    tx = 20
    for t in tools:
        if t == "|":
            draw.line([(tx + 5, 85), (tx + 5, 115)], fill='#475569', width=1)
            tx += 20
        else:
            draw.rounded_rectangle([tx, 83, tx + len(t)*9 + 16, 117], radius=4, fill='#334155')
            draw.text((tx + 8, 92), t, fill='#F1F5F9')
            tx += len(t)*9 + 26

    # 4. Main Body: Split Sidebar (Left) and Canvas (Right)
    sidebar_w = 260
    draw.rectangle([0, 126, sidebar_w, h - 30], fill='#1E293B')
    draw.line([(sidebar_w, 126), (sidebar_w, h - 30)], fill='#334155', width=1)

    # Sidebar bookmarks
    draw.text((16, 140), "BOOKMARKS & THUMBNAILS", fill='#94A3B8')
    bookmarks = [
        "▶ 1. Master Services Agreement",
        "   • 1.1 Scope of Operations",
        "   • 1.2 Confidentiality & IP",
        "▶ 2. Data Protection & SLA",
        "   • 2.1 Security Invariants",
        "   • 2.2 Air-Gapped Verification",
        "▶ 3. Execution & Digital Signature"
    ]
    by = 170
    for b in bookmarks:
        color = '#38BDF8' if '3.' in b else '#CBD5E1'
        draw.text((16, by), b, fill=color)
        by += 28

    # Thumbnail preview card in sidebar
    draw.rounded_rectangle([25, 420, 235, 680], radius=6, fill='#0F172A', outline='#334155')
    draw.rectangle([40, 440, 220, 660], fill='#FFFFFF')
    draw.text((50, 450), "PAGE 3 (ACTIVE)", fill='#0F172A')
    draw.rectangle([50, 470, 210, 480], fill='#CBD5E1')
    draw.rectangle([50, 490, 180, 500], fill='#E2E8F0')

    # 5. Canvas: Center PDF Document
    canvas_cx = sidebar_w + (w - sidebar_w) // 2
    doc_w = 520
    doc_left = canvas_cx - doc_w // 2
    doc_right = canvas_cx + doc_w // 2
    # Document drop shadow
    draw.rectangle([doc_left + 6, 146, doc_right + 6, h - 44], fill='#000000')
    # Document page surface
    draw.rectangle([doc_left, 140, doc_right, h - 50], fill='#FFFFFF')

    # Document contents
    draw.text((doc_left + 40, 170), "MASTER PROFESSIONAL SERVICES AGREEMENT", fill='#0F172A')
    draw.text((doc_left + 40, 195), "StackSync Solutions — Enterprise Software License", fill='#64748B')
    draw.line([(doc_left + 40, 215), (doc_right - 40, 215)], fill='#CBD5E1', width=2)

    # Paragraph lines
    py = 235
    for _ in range(6):
        draw.rectangle([doc_left + 40, py, doc_right - 40, py + 12], fill='#F1F5F9')
        py += 22

    # Highlighted text region
    draw.rectangle([doc_left + 40, py, doc_right - 120, py + 16], fill='#FEF08A')
    draw.text((doc_left + 45, py), "All PDF processing executes 100% offline with zero cloud telemetry.", fill='#713F12')
    py += 34

    for _ in range(4):
        draw.rectangle([doc_left + 40, py, doc_right - 40, py + 12], fill='#F1F5F9')
        py += 22

    # Signature verification badge in document
    draw.rounded_rectangle([doc_left + 40, py + 10, doc_right - 40, py + 90], radius=6, fill='#F0FDF4', outline='#86EFAC')
    draw.text((doc_left + 60, py + 25), "✓ CRYPTOGRAPHIC DIGITAL SIGNATURE VERIFIED", fill='#166534')
    draw.text((doc_left + 60, py + 48), "Signer: Jake Ashley Panlilio (StackSync Solutions)", fill='#15803D')
    draw.text((doc_left + 60, py + 68), "PAdES / PKCS#7 Valid • SHA-256 Digest Match • Zero Modifications", fill='#166534')

    # 6. Status bar
    draw.rectangle([0, h - 30, w, h], fill='#1E293B')
    draw.text((16, h - 22), "Page 3 of 12  |  125%  |  Fit Width  |  ISO 32000-1 (PDF 1.7)  |  Rendering: Google PDFium", fill='#94A3B8')
    draw.text((w - 320, h - 22), "StackSync Solutions  •  MPL-2.0  •  Zero Telemetry", fill='#10B981')

    out_path = os.path.join(os.path.dirname(__file__), '..', 'docs', 'screenshots', 'main-workspace.png')
    img.save(out_path, format='PNG')
    print(f"[+] Generated: {out_path}")

def create_page_organizer_screenshot():
    w, h = 1280, 800
    img = Image.new('RGB', (w, h), '#0F172A')
    draw = ImageDraw.Draw(img)

    # Window header
    draw.rectangle([0, 0, w, 40], fill='#1E293B')
    draw.text((20, 12), "PDFinity Page Assembly & Organizer Studio", fill='#F8FAFC')
    draw.text((w - 220, 12), "contract_enterprise_2026.pdf", fill='#94A3B8')

    # Dialog toolbar
    draw.rectangle([0, 40, w, 90], fill='#1E293B')
    draw.line([(0, 90), (w, 90)], fill='#334155', width=1)
    actions = ["➕ Add Pages...", "🗑️ Delete Page", "↺ Rotate Left", "↻ Rotate Right", "✂️ Extract Selection", "⚡ Split Document...", "💾 Commit Assembly"]
    ax = 20
    for a in actions:
        fill_color = '#DC2626' if 'Commit' in a else '#334155'
        draw.rounded_rectangle([ax, 48, ax + len(a)*10 + 10, 82], radius=4, fill=fill_color)
        draw.text((ax + 8, 56), a, fill='#F8FAFC')
        ax += len(a)*10 + 20

    # Grid of page cards (2 rows of 4 cards)
    card_w, card_h = 240, 310
    start_x, start_y = 50, 120
    gap_x, gap_y = 60, 40

    pages = [
        ("Page 1 (Cover)", True), ("Page 2 (TOC)", False), ("Page 3 (MSA)", False), ("Page 4 (SLA)", False),
        ("Page 5 (Security)", False), ("Page 6 (Data Flow)", False), ("Page 7 (Signatures)", True), ("Page 8 (Appendix)", False)
    ]

    for idx, (p_title, is_selected) in enumerate(pages):
        row = idx // 4
        col = idx % 4
        x = start_x + col * (card_w + gap_x)
        y = start_y + row * (card_h + gap_y)

        # Card container
        outline_color = '#38BDF8' if is_selected else '#334155'
        draw.rounded_rectangle([x, y, x + card_w, y + card_h], radius=8, fill='#1E293B', outline=outline_color, width=2 if is_selected else 1)

        # Miniature document sheet
        draw.rectangle([x + 20, y + 20, x + card_w - 20, y + card_h - 50], fill='#FFFFFF')
        draw.text((x + 30, y + 30), f"P. {idx + 1}", fill='#0F172A')
        # Content lines
        for ly in range(y + 60, y + card_h - 70, 20):
            draw.rectangle([x + 30, ly, x + card_w - 30, ly + 8], fill='#E2E8F0')

        # Card Footer Label
        draw.text((x + 20, y + card_h - 35), p_title, fill='#38BDF8' if is_selected else '#CBD5E1')
        if is_selected:
            draw.rounded_rectangle([x + card_w - 65, y + card_h - 40, x + card_w - 15, y + card_h - 15], radius=4, fill='#0284C7')
            draw.text((x + card_w - 60, y + card_h - 35), "Selected", fill='#FFFFFF')

    # Status Bar
    draw.rectangle([0, h - 35, w, h], fill='#1E293B')
    draw.text((20, h - 25), "Selected: 2 pages  |  Lossless QPDF Reassembly Engine  |  Atomic Output Verification", fill='#94A3B8')

    out_path = os.path.join(os.path.dirname(__file__), '..', 'docs', 'screenshots', 'page-organizer.png')
    img.save(out_path, format='PNG')
    print(f"[+] Generated: {out_path}")

def create_redaction_signing_screenshot():
    w, h = 1280, 800
    img = Image.new('RGB', (w, h), '#0F172A')
    draw = ImageDraw.Draw(img)

    # Window title
    draw.rectangle([0, 0, w, 40], fill='#1E293B')
    draw.text((20, 12), "PDFinity Security & Cryptographic Redaction Studio", fill='#F8FAFC')
    draw.text((w - 220, 12), "Air-Gapped Privacy Mode", fill='#10B981')

    # Main split layout: Document view on left (780px), Security audit panel on right (500px)
    left_w = 780
    draw.rectangle([0, 40, left_w, h], fill='#0F172A')
    draw.rectangle([left_w, 40, w, h], fill='#1E293B')
    draw.line([(left_w, 40), (left_w, h)], fill='#334155', width=2)

    # Document preview in left view
    doc_x, doc_y = 60, 70
    doc_w, doc_h = 660, 680
    draw.rectangle([doc_x + 6, doc_y + 6, doc_x + doc_w + 6, doc_y + doc_h + 6], fill='#000000')
    draw.rectangle([doc_x, doc_y, doc_x + doc_w, doc_y + doc_h], fill='#FFFFFF')

    draw.text((doc_x + 40, doc_y + 40), "SECTION 4: CONFIDENTIAL AUDIT DATA", fill='#0F172A')
    draw.text((doc_x + 40, doc_y + 65), "Internal Network Topology and Account Identifiers", fill='#64748B')
    draw.line([(doc_x + 40, doc_y + 85), (doc_x + doc_w - 40, doc_y + 85)], fill='#CBD5E1', width=1)

    dy = doc_y + 110
    for _ in range(3):
        draw.rectangle([doc_x + 40, dy, doc_x + doc_w - 40, dy + 12], fill='#F1F5F9')
        dy += 24

    # Redacted boxes (True irreversible excision)
    draw.rectangle([doc_x + 40, dy, doc_x + 360, dy + 28], fill='#000000')
    draw.text((doc_x + 50, dy + 6), "[EXCISED: INTERNAL SERVER IP & PORT]", fill='#FFFFFF')
    dy += 45

    for _ in range(2):
        draw.rectangle([doc_x + 40, dy, doc_x + doc_w - 40, dy + 12], fill='#F1F5F9')
        dy += 24

    draw.rectangle([doc_x + 40, dy, doc_x + 420, dy + 28], fill='#000000')
    draw.text((doc_x + 50, dy + 6), "[EXCISED: PRODUCTION API BEARER TOKEN]", fill='#FFFFFF')
    dy += 60

    # Right side: Security & Redaction Inspector
    draw.text((left_w + 30, 60), "REDACTION & SANITIZATION ENGINE", fill='#38BDF8')
    draw.line([(left_w + 30, 85), (w - 30, 85)], fill='#334155', width=1)

    items = [
        ("• Glyphs Excised:", "248 font characters permanently removed"),
        ("• Content Stream:", "Tj and TJ operators deleted from stream"),
        ("• Vector Overlays:", "Irreversible rasterization applied"),
        ("• Metadata Scrubbed:", "Author, Producer, CreationDate zeroed"),
        ("• Embedded Attachments:", "None detected"),
        ("• Javascript / Actions:", "0 executable streams")
    ]
    ry = 105
    for lbl, val in items:
        draw.text((left_w + 30, ry), lbl, fill='#F8FAFC')
        draw.text((left_w + 200, ry), val, fill='#94A3B8')
        ry += 32

    # Digital Signature Box
    ry += 20
    draw.text((left_w + 30, ry), "PKCS#7 DIGITAL SIGNATURE CARD", fill='#10B981')
    draw.line([(left_w + 30, ry + 25), (w - 30, ry + 25)], fill='#334155', width=1)
    ry += 40

    sig_card = [
        ("Certificate Status:", "✓ Trusted Root X.509"),
        ("Signer CN:", "Jake Ashley Panlilio"),
        ("Organization (O):", "StackSync Solutions"),
        ("Digest Algorithm:", "SHA-256 (256-bit)"),
        ("Signature Standard:", "PAdES-B-B / ISO 32000-1"),
        ("Document Byte Range:", "Complete (No modifications after signing)")
    ]
    for lbl, val in sig_card:
        draw.text((left_w + 30, ry), lbl, fill='#E2E8F0')
        draw.text((left_w + 190, ry), val, fill='#86EFAC' if '✓' in val else '#CBD5E1')
        ry += 30

    # Redaction Execution Button
    draw.rounded_rectangle([left_w + 30, h - 80, w - 30, h - 35], radius=6, fill='#DC2626')
    draw.text((left_w + 110, h - 63), "🛡️ APPLY IRREVERSIBLE REDACTION (ATOMIC SAVE)", fill='#FFFFFF')

    out_path = os.path.join(os.path.dirname(__file__), '..', 'docs', 'screenshots', 'redaction-signing.png')
    img.save(out_path, format='PNG')
    print(f"[+] Generated: {out_path}")

def create_android_screenshot():
    # Mobile phone / tablet portrait aspect ratio
    w, h = 720, 1280
    img = Image.new('RGB', (w, h), '#0F172A')
    draw = ImageDraw.Draw(img)

    # 1. Bezel phone status bar
    draw.rectangle([0, 0, w, 40], fill='#020617')
    draw.text((24, 12), "10:30", fill='#FFFFFF')
    draw.text((w - 160, 12), "5G  100% 🔋", fill='#FFFFFF')

    # 2. Android App Action Bar
    draw.rectangle([0, 40, w, 110], fill='#1E293B')
    draw.text((24, 60), "PDFinity Mobile", fill='#F8FAFC')
    draw.text((w - 240, 60), "🔍  ✏️  ⋮", fill='#CBD5E1')

    # 3. Document Viewport
    doc_x, doc_y = 30, 130
    doc_w, doc_h = w - 60, h - 230
    # Document surface
    draw.rounded_rectangle([doc_x, doc_y, doc_x + doc_w, doc_y + doc_h], radius=8, fill='#FFFFFF')

    # Document contents
    draw.text((doc_x + 30, doc_y + 40), "EXECUTIVE SUMMARY", fill='#0F172A')
    draw.text((doc_x + 30, doc_y + 70), "PDFinity Android Storage Access Framework (SAF)", fill='#64748B')
    draw.line([(doc_x + 30, doc_y + 95), (doc_x + doc_w - 30, doc_y + 95)], fill='#CBD5E1', width=1)

    dy = doc_y + 120
    for _ in range(8):
        draw.rectangle([doc_x + 30, dy, doc_x + doc_w - 30, dy + 16], fill='#F1F5F9')
        dy += 34

    # Highlighted touch annotation
    draw.rectangle([doc_x + 30, dy, doc_x + doc_w - 80, dy + 22], fill='#BAE6FD')
    draw.text((doc_x + 35, dy + 2), "Pinch-to-zoom and stylus pen annotation active", fill='#0369A1')
    dy += 50

    for _ in range(6):
        draw.rectangle([doc_x + 30, dy, doc_x + doc_w - 30, dy + 16], fill='#F1F5F9')
        dy += 34

    # 4. Floating Action Button (FAB)
    fab_cx, fab_cy, fab_r = w - 90, h - 180, 40
    draw.ellipse([fab_cx - fab_r, fab_cy - fab_r, fab_cx + fab_r, fab_cy + fab_r], fill='#DC2626')
    draw.text((fab_cx - 12, fab_cy - 16), "✎", fill='#FFFFFF')

    # 5. Mobile Navigation Bottom Bar
    draw.rectangle([0, h - 80, w, h], fill='#1E293B')
    draw.line([(0, h - 80), (w, h - 80)], fill='#334155', width=1)
    navs = ["📄 Pages", "🔖 Outline", "🛡️ Redact", "⚙️ Settings"]
    nx = 40
    for n in navs:
        draw.text((nx, h - 50), n, fill='#38BDF8' if 'Pages' in n else '#94A3B8')
        nx += 170

    out_path = os.path.join(os.path.dirname(__file__), '..', 'docs', 'screenshots', 'android-view.png')
    img.save(out_path, format='PNG')
    print(f"[+] Generated: {out_path}")

def main():
    print("Generating official visual showcase assets...")
    create_app_icon()
    create_desktop_workspace_screenshot()
    create_page_organizer_screenshot()
    create_redaction_signing_screenshot()
    create_android_screenshot()
    print("All visual assets successfully generated!")

if __name__ == '__main__':
    main()
