# PDFinity: Third-Party Licenses and Intellectual Property Notices

**Organization / Publisher:** [StackSync Solutions](https://www.stacksyncsolutions.com/)  
**Lead Programmer:** [Jake Ashley Panlilio](https://github.com/jakepanlilio2000)  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  

---

## 1. Intellectual Property & Licensing Philosophy

PDFinity is an open-source, modular, offline-first PDF productivity suite released under the **Mozilla Public License Version 2.0 (MPL-2.0)**. The project enforces strict intellectual property governance:

1. **Weak Copyleft at File Boundary (MPL-2.0):** Core application source files are governed by MPL-2.0. Modifications to existing PDFinity source files remain open under MPL-2.0, while larger works combining PDFinity with independent modules or external proprietary plugins retain their independent licensing.
2. **Dynamic Linking Compliance (LGPLv3):** Where LGPLv3 dependencies are utilized (specifically the Qt 6 framework), the architecture strictly enforces dynamic linking and relocatable libraries to satisfy all LGPLv3 Section 4 requirements.
3. **Permissive Third-Party Foundation:** All external dependencies are governed by permissive licenses (Apache 2.0, BSD-3-Clause, MIT, and Public Domain).
4. **Strict Copyleft Prohibition:** Libraries governed by **GPLv2, GPLv3, or AGPLv3** (such as Poppler, Ghostscript, or MuPDF) are strictly excluded from the application to prevent copyleft viral contamination.

---

## 2. Dependency License Inventory & MPL-2.0 Compatibility

| Component / Library | SPDX Identifier | License Model | Linking Method | MPL-2.0 Compatibility Analysis |
| :--- | :--- | :--- | :--- | :--- |
| **PDFinity Core & UI** | `MPL-2.0` | Weak Copyleft (File-level) | Executable / Static Libs | Primary project license; file-level modifications must remain under MPL-2.0. |
| **Qt 6 Framework** | `LGPL-3.0-only` | Weak Copyleft (Library-level) | **Dynamic Linking Only** (`.dll`, `.so`, `.dylib`) | Compatible as a Combined Work under LGPLv3 Sec. 4 and MPL-2.0 Sec. 3.3 Larger Works. |
| **Google PDFium** | `Apache-2.0` / `BSD-3-Clause` | Permissive | Dynamic / Import Lib | Permissive license fully compatible with MPL-2.0. Copyright and Apache notices preserved. |
| **QPDF** | `Apache-2.0` | Permissive | Dynamic / Static Lib | Permissive Apache-2.0 terms compatible with MPL-2.0. Original copyright notices preserved. |
| **OpenSSL** | `Apache-2.0` | Permissive | Dynamic / OS-Native | Permissive Apache-2.0 license compatible with MPL-2.0. Attribution statements retained. |
| **SQLite** | `blessing` | Public Domain | Static Source | Public domain dedication allows unencumbered incorporation under MPL-2.0. |
| **nlohmann/json** | `MIT` | Permissive | Header-only | Permissive MIT terms permit inclusion in MPL-2.0 source files with notice retention. |
| **zlib** | `Zlib` | Permissive | Static / Dynamic Lib | Permissive zlib terms compatible with MPL-2.0 without copyleft conflict. |
| **Tesseract OCR / Leptonica** | `Apache-2.0` / `BSD-2-Clause` | Permissive | Subprocess Tool / Dynamic | Permissive subprocess and dynamic invocation compatible with MPL-2.0. |

---

## 3. Dynamic Linking Compliance for Qt 6 (LGPLv3)

The Qt 6 Framework is licensed under the GNU Lesser General Public License version 3 (`LGPL-3.0`). To ensure complete and verifiable compliance with LGPLv3 Section 4 ("Combined Works"), PDFinity implements the following architectural controls:

### 3.1 Dynamic Linking Architecture
- All Qt components (`Qt6Core`, `Qt6Gui`, `Qt6Widgets`, `Qt6PrintSupport`, `Qt6Network`) are linked **exclusively as shared dynamic link libraries** (`.dll` on Windows, `.so` on Linux, `.dylib` on macOS).
- **No static linking of Qt is permitted** in any build preset or packaging pipeline.
- Users may replace the Qt dynamic link libraries in their installation directory with compatible custom builds without requiring recompilation of PDFinity.

### 3.2 Attribution and Source Code Access
- Binary distributions bundle `NOTICE` and `THIRD_PARTY_LICENSES.md`.
- The desktop interface includes an accessible **Help -> About -> Licenses** dialog displaying all third-party licenses, copyright holders, and an explicit hyperlink to the official Qt source code repository ([https://code.qt.io](https://code.qt.io)).

---

## 4. Full Attributions & Third-Party License Texts

### 4.1 Google PDFium
```text
Copyright 2014-2026 The PDFium Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

### 4.2 QPDF
```text
Copyright (c) 2005-2026 Jay Berkenbilt and contributors.
Licensed under the Apache License, Version 2.0.
Upstream repository: https://github.com/qpdf/qpdf
```

### 4.3 OpenSSL
```text
Copyright 1998-2026 The OpenSSL Project Authors. All rights reserved.
Licensed under the Apache License, Version 2.0.
Upstream repository: https://www.openssl.org
```

### 4.4 SQLite Blessing
```text
2001-2026 D. Richard Hipp.
The author disclaims copyright to this source code. In place of a legal notice,
here is a blessing:
   May you do good and not evil.
   May you find forgiveness for yourself and forgive others.
   May you share freely, never taking more than you give.
```

### 4.5 nlohmann/json
```text
MIT License
Copyright (c) 2013-2026 Niels Lohmann
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction.
```

### 4.6 zlib
```text
Copyright (C) 1995-2026 Jean-loup Gailly and Mark Adler.
This software is provided 'as-is', without any express or implied warranty.
```
