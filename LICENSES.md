# PDFinity: Licensing Governance and Intellectual Property Architecture

**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Status:** Official Open-Source Release Specification  

---

## 1. Licensing Philosophy and Objectives

PDFinity is an open-source, modular, offline-first PDF productivity suite released under the **Mozilla Public License Version 2.0 (MPL-2.0)**. The project enforces strict IP governance to ensure that PDFinity can be used, modified, and integrated into enterprise and developer workflows cleanly and without legal ambiguity:

1. **Weak Copyleft at File Boundary (MPL-2.0):** Core application source files are governed by MPL-2.0. Modifications to existing PDFinity source files remain open under MPL-2.0, while larger works combining PDFinity with independent modules or external proprietary plugins retain their independent licensing.
2. **Dynamic Linking Compliance (LGPLv3):** Where LGPLv3 dependencies are utilized (specifically the Qt 6 framework), the architecture strictly enforces dynamic linking and relocatable libraries to satisfy all LGPLv3 Section 4 requirements.
3. **Permissive Third-Party Foundation:** All external dependencies are governed by permissive licenses (Apache 2.0, BSD-3-Clause, MIT, and Public Domain).
4. **Strict Copyleft Prohibition:** Libraries governed by **GPLv2, GPLv3, or AGPLv3** (such as Poppler, Ghostscript, or MuPDF) are strictly excluded from the application to prevent copyleft viral contamination.

---

## 2. License Inventory of Project Components

| Component / Library | SPDX Identifier | License Model | Linking Method | Compliance Requirements |
| :--- | :--- | :--- | :--- | :--- |
| **PDFinity Core & UI** | `MPL-2.0` | Weak Copyleft (File-level) | Executable / Static Libs | Source code modifications released under MPL-2.0; copyright notices retained. |
| **Qt 6 Framework** | `LGPL-3.0-only` | Weak Copyleft (Library-level) | **Dynamic Linking Only** (`.dll`, `.so`, `.dylib`) | Permit user replacement of dynamic Qt libraries; provide license texts and link to Qt source. |
| **Google PDFium** | `Apache-2.0` / `BSD-3-Clause` | Permissive | Dynamic / Import Lib | Include Apache 2.0 and BSD notices; retain NOTICE file. |
| **QPDF** | `Apache-2.0` | Permissive | Dynamic / Static Lib | Include Apache 2.0 license notice and original QPDF copyright attributions. |
| **OpenSSL** | `Apache-2.0` | Permissive | Dynamic / OS-Native | Include OpenSSL Apache 2.0 notice and attribution statement. |
| **SQLite** | `blessing` | Public Domain | Static Source | Retain standard SQLite blessing notice. |
| **nlohmann/json** | `MIT` | Permissive | Header-only | Retain MIT copyright notice in source headers and notices. |
| **zlib** | `Zlib` | Permissive | Static / Dynamic Lib | Retain zlib copyright notice and disclaimer. |
| **Tesseract OCR / Leptonica** | `Apache-2.0` / `BSD-2-Clause` | Permissive | Subprocess Tool / Dynamic | Retain Apache 2.0 and Leptonica notices. |

---

## 3. Compliance Architecture for LGPLv3 (Qt 6)

The Qt 6 Framework is licensed under the GNU Lesser General Public License version 3 (`LGPL-3.0`). To ensure complete and verifiable compliance with LGPLv3 Section 4 ("Combined Works"), PDFinity implements the following architectural controls:

### 3.1 Dynamic Linking Architecture
- All Qt components (`Qt6Core`, `Qt6Gui`, `Qt6Widgets`, `Qt6PrintSupport`, `Qt6Network`) are linked **exclusively as shared dynamic link libraries** (`.dll` on Windows, `.so` on Linux, `.dylib` on macOS).
- **No static linking of Qt is permitted** in any build preset or packaging pipeline.
- Users may replace the Qt dynamic link libraries in their installation directory with compatible custom builds without requiring recompilation of PDFinity.

### 3.2 Attribution and Source Code Access
- Binary distributions bundle `THIRDPARTY_NOTICES.txt` containing full license texts.
- The desktop interface includes an accessible **Help -> About -> Licenses** dialog displaying all third-party licenses, copyright holders, and an explicit hyperlink to the official Qt source code repository ([https://code.qt.io](https://code.qt.io)).

---

## 4. Evaluation and Rejection of Incompatible Engines

| Incompatible Library | License | Reason for Exclusion |
| :--- | :--- | :--- |
| **Poppler** | **GPLv2 / GPLv3** | Viral copyleft obligates all combining code to adopt GPLv3, preventing modular MPL-2.0 enterprise adoption. |
| **MuPDF** | **AGPLv3** / Commercial | Network copyleft requires source disclosure across network interactions; commercial license requires per-seat royalties. |
| **Ghostscript** | **AGPLv3** / Commercial | Historical memory-safety attack surface and AGPLv3 copyleft contamination. |
| **iText (Modern)** | **AGPLv3** / Commercial | AGPLv3 license taint; Java runtime overhead. |

---

## 5. Distribution Verification Checklist

Every production installer, archive, and package must include:
- [x] `LICENSE` (Official MPL-2.0 license text with StackSync Solutions copyright)
- [x] `THIRDPARTY_NOTICES.txt` (Complete upstream license texts and notices)
- [x] `docs/SBOM.md` (Software Bill of Materials)
- [x] Dynamically linked Qt 6 libraries with intact version metadata
