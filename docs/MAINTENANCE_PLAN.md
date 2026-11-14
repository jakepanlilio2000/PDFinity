# PDFinity Long-Term Maintenance and Sustainability Plan

**Document Version:** 1.0.0  
**Status:** Approved for Production Baseline  
**Scope:** Architecture, Dependencies, Release Cadence, Vulnerability Response, and CI Operations  

---

## 1. Principles of Maintenance

PDFinity is engineered as a dependable desktop utility. The maintenance plan adheres to four core tenets:

1. **Strict Offline-First Invariant:** No maintenance update, feature addition, or patch may introduce mandatory cloud dependencies, silent background telemetry, or automated network polling.
2. **Backward Compatibility Guarantee:** Existing user configuration files (`settings.json`), batch profiles (`batch_profiles.json`), and paired device databases (`paired_devices.json`) must load seamlessly across versions with automated forward migration.
3. **Zero Viral Copyleft Contamination:** The strict ban on GPL/AGPL dependencies remains permanent. All additions to the dependency tree must be permissive (Apache-2.0, MIT, BSD, Public Domain) or dynamic LGPLv3.
4. **Deterministic Multi-Platform Parity:** Core document, cryptographic, and automation logic must function identically across Windows, Linux, and macOS.

---

## 2. Release Cadence and Semantic Versioning

PDFinity adheres to [Semantic Versioning 2.0.0](https://semver.org/):

| Release Tier | Frequency | Scope | Validation Gate |
| :--- | :--- | :--- | :--- |
| **Patch Releases (`1.0.x`)** | Monthly (or Emergency SLA) | Security advisories, bug fixes, performance optimizations. No UI breakage or schema changes. | 100% pass on all 51 test suites; zero compiler warnings. |
| **Minor Releases (`1.x.0`)** | Quarterly (every 3 months) | New tools, format enhancements, new batch operations, new MCP tools. Full backward compatibility. | Full regression audit; documentation update; multi-platform packaging. |
| **Major Releases (`x.0.0`)** | 12 - 24 Months | Architectural updates, major UI overhauls, underlying Qt major version transitions. | Comprehensive ADR review; deprecation notices; schema migration. |

---

## 3. Vulnerability Response and SLA

When a security vulnerability is identified in PDFinity or an underlying third-party library, the following Service Level Agreement (SLA) applies:

| Severity (CVSS v3.1) | Examples | Response SLA | Target Delivery |
| :--- | :--- | :--- | :--- |
| **Critical (9.0 - 10.0)** | Remote code execution, unauthenticated buffer overflow | 24 Hours Triage | Patch released within **72 Hours** |
| **High (7.0 - 8.9)** | Sandbox escape, PIN bypass, denial-of-service loop | 48 Hours Triage | Patch released within **7 Business Days** |
| **Medium (4.0 - 6.9)** | Local path traversal in non-standard configurations | 5 Business Days | Included in next scheduled monthly patch |
| **Low (0.1 - 3.9)** | Minor information leakage in verbose debug logs | 10 Business Days | Included in next minor release |

---

## 4. Upstream Dependency Tracking

| Component | Upstream Source | Tracking Cadence | Upgrade Criteria |
| :--- | :--- | :--- | :--- |
| **Qt 6 Framework** | `https://code.qt.io` | Bi-weekly | Stick to Long Term Support (LTS) releases (6.8.x LTS). Test dynamic linker compatibility. |
| **Google PDFium** | Chromium Stable Branch | Monthly | Adopt security-patched revisions from stable Chromium builds. |
| **QPDF** | `https://github.com/qpdf/qpdf` | Monthly | Update upon stable release tags; verify ABI compatibility. |
| **OpenSSL** | `https://www.openssl.org` | Immediate for CVEs | Adopt LTS patch releases (3.3.x). |
| **SQLite** | `https://www.sqlite.org` | Quarterly | Update amalgamation file during minor releases. |
| **nlohmann/json** | `https://github.com/nlohmann/json` | Semi-annually | Header update; verify JSON-RPC 2.0 conformance. |

---

## 5. Deprecation Policy

- **CLI Commands & Flags:** Deprecated switches will issue a warning to stderr for a minimum of **two minor versions** before removal.
- **MCP Tool Catalog:** Deprecated tools will retain their JSON schemas and return warnings for at least **one minor version** prior to sunset.
- **Remote Controller Commands:** Allowlisted commands will remain stable across minor versions. Any deprecation will be flagged in `REMOTE_CONTROLLER_SPEC.md`.
