# PDFinity: Batch Processing, Profiles & Automation Architecture

**Module Version:** 1.0.0  
**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Status:** Official Batch Automation Specification  

---

## 1. Architectural Overview & Philosophy

PDFinity's Batch Automation system provides high-assurance, offline-first multi-file processing designed for enterprise document pipelines, prepress studios, legal archiving, and system scripting.

### Key Architectural Tenets:
1. **Deterministic Execution:** Given identical inputs and profile parameters, batch outputs are reproducible across runs.
2. **Per-File Error Isolation:** Failure or corruption in one file (e.g., malformed syntax, password-locked without credentials, zero bytes) is strictly isolated; it logs an error for that file and continues processing subsequent files in the queue.
3. **Non-Destructive File Safety:** Original source documents are treated as strictly immutable and read-only. Intermediate pipeline steps run inside isolated sandbox directories (`QTemporaryDir`) and move atomically to the target destination only upon 100% successful completion of all operations in the chain.
4. **Safe Resume:** Interrupted, cancelled, or partially failed jobs can be safely resumed. Files whose destination outputs already exist and pass structural validation (`IPdfEngine::OpenDocument`) are automatically skipped without re-processing.
5. **Bounded Resource Concurrency:** Worker threads are bounded (default 2, up to 16) with memory footprint throttling, preventing memory exhaustion or system freeze on resource-constrained hardware (e.g. legacy dual-core systems).
6. **Zero Shell Injection Risk:** The command-line interface validates all flags, options, and filesystem paths natively in C++20. It executes domain services directly in-process and never passes user-supplied input to shell interpreters (`cmd.exe`, `sh`, `system()`).

```
+-------------------------------------------------------------------------------------------------+
|                                    PDFinity Batch Pipeline                                      |
+-------------------------------------------------------------------------------------------------+
|                                                                                                 |
|   Input Files / Directory         Workflow Profile (Ordered Operation Chain)                    |
|   [file1.pdf, file2.pdf, ...] --> [ 1. OCR -> 2. PDF/A -> 3. Sanitize -> 4. Optimize ]          |
|                                                                                                 |
|                                         |                                                       |
|                                         v                                                       |
|   +-----------------------------------------------------------------------------------------+   |
|   |                        Worker Pool (Bounded Concurrency N Threads)                      |   |
|   |                                                                                         |   |
|   |   Worker 1: [file1.pdf] -> [temp0] -> [temp1 (OCR)] -> [temp2 (PDF/A)] -> [out/file1.pdf]|   |
|   |   Worker 2: [file2.pdf] -> [temp0] -> [temp1 (OCR)] -> [temp2 (PDF/A)] -> [out/file2.pdf]|   |
|   +-----------------------------------------------------------------------------------------+   |
|                                         |                                                       |
|                                         v                                                       |
|   Destination Collision Handling: [ AutoRename | Overwrite | Skip | Fail ]                      |
|                                                                                                 |
|   Safe Resume Check: [ Existing Output Valid? -> Skip redundant execution ]                    |
|                                                                                                 |
|   Summary & Audit Report: [ HTML Report | JSON Stream | Persistent History (batch_jobs.json) ]  |
+-------------------------------------------------------------------------------------------------+
```

---

## 2. Job Queue State Machine

The batch engine tracks both high-level job states and granular per-file item states:

### A. Job Lifecycle States (`BatchJobState`)
- `Queued`: Job is initialized, input files collected, and awaiting thread dispatch.
- `Running`: Worker threads are actively executing file operation chains.
- `Paused`: Worker threads pause before beginning the next file. Current in-flight files complete cleanly.
- `Succeeded`: All files in the batch completed with `Success` or `Skipped`.
- `Failed`: One or more files failed all retry attempts. (All non-failing files completed successfully).
- `Cancelled`: User requested job abort. Remaining pending files transition to `Cancelled`.

### B. Per-File Item States (`BatchFileStatus`)
- `Pending`: File is queued and waiting for an available worker thread.
- `Processing`: Operations are actively executing in a sandbox directory.
- `Success`: All operations in the chain succeeded and the output file was verified.
- `Error`: An operation failed permanently after exhausting configured retry attempts.
- `Skipped`: File was skipped due to collision policy (`Skip`) or safe resume validation.

---

## 3. Ordered Operation Chains & Built-In Profiles

Profiles define reusable, ordered sequences of atomic domain operations:

| Built-In Profile ID | Display Name | Operation Chain | Target Use Case |
| :--- | :--- | :--- | :--- |
| `builtin_pdfa_archival` | **PDF/A Archival & Metadata Sanitization** | 1. Convert to PDF/A-1b<br>2. Sanitize Metadata & Hidden Elements | Long-term compliance archiving (ISO 19005-1), government and legal records. |
| `builtin_web_optimize` | **Web Optimization (96 DPI Downsample)** | 1. Screen Optimization (96 DPI downsample, strip comments) | Fast web publishing, mobile viewing, and email attachment compression. |
| `builtin_prepress_optimize` | **Prepress Optimization (300 DPI Lossless)** | 1. Prepress Optimization (300 DPI, lossless stream compression) | Commercial printing prepress preparation. |
| `builtin_searchable_ocr` | **Searchable PDF Generation (OCR)** | 1. Tesseract OCR Text Layer (150 DPI) | Converting scanned paper archives into full-text searchable documents. |
| `builtin_full_archival` | **Full Archival Pipeline** | 1. OCR Text Layer<br>2. Convert to PDF/A-1b<br>3. Recommended Optimization | End-to-end digitize, conform, sanitize, and compress in a single pass. |
| `builtin_export_images` | **High-Resolution Image Export** | 1. Export Pages as PNG (300 DPI) | Extracting raster artwork or slide decks for presentations. |
| `builtin_export_text` | **Plain Text Extraction (UTF-8)** | 1. Export Text to UTF-8 | Content indexing and NLP corpus preparation. |

Custom profiles can be created, edited, cloned, and exported as standalone `.json` files via the **Batch Profile Editor** or CLI.

---

## 4. Input Rules, Filename Templates & Collision Handling

### A. Output Filename Formatting
Output filenames are calculated using customizable template patterns:
- `{basename}` or `{name}`: Original filename without extension (e.g. `contract`).
- `{ext}`: Source file extension (default `.pdf`).
- `{date}`: Current calendar date in `YYYYMMDD` format (e.g. `20261114`).
- `{time}`: Current timestamp in `HHMMSS` format (e.g. `143000`).

Default template: `{basename}_processed.pdf`.

### B. Collision Policies
When an output file already exists at the target destination:
1. `AutoRename` (Default): Appends an incrementing numeric counter before the extension (e.g. `contract_processed_1.pdf`, `contract_processed_2.pdf`).
2. `Overwrite`: Overwrites the existing file atomically.
3. `Skip`: Leaves the existing file untouched, marks the file item as `Skipped`, and logs the action.
4. `Fail`: Flags an error for that specific file and halts its operation, leaving existing data unmodified.

---

## 5. Safe Resume Semantics

When `resumeExistingOutputs` is enabled (default in GUI and via `--resume` in CLI):
1. The engine checks if `outputPath` exists and is non-empty (`file_size > 0`).
2. The engine calls `IPdfEngine::OpenDocument(outputPath)` to verify that the file is not partial or corrupt.
3. If valid, the file is marked `Skipped` with log: `"Output already exists and passes integrity check (safe resume)."`.
4. If invalid or missing, the file is processed from scratch.

---

## 6. Command-Line Interface (CLI) Manual

PDFinity includes a scriptable CLI for batch automation in headless servers, CI/CD runners, and desktop scripts:

### Commands Summary
```text
PDFinity batch [options]             Execute automated multi-file workflow
PDFinity batch-profiles [options]    Manage, list, export and import profiles
PDFinity batch-history [options]     Inspect or clear past job execution logs
```

### `PDFinity batch` Options
| Option | Description | Required / Default |
| :--- | :--- | :---: |
| `--profile <name\|id>` | Profile name or ID to execute | **Required** |
| `--input <path...>` | One or more input files or directory paths | **Required** |
| `--output <dir>` | Destination directory for processed files | **Required** |
| `--collision <policy>` | Collision policy: `autorename` (default), `overwrite`, `skip`, `fail` | Optional |
| `--concurrency <N>` | Concurrent worker thread count (1 to 16) | Default: `2` |
| `--retries <N>` | Retry attempts per file on transient I/O failure (0 to 3) | Default: `1` |
| `--template <pattern>` | Filename pattern (e.g. `{basename}_archived.pdf`) | Default: `{basename}_processed.pdf` |
| `--resume` | Skip files with existing valid destination outputs | Optional flag |
| `--recursive` | Recursively scan input directories for `.pdf` files | Optional flag |
| `--report <path>` | Export execution summary report (`.html` or `.json`) | Optional |
| `--json` | Output final execution statistics to `stdout` as JSON | Optional flag |

### Exit Codes
- `0`: All files processed successfully (or skipped per policy).
- `1`: Invalid arguments, missing input files, or non-existent profile.
- `2`: Partial or complete batch failure (at least one file failed all retries).

### CLI Usage Examples

**1. Archival PDF/A Conversion with HTML Report:**
```bash
PDFinity.exe batch --profile "builtin_pdfa_archival" --input "C:\Contracts" --output "C:\Archive" --recursive --report "C:\Archive\report.html"
```

**2. Automated CI/CD Web Optimization with JSON Output:**
```bash
PDFinity.exe batch --profile "builtin_web_optimize" --input "docs/*.pdf" --output "dist/web" --collision overwrite --concurrency 4 --json
```

**3. Resuming an Interrupted Batch Run:**
```bash
PDFinity.exe batch --profile "builtin_full_archival" --input "D:\Scans" --output "D:\Searchable" --resume
```

**4. Inspecting Profiles & History:**
```bash
PDFinity.exe batch-profiles --list
PDFinity.exe batch-history --list
```

---

## 7. Quality Assurance & Verification

- **Determinism:** Identical input fixtures consistently generate identical PDF/A-1b and optimized outputs verified by SHA-256 and byte inspections.
- **Failure Isolation:** Proven by `TestBatchService::testErrorIsolationPerFile()` where a corrupt header PDF fails locally while surrounding valid documents complete with 100% success.
- **Safe Cancellation:** Worker loops check atomic cancellation tokens before and after each file, terminating gracefully without leaving half-written destination files.
- **Output Integrity:** Every produced PDF is reopened with `IPdfEngine` and validated before concluding execution.
