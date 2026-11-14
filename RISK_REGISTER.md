# PDFinity: Comprehensive Risk Register

**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Status:** Production Risk Governance Baseline  

---

## 1. Risk Matrix and Severity Definitions

- **Critical:** Potential for arbitrary code execution, catastrophic document corruption, private key exposure, or unauthenticated remote takeover. Blocks release immediately.
- **High:** Major security weakness, core feature corruption, irreversible data loss, or license violation. Must be mitigated prior to production release.
- **Medium:** Significant UX defect, recoverable document error, partial platform degradation, or missing non-critical validation evidence.
- **Low:** Minor visual glitch, cosmetic string formatting, or non-blocking documentation discrepancy.

---

## 2. Active Risk Register

| Risk ID | Category | Description | Likelihood | Impact | Severity | Detection Mechanism | Mitigation Strategy | Residual Risk |
| :--- | :--- | :--- | :---: | :---: | :---: | :--- | :--- | :---: |
| **RSK-001** | **Security** | **Parsing Untrusted PDFs:** Maliciously crafted PDF streams, circular object references, or corrupted font tables exploit memory corruption vulnerabilities in parsing engines. | High | Critical | **Critical** | Automated fuzz testing (AFL++/libFuzzer), ASan/UBSan sanitizers, regression suite of known exploit PDFs. | 1. Use Google PDFium (heavily fuzz-tested in Chromium).<br>2. Disable embedded PDF JavaScript engine entirely.<br>3. Never execute PDF Launch actions.<br>4. Run parser within bounded memory limits. | **Low** |
| **RSK-002** | **Security / Compliance** | **Incomplete Redaction (Visual Masking Fallacy):** Drawing a black rectangle over text leaves the underlying glyphs, text stream, vector objects, and XMP metadata extractable by search tools. | High | Critical | **Critical** | Automated text extraction audit on redacted output; byte-level object tree inspection test. | **Multi-Pass Irreversible Redaction:**<br>1. Rasterize masked region to flat pixels.<br>2. Structurally remove underlying text operators (`Tj`, `TJ`), font glyphs, and vector paths from content streams.<br>3. Purge matching strings from metadata, annotations, bookmarks, and form fields.<br>4. Rebuild xref table and sanitize unreferenced objects via QPDF. | **Very Low** |
| **RSK-003** | **Data Integrity** | **Source File Corruption on Save:** Power loss, process crash, or disk exhaustion during direct in-place save corrupts the user's primary document. | Medium | Critical | **Critical** | File I/O fault-injection tests (simulated disk full, sudden thread termination). | **Atomic Replacement Protocol:**<br>1. Never overwrite source files in-place.<br>2. Always serialize to a secure temporary file (`.tmp`).<br>3. Verify new document header, xref, and trailer integrity before touch.<br>4. Execute atomic file rename (`MoveFileEx` with `MOVEFILE_REPLACE_EXISTING` on Windows, `rename()` on POSIX).<br>5. Preserve backup copy until write is confirmed. | **Very Low** |
| **RSK-004** | **Platform** | **Windows 7 Legacy Drift Crippling Modern Build:** Attempting to force modern Qt 6 and C++20 code into Windows 7 SP1 breaks the modern build or forces compromises on modern OS capabilities. | High | High | **High** | CI cross-compilation checks and explicit platform API usage audits. | **Two-Profile Isolation:**<br>1. Primary modern builds use C++20 and Qt 6.8+ targeting Windows 10/11, Linux, and macOS.<br>2. Legacy Windows 7 profile is maintained as an isolated branch/preset with Qt 5.15 and MSVC `v141_xp`.<br>3. No claims of Windows 7 support until an authentic Windows 7 environment passes physical/VM automated test suites. | **Low** |
| **RSK-005** | **Security** | **MCP / Remote Controller Attack Surface:** Enabling external tool integration or remote companion control creates avenues for unauthorized filesystem browsing, arbitrary command execution, or path traversal. | High | Critical | **Critical** | Network security audit, fuzz testing tool inputs, path traversal regression test suite. | 1. **Disabled by default:** Both features require deliberate user activation.<br>2. **Local Stdio first:** MCP operates primarily via standard I/O; network binds strictly to localhost.<br>3. **Path Canonicalization:** Reject any path containing `..`, symlink escapes, or targeting outside user-approved root directories.<br>4. **Explicit Consent:** Destructive actions (save, overwrite, delete, redact, sign) mandate UI confirmation dialogs.<br>5. **No Shell Execution:** MCP/Remote schemas do not expose shell or system execution capabilities. | **Low** |
| **RSK-006** | **Security** | **Cryptographic Signing Misrepresentation:** Displaying a visual signature graphic and claiming the document is digitally signed without valid PKCS#7 / CMS cryptographic byte-range hashes. | Medium | High | **High** | Cryptographic validation suite using OpenSSL/WinCNG CLI to independently verify byte ranges and certificate chains. | 1. Clear UI distinction between "Visual Stamp" and "Cryptographic Digital Signature".<br>2. Implement standard Adobe.PPKLite / adbe.pkcs7.detached cryptographic signing with valid `/ByteRange`.<br>3. Validate certificate trust chain, expiry, and alteration detection.<br>4. Never report "Valid Signature" if byte-range hash does not match or document was modified after signing. | **Very Low** |
| **RSK-007** | **Performance** | **UI Freeze on Gigabyte-scale Documents:** Loading large architectural plans (500 MB+) or multi-thousand-page documents locks the main Qt GUI thread. | High | Medium | **Medium** | Automated responsiveness profiler; UI frame drop detection during stress load. | 1. All document parsing, rendering, and indexing offloaded to background threads (`QtConcurrent` / `JobManager`).<br>2. Virtualized viewport rendering: only visible pages at current zoom level are rasterized.<br>3. LRU bitmap memory cache with configurable ceiling (e.g., max 200 MB bitmap cache).<br>4. Cancellation tokens on all rendering requests. | **Low** |
| **RSK-008** | **Security** | **Plaintext Secret Leakage:** Storing digital certificate passwords, private keys, or encryption passphrases in application settings, crash dumps, or memory swap. | Medium | Critical | **Critical** | Memory dump scanner; static analysis of settings serialization files. | 1. Never serialize passwords or private keys to disk or configuration files.<br>2. Use secure zeroing memory wrappers (`SecureBuffer` / `SecureString` using `SecureZeroMemory` / `explicit_bzero`).<br>3. Protect memory using OS-level DPAPI / `VirtualLock` where supported.<br>4. Strip sensitive parameters from log output. | **Very Low** |
| **RSK-009** | **Compliance** | **False Standards Claims (PDF/A & Accessibility):** Claiming a document is PDF/A compliant or fully accessible without validating PDF/A color profiles, font embeddings, or structure tree tags. | Medium | Medium | **Medium** | Standards validation engine (preflight check) against ISO 19005 (PDF/A) and ISO 14289 (PDF/UA). | Explicitly disclose preflight validation results; never emit a PDF/A conformity marker without verifying font embedding and color output intent dictionaries. | **Low** |

---

## 3. Risk Governance and Review Protocol

- The Risk Register is a living document reviewed during quality assurance audits and before every release.
- Any new dependency or architectural refactor must include an explicit risk assessment entry prior to merging.
- No release may proceed if an open **Critical** or unmitigated **High** risk remains unresolved.
