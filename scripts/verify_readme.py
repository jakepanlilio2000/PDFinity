import re
import os
import sys

readme_path = 'README.md'
with open(readme_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Match markdown links [text](path)
md_links = re.findall(r'\[(?:[^\]]*)\]\(([^)#\s]+)(?:#[^\)]*)?\)', content)
# Match HTML <img src="path">
html_imgs = re.findall(r'<img[^>]+src=[\'"]([^\'"]+)[\'"]', content)

all_paths = set(md_links + html_imgs)
print(f"Checking {len(all_paths)} referenced paths...")

missing = []
valid = []
for p in sorted(all_paths):
    if p.startswith('http://') or p.startswith('https://') or p.startswith('mailto:'):
        continue
    norm_p = os.path.normpath(p)
    if not os.path.exists(norm_p):
        missing.append((p, norm_p))
    else:
        size = os.path.getsize(norm_p)
        valid.append((p, norm_p, size))
        print(f"  [OK] {p} -> {norm_p} ({size} bytes)")

print(f"\nTotal local references checked: {len(valid) + len(missing)}")
if missing:
    print("ERROR: Missing paths:")
    for orig, norm in missing:
        print(f"  MISSING: {orig} -> {norm}")
    sys.exit(1)
else:
    print("SUCCESS: 100% of local links and images in README.md exist and are non-empty!")
