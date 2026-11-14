# Security Policy for PDFinity

**Organization / Publisher:** [StackSync Solutions](https://www.stacksyncsolutions.com/)  
**Lead Programmer:** [Jake Ashley Panlilio](https://github.com/jakepanlilio2000)  
**License:** Mozilla Public License Version 2.0 (MPL-2.0)  

PDFinity is engineered from the ground up for high-security, enterprise, and air-gapped environments. We take vulnerabilities and potential security regressions seriously.

---

## 1. Supported Versions

Security updates and critical vulnerability patches are actively maintained for the following release tiers:

| Version Tier | Status | Security Maintenance Window |
| :--- | :--- | :--- |
| **1.0.x Stable** | Supported | Current General Availability Release |
| **0.1.x Beta** | Supported | Maintenance Baseline |
| **Development (`main`)** | Active | Rolling Development Trunk |

---

## 2. Core Security & Privacy Invariants

PDFinity guarantees the following baseline security properties:
- **Zero Remote Telemetry:** The application makes zero uninitiated outgoing network connections. There are no analytics libraries, automatic telemetry beacons, or hidden update check threads.
- **Strict Local Sandboxing:** Optional Model Context Protocol (MCP) servers and companion Remote Controller connections operate strictly over local loopback (`127.0.0.1`) or user-approved local sockets.
- **Path Traversal Defense:** All filesystem inputs are canonicalized via `PathSanitizer` to prevent directory traversal (`../`) and symbolic link escape attacks.
- **Irreversible Redaction:** Redaction operations permanently excise underlying glyphs, vector paths, and metadata from the document stream rather than applying superficial visual black masks.
- **Cryptographic Memory Zeroing:** Passwords, digital signature keys, and pairing PINs are wrapped in `SecureString` to ensure deterministic memory zeroing upon destruction.

---

## 3. Reporting a Vulnerability

If you discover a security vulnerability in PDFinity, please disclose it responsibly:

### 3.1 Contact Methods
- **Email:** Send details to `security@stacksyncsolutions.com` and cc `jakepanlilio2000@users.noreply.github.com`.
- **Private Advisory:** Open a private security advisory through the GitHub repository at:
  [https://github.com/jakepanlilio2000/PDFinity/security/advisories/new](https://github.com/jakepanlilio2000/PDFinity/security/advisories/new)

Please do not open public GitHub issues for undisclosed security vulnerabilities.

### 3.2 Information to Include
To help us triage and remediate the issue quickly, please provide:
1. Description of the vulnerability and attack vector.
2. Affected version(s) and operating system(s).
3. Proof-of-concept (PoC) file, script, or reproducible step-by-step instructions.
4. Assessment of potential impact (CVSS score or severity estimate).

---

## 4. Vulnerability Response SLA

StackSync Solutions commits to the following Service Level Agreement (SLA) for confirmed security reports:

| Severity (CVSS v3.1) | Initial Triage | Fix & Patch Target |
| :--- | :--- | :--- |
| **Critical (9.0 - 10.0)** | Within 24 hours | Within 72 hours |
| **High (7.0 - 8.9)** | Within 48 hours | Within 7 business days |
| **Medium (4.0 - 6.9)** | Within 5 business days | Next monthly patch release |
| **Low (0.1 - 3.9)** | Within 10 business days | Next scheduled release |

Coordinated public disclosure will take place after the remediation patch is released to users.
