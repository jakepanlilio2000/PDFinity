#!/usr/bin/env python3
"""
PDFinity Studio SVG Icon Generator
Generates clean, consistent 24x24 vector SVG icons with 1.6px stroke,
rounded caps/joins, and crimson accent highlights.
"""

import os

ICONS = {
    "home": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M3 10.5L12 3l9 7.5V20a1.5 1.5 0 0 1-1.5 1.5H4.5A1.5 1.5 0 0 1 3 20v-9.5z"/>
  <path d="M9 21v-6a1 1 0 0 1 1-1h4a1 1 0 0 1 1 1v6" stroke="#DC2626"/>
</svg>""",

    "open": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M3 6.5A1.5 1.5 0 0 1 4.5 5H9l2 2h8.5A1.5 1.5 0 0 1 21 8.5V10H3V6.5z"/>
  <path d="M2.5 11h19l-2 9H4.5l-2-9z" stroke="#DC2626"/>
</svg>""",

    "save": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/>
  <polyline points="17 21 17 13 7 13 7 21"/>
  <polyline points="7 3 7 8 15 8" stroke="#DC2626"/>
</svg>""",

    "save-as": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M15 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v5"/>
  <polyline points="17 21 17 14 7 14 7 21"/>
  <circle cx="18" cy="18" r="3" stroke="#DC2626"/>
  <path d="M18 16.5v3M16.5 18h3" stroke="#DC2626"/>
</svg>""",

    "print": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <polyline points="6 9 6 3 18 3 18 9"/>
  <path d="M6 18H4a2 2 0 0 1-2-2v-5a2 2 0 0 1 2-2h16a2 2 0 0 1 2 2v5a2 2 0 0 1-2 2h-2"/>
  <rect x="6" y="14" width="12" height="7" rx="1" stroke="#DC2626"/>
</svg>""",

    "undo": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M3 9h11a5 5 0 0 1 5 5v1a5 5 0 0 1-5 5H7"/>
  <polyline points="7 5 3 9 7 13" stroke="#DC2626"/>
</svg>""",

    "redo": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M21 9H10a5 5 0 0 0-5 5v1a5 5 0 0 0 5 5h7"/>
  <polyline points="17 5 21 9 17 13" stroke="#DC2626"/>
</svg>""",

    "search": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <circle cx="11" cy="11" r="7.5"/>
  <line x1="21" y1="21" x2="16.5" y2="16.5" stroke="#DC2626"/>
</svg>""",

    "pan": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M18 11V6a1.5 1.5 0 0 0-3 0v4M15 10V4.5a1.5 1.5 0 0 0-3 0v6M12 9.5V3a1.5 1.5 0 0 0-3 0v8M9 9.5a1.5 1.5 0 0 0-3 0v6c0 4 3 6.5 7 6.5s7-2.5 7-6.5V9.5a1.5 1.5 0 0 0-3 0"/>
</svg>""",

    "select": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M5 3l12 12-5.5.5-2.5 5.5-2.5-1.5 2.5-5.5L5 13V3z" stroke="#DC2626"/>
</svg>""",

    "zoom-in": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <circle cx="11" cy="11" r="7"/>
  <line x1="21" y1="21" x2="16" y2="16"/>
  <line x1="11" y1="8" x2="11" y2="14" stroke="#DC2626"/>
  <line x1="8" y1="11" x2="14" y2="11" stroke="#DC2626"/>
</svg>""",

    "zoom-out": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <circle cx="11" cy="11" r="7"/>
  <line x1="21" y1="21" x2="16" y2="16"/>
  <line x1="8" y1="11" x2="14" y2="11" stroke="#DC2626"/>
</svg>""",

    "fit-width": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <rect x="5" y="4" width="14" height="16" rx="1.5"/>
  <path d="M2 12h3M19 12h3M4 10l-2 2 2 2M20 10l2 2-2 2" stroke="#DC2626"/>
</svg>""",

    "fit-page": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <rect x="6" y="3" width="12" height="18" rx="1.5"/>
  <path d="M9 7h6M9 11h6M9 15h4" stroke="#DC2626"/>
</svg>""",

    "spread": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <rect x="2" y="4" width="9" height="16" rx="1"/>
  <rect x="13" y="4" width="9" height="16" rx="1" stroke="#DC2626"/>
</svg>""",

    "thumbnails": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <rect x="3" y="3" width="7" height="8" rx="1"/>
  <rect x="14" y="3" width="7" height="8" rx="1"/>
  <rect x="3" y="14" width="7" height="8" rx="1"/>
  <rect x="14" y="14" width="7" height="8" rx="1" stroke="#DC2626"/>
</svg>""",

    "bookmarks": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M5 4a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2v18l-7-4-7 4V4z" stroke="#DC2626"/>
</svg>""",

    "attachments": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M21.44 11.05l-9.19 9.19a6 6 0 0 1-8.49-8.49l9.19-9.19a4 4 0 0 1 5.66 5.66l-9.2 9.19a2 2 0 0 1-2.83-2.83l8.49-8.48"/>
</svg>""",

    "layers": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <polygon points="12 2 2 7 12 12 22 7 12 2"/>
  <polyline points="2 12 12 17 22 12" stroke="#DC2626"/>
  <polyline points="2 17 12 22 22 17"/>
</svg>""",

    "comments": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M21 11.5a8.38 8.38 0 0 1-.9 3.8 8.5 8.5 0 0 1-7.6 4.7 8.38 8.38 0 0 1-3.8-.9L3 21l1.9-5.7a8.38 8.38 0 0 1-.9-3.8 8.5 8.5 0 0 1 4.7-7.6 8.38 8.38 0 0 1 3.8-.9h.5a8.48 8.48 0 0 1 8 8v.5z" stroke="#DC2626"/>
</svg>""",

    "signatures": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M3 20h18"/>
  <path d="M4 16c2-4 4-2 6-4s4-1 6-4 2-1 4-2" stroke="#DC2626"/>
  <circle cx="19" cy="6" r="2" stroke="#DC2626"/>
</svg>""",

    "preflight": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <circle cx="12" cy="12" r="9"/>
  <polyline points="9 12 11.5 14.5 16 9.5" stroke="#DC2626"/>
</svg>""",

    "highlight": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M12 3l7 7-9 9-5-1 1-5 6-10z"/>
  <path d="M4 21h16" stroke="#DC2626"/>
</svg>""",

    "underline": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M6 4v7a6 6 0 0 0 12 0V4"/>
  <line x1="4" y1="20" x2="20" y2="20" stroke="#DC2626"/>
</svg>""",

    "strikeout": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M17.5 7.5A4.5 4.5 0 0 0 12 4a4.5 4.5 0 0 0-4.5 4.5c0 3 4.5 3.5 4.5 6s-2 3-4.5 2"/>
  <line x1="3" y1="12" x2="21" y2="12" stroke="#DC2626"/>
</svg>""",

    "sticky-note": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M15 3H5a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2V9l-6-6z"/>
  <polyline points="14 3 14 9 20 9" stroke="#DC2626"/>
  <line x1="7" y1="13" x2="13" y2="13"/>
  <line x1="7" y1="17" x2="15" y2="17"/>
</svg>""",

    "draw": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L7.5 19.5 3 21l1.5-4.5L18.5 2.5z" stroke="#DC2626"/>
</svg>""",

    "shapes": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <rect x="3" y="11" width="9" height="9" rx="1"/>
  <circle cx="16.5" cy="7.5" r="4.5" stroke="#DC2626"/>
</svg>""",

    "callout": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"/>
  <line x1="8" y1="9" x2="16" y2="9" stroke="#DC2626"/>
  <line x1="8" y1="13" x2="13" y2="13"/>
</svg>""",

    "stamp": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M7 17h10l1 3H6l1-3z"/>
  <path d="M12 3a3 3 0 0 0-3 3c0 2 2 3.5 2 5v3h2v-3c0-1.5 2-3 2-5a3 3 0 0 0-3-3z"/>
  <line x1="4" y1="21" x2="20" y2="21" stroke="#DC2626"/>
</svg>""",

    "edit": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7"/>
  <path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z" stroke="#DC2626"/>
</svg>""",

    "organize": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <rect x="2" y="3" width="8" height="10" rx="1"/>
  <rect x="14" y="3" width="8" height="10" rx="1"/>
  <rect x="8" y="11" width="8" height="10" rx="1" stroke="#DC2626"/>
</svg>""",

    "merge": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M4 4v6h6M20 4v6h-6"/>
  <path d="M4 10l7 7M20 10l-7 7"/>
  <path d="M12 17v4" stroke="#DC2626"/>
</svg>""",

    "split": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M12 4v16"/>
  <path d="M7 8l-4 4 4 4M17 8l4 4-4 4" stroke="#DC2626"/>
</svg>""",

    "crop": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M6 2v14a2 2 0 0 0 2 2h14"/>
  <path d="M18 22V8a2 2 0 0 0-2-2H2" stroke="#DC2626"/>
</svg>""",

    "rotate": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M21 12a9 9 0 1 1-3-6.7L21 8"/>
  <polyline points="21 3 21 8 16 8" stroke="#DC2626"/>
</svg>""",

    "fill-sign": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <rect x="3" y="3" width="18" height="18" rx="2"/>
  <path d="M7 8h10M7 12h5" stroke="currentColor"/>
  <path d="M14 16c1-2 2-1 3-2s2 0 3-1" stroke="#DC2626"/>
</svg>""",

    "certify": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <circle cx="12" cy="9" r="6" stroke="#DC2626"/>
  <path d="M9 14.5L7 22l5-3 5 3-2-7.5"/>
</svg>""",

    "protect": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <rect x="4" y="11" width="16" height="10" rx="2"/>
  <path d="M7 11V7a5 5 0 0 1 10 0v4" stroke="#DC2626"/>
  <circle cx="12" cy="16" r="1.5"/>
</svg>""",

    "redact": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <rect x="3" y="4" width="18" height="16" rx="2"/>
  <rect x="6" y="9" width="12" height="6" fill="#DC2626" stroke="#DC2626"/>
</svg>""",

    "ocr": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M4 7V4h3M17 4h3v3M4 17v3h3M17 20h3v-3" stroke="#DC2626"/>
  <line x1="8" y1="9" x2="16" y2="9"/>
  <line x1="12" y1="9" x2="12" y2="15"/>
</svg>""",

    "compress": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <path d="M4 8h16M4 16h16"/>
  <path d="M12 4v4M12 16v4" stroke="#DC2626"/>
  <polyline points="9 6 12 8 15 6" stroke="#DC2626"/>
  <polyline points="9 18 12 16 15 18" stroke="#DC2626"/>
</svg>""",

    "batch": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <rect x="3" y="3" width="14" height="14" rx="2"/>
  <path d="M7 17v2a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V9a2 2 0 0 0-2-2h-2" stroke="#DC2626"/>
</svg>"""
}

def main():
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    out_dir = os.path.join(root, "assets", "icons", "studio")
    os.makedirs(out_dir, exist_ok=True)
    
    print(f"Generating {len(ICONS)} studio SVG vector icons in {out_dir}...")
    for name, svg in ICONS.items():
        path = os.path.join(out_dir, f"{name}.svg")
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(svg.strip() + "\n")
        print(f"  [+] {name}.svg")

    print("\nAll SVG icons successfully generated!")

if __name__ == "__main__":
    main()
