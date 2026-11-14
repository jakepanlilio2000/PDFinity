# Contributing to PDFinity

**Organization / Publisher:** [StackSync Solutions](https://www.stacksyncsolutions.com/)  
**Lead Programmer:** [Jake Ashley Panlilio](https://github.com/jakepanlilio2000)  
**License:** Mozilla Public License Version 2.0 (MPL-2.0)  

Thank you for your interest in contributing to PDFinity. We welcome code contributions, issue reports, performance enhancements, and platform packaging recipes from the open-source community.

---

## 1. Architectural & Privacy Invariants

All contributions must adhere to the core architectural principles of PDFinity:

1. **Strict Offline-First Design:** Never introduce required network calls, analytics SDKs, background telemetry, or cloud-account login gates. Any networking functionality (such as local loopback MCP stdio or LAN Remote Controller pairing) must be strictly opt-in, disabled by default, and isolated to local sockets.
2. **Strict Copyleft Ban (No GPL / AGPL):** Do not introduce dependencies licensed under GPLv2, GPLv3, or AGPLv3. All dependencies must be permissive (Apache-2.0, MIT, BSD, Public Domain) or dynamic LGPLv3 (such as Qt 6).
3. **Atomic Document Writes:** Document persistence must never overwrite target files in place. Write to temporary files first, verify integrity, and atomically rename upon completion.
4. **Memory Safety & Zeroing:** Sensitive credentials (passwords, encryption keys, pairing PINs) must use `SecureString` to guarantee memory zeroing on destruction.

---

## 2. Development Setup & Coding Standards

### 2.1 C++ Standard & Compiler Support
- **Language Standard:** ISO C++20 (`-std=c++20` or `/std:c++20`).
- **Supported Compilers:**
  - Microsoft Visual C++ (MSVC 2022+ / v143+)
  - GCC 11+
  - Clang 14+ / Apple Clang 14+
- **UI Framework:** Qt 6.5+ LTS (`QtWidgets`, `QtGui`, `QtCore`, `QtPrintSupport`).

### 2.2 Code Style Conventions
- **Naming:**
  - Classes and Structs: `PascalCase` (e.g., `DocumentWorkspace`, `RedactionService`)
  - Methods and Functions: `PascalCase` (e.g., `OpenFile()`, `ExecuteRedaction()`)
  - Member Variables: `m_camelCase` (e.g., `m_documentModel`, `m_renderThread`)
  - Local Variables: `camelCase` (e.g., `pageIndex`, `tempFilePath`)
  - Constants & Enums: `PascalCase` or `UPPER_SNAKE_CASE` (e.g., `DefaultPageDpi`)
- **Headers:** Use `#pragma once` for include guards.
- **Resource Management:** Follow RAII strictly. Prefer `std::unique_ptr` and `std::shared_ptr`. Avoid raw owning pointers.

---

## 3. Testing Requirements

Every bug fix or new feature must be accompanied by automated unit or integration tests:
- Place unit tests under `tests/unit/`.
- Place integration workflows under `tests/integration/`.
- Run the test suite locally using CTest:
  ```bash
  ctest --preset windows-x64-release --output-on-failure
  # or on Linux:
  ctest --preset linux-x64-release --output-on-failure
  ```

---

## 4. Submitting Pull Requests

1. Fork the repository on GitHub: `https://github.com/jakepanlilio2000/PDFinity`
2. Create a topical branch from `main`:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. Commit your changes using conventional commit messages:
   - `feat(...)`: New features
   - `fix(...)`: Bug fixes
   - `perf(...)`: Performance optimizations
   - `docs(...)`: Documentation updates
   - `test(...)`: Adding or updating test cases
4. Ensure all CMake builds and CTest suites pass without compiler warnings.
5. Push to your fork and open a Pull Request against `main`. Provide a clear description of the problem solved and test coverage.
