# PDFinity: Threat Model and Security Architecture

**Document Version:** 1.0.0  
**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Status:** Official Security Threat Model  

---

## 1. Security Philosophy and Boundaries

PDF documents are inherently complex, binary-rich data containers capable of embedding executable scripts, binary streams, vector graphics, font programs, interactive forms, and nested multimedia attachments. In PDFinity, **all external documents, fonts, images, metadata, and IPC inputs are treated as strictly untrusted**.

PDFinity enforces an offline-first, defense-in-depth security model:
1. **Zero Trust for PDF Streams:** Every byte sequence received from a PDF file is parsed with defensive bounds checking.
2. **Zero Unauthorized Execution:** PDF JavaScript execution is disabled by default; automatic launching of embedded attachments is strictly forbidden.
3. **No Covert Channels:** Core document processing has zero telemetry, zero analytics, zero external network calls, and zero cloud dependencies.
4. **Isolated Integrations:** Optional integrations (MCP, Remote Controller) operate under explicit least-privilege policies with local-only bindings and mandatory user consent.

---

## 2. System Architecture and Trust Boundaries

```
[ UNTRUSTED INPUTS ]
  - External PDF Files (.pdf)
  - Embedded Fonts, Images, Attachments
  - MCP Client Commands (stdio / local socket)
  - Remote Controller Commands (LAN paired)
          |
==========|=========================================================== [ TRUST BOUNDARY 1: Input Validation ]
          v
+--------------------------------------------------------------------+
|                        Validation Subsystem                        |
|  - PathSanitizer (Canonicalization, traversal prevention)           |
|  - StreamValidator (Magic bytes, size bounds, depth checks)        |
|  - MCP/Remote Schema Validator (Strict JSON schema enforcement)    |
+--------------------------------------------------------------------+
          |
==========|=========================================================== [ TRUST BOUNDARY 2: Core Domain Isolation ]
          v
+--------------------------------------------------------------------+
|                         Core Application                           |
|  - PdfEngineInterface (Abstracted PDFium / QPDF execution)         |
|  - DocumentModel (In-memory page and object graph)                 |
|  - Command Processor & Undo/Redo Engine                            |
|  - SecureBuffer & CryptoAdapter (WinCNG / OpenSSL)                 |
+--------------------------------------------------------------------+
          |
==========|=========================================================== [ TRUST BOUNDARY 3: Storage & Presentation ]
          v
+--------------------------------------------------------------------+
|                       Output & Persistence                         |
|  - Atomic File Replacer (Write to secure temp, verify, replace)    |
|  - Redacted Document Serializer (Zero leaked streams)              |
|  - Local SQLite Store (Encrypted settings, audit log)              |
|  - Native OS Desktop Viewport (Per-Monitor DPI Qt6 Shell)          |
+--------------------------------------------------------------------+
```

---

## 3. STRIDE Threat Analysis

| Threat Category (STRIDE) | Threat Description | Attack Vector / Scenario | Primary Mitigation & Engineering Invariant | Residual Risk |
| :--- | :--- | :--- | :--- | :---: |
| **Spoofing (S)** | **Signature Forgery:** Attacker crafts a visual signature stamp or attaches an unverified X.509 certificate claiming document integrity. | Malicious PDF displays "Approved by CEO" stamp with forged or missing cryptographic hash. | 1. Strictly separate visual stamps from cryptographic signatures.<br>2. Perform cryptographic verification of `/ByteRange` digests against embedded public key certificates.<br>3. Visually flag untrusted or self-signed roots with explicit warning badges. | **Very Low** |
| **Spoofing (S)** | **Remote Controller Impersonation:** Rogue device on local LAN connects to PDFinity companion interface. | Attacker scans local network, connects to open port, and issues remote commands. | 1. Disabled by default.<br>2. Short-lived one-time code or QR code pairing handshake.<br>3. Mutual challenge-response authentication with rotating session tokens stored in secure memory.<br>4. Persistent UI banner showing connected device identity. | **Very Low** |
| **Tampering (T)** | **Document Stream Alteration:** Modification of signed document contents through post-signing incremental updates. | Attacker appends new revision changing contract terms while preserving original signature dictionary. | 1. Implement strict incremental update analysis.<br>2. Detect any modifications outside approved form field / signature fields.<br>3. Alert user: "Document has been altered since signature was applied". | **Low** |
| **Tampering (T)** | **Path Traversal File Overwrite:** Attacker sends malformed file paths via CLI or MCP to overwrite critical system files. | Path input: `../../../../Windows/System32/calc.exe` or symlink pointing to sensitive targets. | 1. `PathSanitizer::Canonicalize()` resolves all symlinks and relative tokens.<br>2. Verify path resides strictly within user-approved working directory.<br>3. Prohibit path separators and null bytes in extracted file names. | **Zero** |
| **Repudiation (R)** | **Unlogged Destructive Operations:** User or automation denies having performed redaction, permanent deletion, or document signing. | Operator claims system destroyed pages without authorization. | 1. Local append-only audit log stored in SQLite with cryptographic tamper checks.<br>2. Every document modification records timestamp, operation type, affected page ranges, and user/agent identity. | **Low** |
| **Information Disclosure (I)** | **Redaction Residual Leakage:** Sensitive personally identifiable information (PII) obscured by visual box remains extractable. | Attacker opens redacted PDF in text editor or CLI tool and copies underlying text stream. | **Irreversible Multi-Pass Sanitization:**<br>1. Rasterize visual area.<br>2. Delete underlying text glyphs and font references from content streams.<br>3. Strip metadata, thumbnails, form fields, and annotations matching redacted coordinates.<br>4. Re-linearize PDF and reconstruct xref table. | **Very Low** |
| **Information Disclosure (I)** | **Plaintext Key / Password Exposure:** Certificate private key passwords or document passwords leaked in crash dumps or swap. | Attacker inspects memory dump or configuration file after application crash. | 1. `SecureString` and `SecureBuffer` zero out heap memory upon object destruction.<br>2. Use Windows DPAPI / POSIX `mlock()` to prevent page file swapping.<br>3. Never log document passwords or key material. | **Very Low** |
| **Denial of Service (D)** | **PDF Decompression Bomb / Recursive Loop:** Malicious PDF containing nested object streams, circular xrefs, or infinite recursive dictionary definitions. | User opens 50 KB PDF that allocates 16 GB RAM upon decompression, freezing system. | 1. Enforce strict stream decompression size limits (max 50 MB per object stream).<br>2. Bound recursive dictionary and xref traversal depth (max depth 32).<br>3. Enforce maximum memory quota per document. | **Low** |
| **Elevation of Privilege (E)** | **Embedded Script / Launch Action Execution:** PDF exploits embedded JavaScript APIs or `/Launch` action to run local executables. | User opens invoice PDF; document triggers `/Launch /F (powershell.exe) /P (-enc ...)`. | 1. **Complete execution ban:** `/Launch` actions are completely disabled and stripped.<br>2. PDF JavaScript engine is disabled by default; no automatic code execution.<br>3. Embedded file attachments require explicit user extraction with security warning dialog. | **Zero** |
| **Elevation of Privilege (E)** | **MCP Arbitrary Command Execution:** MCP client instructs PDFinity to execute arbitrary system commands or load external native DLLs. | Attacker abuses MCP integration to trigger external code execution. | 1. No shell execution tool in MCP catalog.<br>2. MCP tools are strictly restricted to document domain operations (open, inspect, extract, search, merge).<br>3. Mandatory user confirmation modal for any destructive or export action. | **Zero** |

---

## 4. Non-Negotiable Security Invariants

The following rules are hardcoded into the architecture and must never be bypassed by configuration, flags, or external client commands:

1. **Never Overwrite Source Files In-Place:** All saves write to temporary files, validate integrity, and atomically swap.
2. **Never Execute Embedded Scripts Automatically:** Embedded PDF JavaScript and Launch actions are suppressed.
3. **Never Log Sensitive Data:** Document passwords, decryption keys, private certificate keys, and unredacted text must never appear in log files.
4. **Never Expose Unauthenticated Network Services:** MCP and Remote Controller are disabled by default, bind to localhost/trusted LAN only, and require mutual cryptographic authentication.
5. **Never Bypass User Consent for Destructive Actions:** Overwriting existing documents, permanent redaction, cryptographic signing, and mass batch deletions mandate explicit host-side confirmation.
