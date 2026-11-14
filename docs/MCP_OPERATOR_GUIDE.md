# PDFinity: Model Context Protocol (MCP) Operator & Security Guide

**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Application Version:** 1.0.0  
**Protocol Version:** MCP `2024-11-05` / JSON-RPC 2.0  
**Target Audience:** System Administrators, Security Engineers, AI Application Developers, Desktop Operators  
**Last Updated:** 2026-11-14  

---

## 1. Executive Summary & Core Philosophy

PDFinity's Model Context Protocol (MCP) server provides an offline-first, sandboxed bridge allowing external AI assistants (such as Claude Code, Cursor, and standard MCP clients) to safely inspect, search, analyze, and transform PDF documents via standard JSON-RPC 2.0 messages over standard I/O (`stdio`).

### Core Design Principles
1. **Strictly Off-by-Default:** The MCP server is disabled upon initial installation. No external client can invoke PDFinity domain services without deliberate operator enablement.
2. **Approved Roots Filesystem Sandbox:** Document access is bounded strictly within operator-approved directory roots. Path traversal attacks (`../`, symlink jumping, UNC paths) are blocked before any file I/O occurs.
3. **Immutable Source Preservation:** Write and transformation tools strictly require distinct destination output paths. Destructive operations (redaction, form flattening) are blocked from mutating source documents in-place.
4. **Zero Secret Leakage:** Cryptographic keys, passwords, certificates, and auth tokens are scrubbed by `LogRedactor` and `McpSecurityPolicy::RedactSecrets` before any audit log entry or JSON-RPC response is created.
5. **No Shell Invocations:** All tool calls map directly to in-process C++20 domain services (`DocumentIntelligenceService`, `PreflightService`, `SignatureService`, `OptimizationService`, `SanitizationService`, etc.). No external shell strings are ever evaluated.

---

## 2. Protocol Specification & Architecture

### Supported Standards
- **Protocol Version:** `2024-11-05`
- **Wire Format:** JSON-RPC 2.0 (newline-delimited JSON over `stdio`)
- **Transport:** Standard Input (`stdin`) and Standard Output (`stdout`), with diagnostic logging directed to application logs.

```text
+---------------------+                       +---------------------------------------+
|                     |  JSON-RPC 2.0 (stdio) |              PDFinity                 |
|  AI Client / IDE    | --------------------> | [McpStdioTransport]                   |
|  (Claude / Cursor)  | <-------------------- |         |                             |
+---------------------+                       | [McpSecurityPolicy] (Sandbox Guard)   |
                                              |         |                             |
                                              | [McpServer Engine]                    |
                                              |         |                             |
                                              | [McpToolCatalog] (11 Domain Tools)    |
                                              |         |                             |
                                              | [C++ Domain Services & PDFium Core]   |
                                              +---------------------------------------+
```

### Lifecycle Methods
- `initialize`: Client negotiates capabilities and protocol version (`2024-11-05`).
- `notifications/initialized`: Client acknowledges capability handshake.
- `ping`: Lightweight heartbeat / connection verification.
- `tools/list`: Returns schemas for all authorized MCP tools.
- `tools/call`: Executes an authorized domain tool with validated arguments.
- `resources/list` & `resources/read`: Inspects authorized PDF documents and structural reports as readable MCP resources.

---

## 3. Threat Model & Security Posture

### Threat Assessment Matrix

| Threat Scenario | Vector | PDFinity Mitigation |
| :--- | :--- | :--- |
| **Path Traversal** | AI client passes `../../Windows/System32` or arbitrary absolute paths. | Strict canonicalization via `security::PathSanitizer` and prefix verification against `approvedRoots`. Traversal is blocked with error `-32001`. |
| **Destructive Data Loss** | Malicious or hallucinated tool call attempts to overwrite original contracts. | Source immutability check blocks execution if `outputPath == inputPath`. Destructive tools (`pdf_redact_and_sanitize`, `pdf_flatten_forms`) require explicit consent or `--allow-destructive`. |
| **Denial of Service (DoS)** | Client floods server with multi-gigabyte JSON payloads. | Server checks raw request size against `maxRequestSizeBytes` (default 10 MB) prior to JSON parsing. Violations return `-32003`. |
| **Secret Exfiltration** | Client attempts to inspect private cryptographic signing keys or passwords. | Sensitive fields (`password`, `privateKey`, `certificate`, `token`) are automatically redacted with `[REDACTED]` tokens in both responses and audit logs. |
| **Unauthorized Execution** | External script attempts to trigger MCP while user has it turned off. | Server returns JSON-RPC error `-32001` for all tool and resource operations unless explicitly activated by operator. |

### Tool Categorization & Gating Policies

Each MCP tool is classified under one of five risk tiers:

```cpp
enum class McpToolCategory {
    ReadOnly,           // Non-destructive inspection; permitted in approved roots
    Write,              // Creates new output documents; requires valid output path
    Destructive,        // Modifies layout or burns content; gated by operator policy
    SecuritySensitive,  // Cryptographic operations or security auditing
    Admin               // Configuration and session management
};
```

#### Consent Policies
- **`AskForDestructive` (Default):** Read-only and non-destructive write operations proceed automatically within approved roots. Destructive tools require `--allow-destructive` CLI flag or operator confirmation.
- **`TrustApprovedRootsOnly`:** All operations within approved roots are permitted without interactive dialogs, while any operation outside approved roots is rejected.
- **`AlwaysAsk`:** Conservative mode requiring operator confirmation for all tool calls.
- **`DenyAll`:** Emergency lockdown mode; all tool invocations are blocked.

---

## 4. Tool Catalog & Schema Reference

PDFinity registers 11 specialized tools calling in-process domain services:

### 1. `pdf_inspect` (Category: `ReadOnly`)
Inspects document structural metadata, fonts, images, attachments, and layers.
- **Input Parameters:**
  - `path` (string, required): Absolute or relative path to PDF within approved roots.
- **Returns:** Detailed structural report including page count, PDF version, security flags, font inventory, and image dimensions.

### 2. `pdf_extract_text` (Category: `ReadOnly`)
Extracts text content from a specific page or the entire document.
- **Input Parameters:**
  - `path` (string, required): Document path.
  - `page` (integer, optional): 1-based page index (omit for all pages).
- **Returns:** UTF-8 text content with page-by-page structural delineation.

### 3. `pdf_search` (Category: `ReadOnly`)
Executes an offline text search across document pages.
- **Input Parameters:**
  - `path` (string, required): Document path.
  - `query` (string, required): Search query string.
  - `matchCase` (boolean, optional, default: false): Case-sensitive matching.
  - `wholeWord` (boolean, optional, default: false): Match whole words only.
- **Returns:** List of occurrences with page numbers and character bounding box coordinates.

### 4. `pdf_preflight_validate` (Category: `ReadOnly`)
Executes preflight inspection verifying fonts, image resolution, and compliance warnings.
- **Input Parameters:**
  - `path` (string, required): Document path.
- **Returns:** Preflight report with pass/warning/error findings, severity levels, and remediation guidance.

### 5. `pdf_verify_signatures` (Category: `ReadOnly`)
Verifies cryptographic digital signatures and document integrity.
- **Input Parameters:**
  - `path` (string, required): Document path.
- **Returns:** List of signatures, signer subject identities, signing timestamps, and byte-range cryptographic integrity statuses.

### 6. `pdf_optimize` (Category: `Write`)
Compresses streams, downsamples images, and removes orphaned resources.
- **Input Parameters:**
  - `inputPath` (string, required): Source PDF path.
  - `outputPath` (string, required): Destination PDF path (must not equal inputPath).
  - `qualityPreset` (string, optional, enum: `"web"`, `"prepress"`, `"lossless"`).
- **Returns:** Before/after file sizes, reduction percentage, and operation status.

### 7. `pdf_convert_to_pdfa` (Category: `Write`)
Transforms document into standard archival PDF/A-1b or PDF/A-2b format.
- **Input Parameters:**
  - `inputPath` (string, required): Source PDF path.
  - `outputPath` (string, required): Target archival PDF path.
  - `profile` (string, optional, enum: `"PDF/A-1b"`, `"PDF/A-2b"`, default: `"PDF/A-2b"`).
- **Returns:** Conformance validation status and resulting output file path.

### 8. `pdf_ocr` (Category: `Write`)
Performs offline OCR on scanned pages, creating a searchable invisible text layer.
- **Input Parameters:**
  - `inputPath` (string, required): Scanned document path.
  - `outputPath` (string, required): Searchable output document path.
  - `language` (string, optional, default: `"eng"`): OCR language pack identifier.
- **Returns:** Character count generated and OCR execution summary.

### 9. `pdf_flatten_forms` (Category: `Destructive`)
Permanently flattens interactive AcroForms into visual page content.
- **Input Parameters:**
  - `inputPath` (string, required): Form document path.
  - `outputPath` (string, required): Destination flattened PDF path.
- **Returns:** Number of fields flattened and confirmation of read-only output.

### 10. `pdf_redact_and_sanitize` (Category: `Destructive`)
Irreversibly burns redaction annotations and scrubs metadata, hidden layers, and attachments.
- **Input Parameters:**
  - `inputPath` (string, required): Unredacted source PDF path.
  - `outputPath` (string, required): Secure redacted destination PDF path.
  - `sanitizeMetadata` (boolean, optional, default: true): Remove document metadata.
- **Returns:** Scrubbed object statistics and sanitization verification certificate.

### 11. `mcp_list_approved_roots` (Category: `ReadOnly`)
Lists the directory roots currently permitted by the operator's security sandbox.
- **Input Parameters:** None.
- **Returns:** Vector of normalized approved root directory paths.

---

## 5. Desktop Management Interface

Operators can configure, monitor, and revoke MCP access directly in the desktop GUI:

- **Access Point:** `Tools → Model Context Protocol (MCP) Manager...` (`Ctrl + Shift + M`).
- **Core Controls:**
  - **Enable MCP Integration:** Master toggle. When unchecked, all external requests are refused.
  - **Consent Policy Dropdown:** Select between `Ask for Destructive Operations`, `Trust Approved Roots Only`, `Always Ask`, or `Deny All`.
  - **Approved Directory Roots:** Add or remove folders permitted for AI document access.
  - **Active Session Monitor:** Displays connected client name, version, negotiated protocol, and handled request count.
  - **Revoke Session Button:** Instantly terminates the active client session, resets credentials, and clears cache.
  - **Audit Log Inspector:** Real-time log table showing timestamp, client ID, method, target path, outcome status (`SUCCESS`, `BLOCKED`, `DENIED`), and diagnostic details.

---

## 6. Command-Line Interface Reference

Launch the standalone headless stdio MCP server using the `mcp-server` subcommand:

```bash
# Syntax
PDFinity.exe mcp-server --approved-roots <path...> [options]
```

### CLI Arguments

| Argument | Description | Required / Default |
| :--- | :--- | :--- |
| `--approved-roots <path...>` | One or more directory paths permitted for file access. | **Required** |
| `--allow-destructive` | Enables destructive tools (`pdf_flatten_forms`, `pdf_redact_and_sanitize`). | Optional (Default: False) |
| `--consent-policy <policy>` | Enforces consent policy (`AskForDestructive`, `TrustApprovedRootsOnly`, `AlwaysAsk`, `DenyAll`). | Optional (Default: `AskForDestructive`) |
| `--max-request-size <N>` | Maximum incoming JSON payload in bytes. | Optional (Default: 10485760 / 10MB) |
| `-h, --help` | Displays command usage and exits. | Optional |

### Usage Examples

```bash
# Standard local development session with Claude Code or Cursor
PDFinity.exe mcp-server --approved-roots "C:\Users\User\Documents\Research" "D:\LegalPdfs"

# Automated CI/CD server with destructive tools permitted
PDFinity.exe mcp-server --approved-roots "C:\Pipeline\Workspace" --allow-destructive --consent-policy TrustApprovedRootsOnly

# Highly constrained audit session with custom 2MB payload ceiling
PDFinity.exe mcp-server --approved-roots "C:\Audits" --max-request-size 2097152
```

---

## 7. AI Assistant Integration Guide

### A. Claude Code Configuration
Add PDFinity to your Claude Code configuration file (`~/.claude/claude.json` or project `.claude.json`):

```json
{
  "mcpServers": {
    "pdfinity": {
      "command": "C:\\Program Files\\PDFinity\\bin\\PDFinity.exe",
      "args": [
        "mcp-server",
        "--approved-roots",
        "C:\\Users\\User\\Documents\\PDFs"
      ]
    }
  }
}
```

### B. Cursor Configuration
Add to `.cursor/mcp.json`:

```json
{
  "mcpServers": {
    "pdfinity-docs": {
      "command": "C:\\Users\\User\\AppData\\Local\\PDFinity\\PDFinity.exe",
      "args": [
        "mcp-server",
        "--approved-roots",
        "C:\\Workspace\\Contracts"
      ]
    }
  }
}
```

### C. Generic MCP Client Configuration
When invoking via any standard MCP client or automation tool bridge:

```json
{
  "name": "pdfinity",
  "command": "C:/Users/ADMINISTRATOR/Desktop/projects/PDFinity/build/windows-x64-release/bin/PDFinity.exe",
  "args": [
    "mcp-server",
    "--approved-roots",
    "C:/Users/ADMINISTRATOR/Desktop/projects/PDFinity/tests/fixtures"
  ],
  "transport": "stdio",
  "protocolVersion": "2024-11-05"
}
```

---

## 8. Audit Logging and Diagnostic Verification

PDFinity maintains a ring buffer of the 500 most recent MCP operations in memory and commits all events to the application log (`%LOCALAPPDATA%\PDFinity\logs\pdfinity.log`).

### Typical Audit Event Entries

```text
[2026-11-14 15:00:59.043] [INFO ] [McpSecurity] Added approved root: C:\Users\User\Documents
[2026-11-14 15:00:59.066] [INFO ] [McpSecurity] [SUCCESS] initialize (ClaudeCode): Client connected with protocol: 2024-11-05
[2026-11-14 15:01:02.115] [INFO ] [McpSecurity] [SUCCESS] tools/call (pdf_inspect): Inspected annual_report.pdf
[2026-11-14 15:01:04.220] [WARN ] [McpSecurity] [BLOCKED] tools/call (pdf_inspect): Path traversal/Sandbox violation: Path 'C:\Windows\System32\drivers\etc\hosts' is outside all approved directory roots
[2026-11-14 15:01:06.412] [WARN ] [McpSecurity] [DENIED] tools/call (pdf_flatten_forms): Destructive tool 'pdf_flatten_forms' is blocked: requires explicit operator consent or --allow-destructive flag
```
