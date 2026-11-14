# PDFinity Digital Signatures & Document Security Specification

## 1. Executive Summary & Design Principles

PDFinity provides two fundamentally separate signature capabilities to guarantee legal clarity, cryptographic safety, and standards compliance:
1. **Visual Signatures (`FEAT-SG-01`):** Graphical appearances (hand-drawn cursive, stylized typography, or imported scanned image/stamp) placed as page annotation/content elements. They provide visible assent but **zero cryptographic integrity** or tamper evidence. The UI explicitly flags them with amber warning banners: *"VISUAL SIGNATURE ONLY (No cryptographic security)"*, and the data model enforces `isCryptographicBound = false`.
2. **Cryptographic Digital Signatures (`FEAT-SG-02`, `FEAT-SG-03`):** Standards-compliant ISO 32000-1 / ISO 32000-2 signatures utilizing detached PKCS#7 / CMS (`adbe.pkcs7.detached`) format over an exact `/ByteRange`. Private keys are stored in a DPAPI-encrypted hardware/OS vault (`%LOCALAPPDATA%\PDFinity\certs\`), never held in plaintext, and validated against byte tampering and post-signing alterations.

---

## 2. Technical Architecture

### 2.1 Visual Signatures vs. Cryptographic Signatures

| Attribute | Visual Signature (`FEAT-SG-01`) | Cryptographic Signature (`FEAT-SG-02`) |
| :--- | :--- | :--- |
| **Object Representation** | Page image/vector overlay | `/Type /Sig` dictionary + AcroForm field widget |
| **Cryptographic Hash** | None | SHA-256 over document `/ByteRange` |
| **Security Handler** | None | `/Filter /Adobe.PPKLite` / `/SubFilter /adbe.pkcs7.detached` |
| **Tamper Detection** | Impossible | Cryptographically verified via PKCS#7 CMS digest |
| **Key Material Required** | None | X.509 Certificate + RSA/ECC Private Key |
| **UI Presentation** | Amber informational badge | Green security shield or Red tamper warning |
| **Data Contract Flag** | `isCryptographicBound = false` | `isCryptographicBound = true` |

### 2.2 ISO 32000 Incremental Update Architecture

PDFinity implements digital signing using non-destructive **incremental updates** as mandated by ISO 32000-1 Section 12.8:
- The original document bytes remain completely unaltered.
- An incremental revision is appended containing:
  1. `Signature Value Dictionary` (`/Type /Sig`, `/Filter /Adobe.PPKLite`, `/SubFilter /adbe.pkcs7.detached`, `/ByteRange`, `/Contents <hex>`)
  2. `Signature Field Annotation Widget` (`/Type /Annot`, `/Subtype /Widget`, `/FT /Sig`, `/V <sigObj>`, `/T (SignatureN)`)
  3. `AcroForm Dictionary` (`/Fields [...]`, `/SigFlags 3`)
  4. `Updated Root Catalog` referencing `/AcroForm` while preserving existing `/Pages` hierarchy and metadata
  5. `Incremental Cross-Reference (xref) Table` and `trailer` with `/Prev <oldStartXref>` pointer
- The `/ByteRange [0, len1, start2, len2]` excludes the exact byte span occupied by the hexadecimal PKCS#7 envelope `<...>`, allowing detached verification.

### 2.3 Windows DPAPI Secure Certificate Vault

Private key material and PKCS#12 (`.pfx` / `.p12`) archives are protected according to the following security controls:
- **Zero Plaintext Storage:** Neither private keys nor unlock passwords are ever written to disk or configuration files in plaintext.
- **DPAPI Encryption:** Raw PKCS#12 bundles are encrypted via Windows Data Protection API (`CryptProtectData`) with `CRYPTPROTECT_UI_FORBIDDEN` before writing to `%LOCALAPPDATA%\PDFinity\certs\<thumbprint>.pfx.enc`.
- **Session Memory Scrubbing:** Decrypted buffers and password containers (`security::SecureString`) use `SecureZeroMemory` in their destructors to prevent heap snooping.
- **IPC / Protocol Protection:** Private keys and certificate passwords are strictly forbidden from being transmitted across MCP (Model Context Protocol) or Remote Controller endpoints.

---

## 3. Signature Verification & Tamper Detection

### 3.1 Verification Algorithm (`SignatureService::ValidateDocumentSignatures`)

1. **Parser & ByteRange Bounds Check:**
   - Scans all `/Type /Sig` dictionaries.
   - Extracts `/ByteRange [offset1, length1, offset2, length2]`.
   - Validates that `offset1 == 0`, `length1 < offset2`, and `offset2 + length2 <= fileSize`. If bounds exceed physical file size, the signature is immediately marked `isTampered = true`.
2. **Digest Recomputation:**
   - Feeds byte slices `[0 .. length1]` and `[offset2 .. offset2 + length2]` into a SHA-256 cryptographic stream.
   - Computes expected SHA-256 hash.
3. **PKCS#7 Detached Signature Verification:**
   - Parses the DER-encoded PKCS#7 blob from `/Contents`.
   - Invokes CryptoAPI `CryptVerifyDetachedMessageSignature` with `szOID_NIST_sha256`.
   - Verifies mathematical match between digest and signer's public key.
4. **Post-Signing Revision Detection:**
   - Checks if `offset2 + length2 < fileSize`.
   - If trailing bytes exist past the signed byte range, flags `hasModificationsAfterSigning = true` with byte count differential.
5. **Certificate Chain & Revocation:**
   - Builds certificate chain using `CertGetCertificateChain`.
   - Checks expiration dates (`NotBefore` / `NotAfter`) against signing timestamp.
   - Evaluates offline vs. online revocation state.

### 3.2 Offline Validation & Revocation Disclosure

> [!WARNING]
> **Offline Validation Limitations:** In offline or air-gapped environments, PDFinity verifies cryptographic signature integrity, digest validity, and self-consistency of the embedded certificate chain. Real-time revocation checking via OCSP (Online Certificate Status Protocol) or CRL (Certificate Revocation Lists) requires network access to external Certificate Authority endpoints. When running offline, the audit report explicitly states: *"Revocation check: Skipped (Offline mode - no network endpoints queried)"*.

---

## 4. Document Encryption & Permissions (`FEAT-SC-01`)

PDFinity implements ISO 32000 standard security handler encryption:
- **Algorithms:** AES-256 (Revision 6 / V=5) and AES-128 (Revision 4 / V=4).
- **Dual-Password Model:**
  - **User Password:** Required to open and decrypt document contents.
  - **Owner Password:** Required to modify permissions or remove encryption.
- **Permissions Bitmask (ISO 32000-1 Section 7.6.3.2):**
  - Bit 3 (4): Print (low quality / general)
  - Bit 4 (8): Modify document contents
  - Bit 5 (16): Copy or extract content
  - Bit 6 (32): Add or modify annotations & form fields
  - Bit 9 (256): Fill in existing form fields
  - Bit 10 (512): Extract for accessibility (screen readers)
  - Bit 11 (1024): Assemble document (insert, rotate, delete pages)
  - Bit 12 (2048): High quality printing

---

## 5. Independent Verification with OpenSSL

PDFinity digital signatures are independently verifiable using standard OpenSSL CLI tools:
```bash
# 1. Extract PKCS#7 DER envelope from PDF /Contents
# 2. Inspect certificate structure and signer attributes:
openssl pkcs7 -inform DER -in sig.p7 -print_certs -text

# 3. Verify detached signature against the signed document byte streams:
openssl smime -verify -inform DER -in sig.p7 -content signed_bytes.bin -noverify
```
Automated integration test `test_signature_persistence.cpp` (`testIndependentValidatorValidation`) executes this verification against `openssl.exe`, guaranteeing cross-vendor interoperability.
