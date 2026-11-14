# A2 release pipeline audit and repair record

Date: 15 November 2026 (Australia/Perth). Audited parent: `34f9b63c08e2593cb485d82780c1ae182f0bd049`. Current repair branch: `codex/a2-release-gates`. Code candidate before this report: `ac0069a70b42bd6f38aa340b2dd64f3071ec9677`. This report supersedes A1's stale remote/run facts, not its source findings. No product version bump, tag movement, merge or release publication has been performed by A2. Bootstrap patch releases are **queued**, not complete.

## Observed commit and workflow trace

| Stage | Existing lane at audit parent | Demonstrated gap / repair |
|---|---|---|
| Commit | Source history continued well beyond A1's a010244; current main/remote/tag were 34f9b63 | Preserved history. A1 recorded v1.1.0 at a010244; later snapshots recorded e605b19 and now 34f9b63. Tag movement predates these repairs; never move it again to ship the fixes |
| Required checks | ci.yml on main/develop pushes, PRs to main and dispatch; Windows/Linux/macOS build/tests and Android structure check | Authenticated main protection endpoint returned 404; rulesets empty. No enforced required-check or protected merge policy observed. Added Release Pipeline Validation; release preflight requires all five named CI jobs on the exact merged SHA, including latest-run freshness |
| Merge | No merge controller / automatic version bumper workflow found | Human/maintainer merge still required; no unverified auto-merge added. Release source must be in main ancestry |
| Version/tag | One existing scripts/bump_version.py; no YAML tag creator | Retained one bumper; preflight checks all mappings. The workflow never creates or moves tags. Stable tag must exist and resolve to full recorded SHA |
| Trigger | release.yml on v* push and inputless workflow_dispatch | Dispatch formerly inferred branch name as package version. Added explicit release_tag/source_sha/mode. Repository GITHUB_TOKEN tag pushes do not ordinarily cause another push workflow; explicit dispatch is the supported route |
| Packages | Four existing native build jobs | Retained all four builds/tests and existing SDK workaround. Removed package synthesis/error suppression; fresh native artifacts and install/runtime/removal receipts required |
| Aggregation | needs all four jobs, but Android failures/download were optional; flattened arbitrary dist output | Required eight packages, separate Actions artifact per OS/arch/type, preserved download subdirectories, duplicate basename rejection, explicit receipts and checksum/structure checks |
| Draft/verify | softprops published immediately with draft:false | Repaired the existing aggregate publishing job. One writer now creates/reuses a draft, uploads only contract files, verifies downloaded bytes, rechecks tag and CI, promotes, then verifies public downloads |
| Retry | Uncontrolled fallbacks and suppressed failures | No retries of non-idempotent writes. An interrupted draft can be resumed only with identical evidence/files; conflicting existing assets are rejected, not clobbered. Rebuild all lanes in a new attempt to avoid mixing receipts from attempts |

No second release workflow, duplicate publisher or competing version bumper was added. The existing publisher's unsafe internals were replaced with one draft transaction in `scripts/release_gate.py`; there remains exactly one publishing job in release.yml. Build jobs have contents:read/actions:read. Only that publishing job has contents:write and actions:write (the latter is needed for the bounded explicit-dispatch probe). Checkout credentials are not persisted. There is no pull_request_target trigger or release secret exposure to PR jobs. Signing secrets were neither read nor invented.

At the initial A2 snapshot the source had no deployment environments. The workflow now uses `release`; merely naming an environment does **not** establish required reviewers. Repository protection, environment review/branch policies and source/dependency provenance remain operator gates; A2 did not silently change account administration settings. Third-party action references are pinned to resolved official-tag commit SHAs; the tag-resolution evidence is retained locally. This is an integration review, not a claim of exhaustive vendor code audit. Existing macOS AGL dependency/build-file patching remains explicit in the recipe and is not proof of a universally supported Qt SDK.

## Exact trigger/rehearsal evidence

- Existing CI success at parent: [43650925851](https://github.com/jakepanlilio2000/PDFinity/actions/runs/43650925851).
- Existing release success at parent: [43650928587](https://github.com/jakepanlilio2000/PDFinity/actions/runs/43650928587). Its green status did not establish package validity.
- Safe parent rehearsal: [43654637930](https://github.com/jakepanlilio2000/PDFinity/actions/runs/43654637930), **success**, workflow_dispatch by jakepanlilio2000.
- Repository GITHUB_TOKEN child rehearsal: [43654663649](https://github.com/jakepanlilio2000/PDFinity/actions/runs/43654663649), **success**, workflow_dispatch actor github-actions[bot], parent ID in run title.

Both rehearsals executed workflow recipe `5eb87b6a058fd9536efc9166d04f0866e5997e28`, recorded tagged source `34f9b63c08e2593cb485d82780c1ae182f0bd049`, skipped all four package lanes, and skipped draft/upload/promotion. The parent used job-scoped GITHUB_TOKEN to dispatch one child with probe_parent set; the child cannot recursively dispatch another. Per-tag concurrency serializes parent/child. Thus explicit GITHUB_TOKEN dispatch has been observed, not inferred from YAML. It does **not** certify the later candidate's native builds or publishing transaction. A final-candidate rehearsal/CI result must be recorded separately after subsequent code changes.

Official trigger behavior: https://docs.github.com/en/actions/how-tos/write-workflows/choose-when-workflows-run/trigger-a-workflow . The exemption is workflow_dispatch/repository_dispatch; do not introduce a broadly privileged PAT to make a token-created tag push recurse.

Safe rehearsal after pushing the repaired branch:

```powershell
gh workflow run release.yml --ref codex/a2-release-gates -f release_tag=v1.1.0 -f source_sha=34f9b63c08e2593cb485d82780c1ae182f0bd049 -f mode=rehearsal
```

Rehearsal explicitly identifies a separate workflow recipe and tagged source; it builds no packages. For a real release, workflow and package source **must be the same SHA**. After merge, successful main checks, and a new immutable tag on that merge commit, dispatch using `--ref <that-tag>` with its exact source_sha and `mode=release`. If a repository GITHUB_TOKEN created the tag, explicitly dispatch rather than waiting for a push event. Do not dispatch main if it has advanced beyond the tagged candidate. A published existing release is rejected rather than republished.

## Version-source map

CMakeLists.txt project VERSION remains the canonical numeric version, currently 1.1.0. `release_versions.py` validates existing fields; it is not a second bumper. `bump_version.py` uses the same mapping, preflights every field and the concrete changelog entry before writing, preserves literal C++/RC escapes, rolls back on write/validation failure, and supports --dry-run/--check. It only accepts the shipping stable version syntax. Historical beta normalization is no longer a shipping control path.

| Fields | Validated mapping |
|---|---|
| CMake project / CPack | Numeric X.Y.Z; CPack already derives PROJECT_VERSION, so no new declaration |
| src/app/main.cpp | PrintVersion uses X.Y.Z Stable; QApplication uses X.Y.Z |
| src/app/cli_main.cpp; src/ui/AboutDialog.cpp | X.Y.Z Stable literals (no generated C++ version header exists) |
| src/mcp/McpTypes.hpp | MCP_SERVER_VERSION = X.Y.Z; protocol date is a separate version and untouched |
| packaging/windows/version.rc | Numeric X,Y,Z,0 and X.Y.Z.0; product display X.Y.Z Stable; NUL escapes preserved |
| packaging/windows/installer.iss | Numeric X.Y.Z for Inno metadata/filename; removes the Stable filename mismatch |
| packaging/windows/pdfinity.wxs | MSI X.Y.Z; optional MSI is not silently substituted for required EXE |
| packaging/macos/bundle.plist | CFBundleShortVersionString and CFBundleVersion both X.Y.Z (previous build version was 1) |
| packaging/linux/deb/control | Version X.Y.Z; native builder overrides architecture explicitly and must match amd64 contract |
| packaging/linux/rpm/pdfinity.spec | Version and tag_version X.Y.Z; package release 1, actual RPM metadata checked natively |
| packaging/linux/arch/PKGBUILD and .SRCINFO | pkgver/source URL/source directory agree; pkgrel agrees and resets to 1 for a new upstream version |
| Android build.gradle and AndroidManifest.xml in both roots | Display X.Y.Z Stable; code = X*10000 + Y*100 + Z; Y/Z <100 to prevent collisions; overflow rejected |
| Both AppStream metainfo XMLs | Current release attribute X.Y.Z; older descriptive prose/history not rewritten |
| README badges and tag destinations | Numeric badge, Stable display badge, both matching tag URLs |
| CHANGELOG.md | Latest numeric version entry must match; bumper does not invent release notes |

Script fallback defaults are not additional product-version sources. Production release jobs pass the preflight version explicitly. CI tar/DMG validation derives CMake. Fixed screenshot/mockup strings and historical release notes are not rewritten as current metadata.

## Release contract and verification

The checked-in contract is `packaging/release-contract.json` (stable shipping lane). Every listed entry is mandatory; optional recipe targets cannot replace one:

| Lane | Architecture | Required package |
|---|---|---|
| Windows | x86_64 payload | PDFinity-{version}-windows-x64-portable.zip |
| Windows | x86_64 payload; installer stub may be x86 | PDFinity-{version}-Setup-x64.exe |
| Linux | x86_64 | pdfinity_{version}_amd64.deb |
| Linux | x86_64 | pdfinity-{version}-1.x86_64.rpm |
| Linux | x86_64 | PDFinity-{version}-x86_64.AppImage |
| macOS | arm64 + x86_64 | PDFinity-{version}-macOS.dmg |
| Android | arm64-v8a + armeabi-v7a + x86_64 | PDFinity-{version}-android-universal-release.apk |
| Android | same three ABIs | PDFinity-{version}-android-universal-release.aab |

Four native receipts plus release-metadata.json and SHA256SUMS.txt accompany those packages. Each package Actions artifact name includes version, OS, architecture and package type; receipts have distinct names. No wildcard dist directory/staging upload, filename flattening or silent replacement is allowed. Validator recursively indexes basenames and rejects duplicates, extras and missing files before any draft creation.

`verify_native_packages.py` produces a lane receipt only after all its packages pass native structural/version/architecture, runtime, install and removal checks. It binds package size/hash to exact HEAD, workflow run ID and attempt; stores native commands/toolchain and signing state. Windows portable runtime must contain GUI, CLI, PDFium, Qt Core/Gui/Widgets/Network/PrintSupport and qwindows plugin. Windows installs run with an isolated target and no optional tasks, then uninstall. Linux DEB/RPM require actual dependency-resolving native installation, never --nodeps; AppImage extraction and launch exercise the portable payload. macOS verifies/mounts DMG, copies the app, checks both GUI/CLI universal arches and launches; Android requires native payloads, cryptographic signatures, numeric/display version checks, emulator install/start/process check and uninstall. AAB validation uses bundletool and existing signing environment. No debug signing fallback.

`release_assets.py` independently rechecks the complete set, receipt freshness, hashes and package structure. Native receipts are trusted CI evidence, not user-submitted assertions or cryptographic attestations; regression fixtures that simulate receipts are **not releasable packages**. Desktop unsigned outputs are explicitly represented and are not a claim of Authenticode/notarization/trusted distro signing. Android signing is mandatory. Document-signing certificates are unrelated to app package signing. SBOM.md was historical prose; this repair does not mislabel the new package inventory as a complete machine-readable dependency SBOM. A real SBOM remains a release-readiness gap under D12.

Local no-write validation uses:

```text
python scripts/release_versions.py
python -m unittest discover -s tests/pipeline -v
python scripts/release_assets.py --directory <downloaded-input-tree> --version <version> --sha <full-sha> --run-id <id> --attempt <attempt>
```

An interrupted draft retains uploaded files; identical files can be reused. Conflicting assets, wrong tag SHA or existing public release fail before replacing anything. Download verification happens before promotion and again via public URLs after promotion. If the latter fails, the run reports failure; no claim of a verified download is made. There is no automatic deletion of trusted assets, moved tag, or silent source substitution.

## Completeness-validator results and reproduced package defects

Local gate regression checks: 34 passing after the CI freshness correction. Actionlint 1.7.12 (official archive checksum verified) parses both workflows without errors; five edited shell recipes pass bash -n; changed Python modules compile. These checks are not native application/installer tests.

Public v1.1.0 files were actually downloaded without execution. Results and SHA-256 values are retained in local `docs/baseline-evidence/A2/public-download-validation.json`:

| Actual public download | Size | Result |
|---|---:|---|
| pdfinity_1.1.0_amd64.deb | 72 | REJECT: Missing control.tar |
| pdfinity-1.1.0-1.x86_64.rpm | 1080 | REJECT: Invalid/truncated RPM |
| PDFinity-1.1.0-x86_64.AppImage | 4158 | REJECT: Wrong ELF architecture |
| PDFinity-1.1.0-release.apk | 1594 | REJECT: Invalid DEX |

Their download URLs are https://github.com/jakepanlilio2000/PDFinity/releases/download/v1.1.0/<filename> (see evidence JSON for exact URLs and hashes). They were left untouched. The release had 36 assets, including flattened loose Windows files. Windows ZIP and macOS DMG installability were not independently verified here. No missing historical assets were recovered using current source. Recovery must use the original source/provenance and a separately recorded recipe SHA; source repairs get new patch releases.

## Isolated repair commits and remaining gate

| Defect | Commit | Status |
|---|---|---|
| D07-A2-01: completeness/structure/provenance gate | cc095b0b7d233f2e9bf3f0ec2c994fdca24ad00b | Local regression checks pass; native release queued |
| D07-A2-02: version mapping and single fail-closed bumper | 46d71c55f7dbb59d7b09c643dadb1bceace8cb43 | Current mappings pass at 1.1.0; no version bump/release |
| D07-A2-03: SHA/required-CI/draft transaction | e82c4f5f431d12db640ff066318c2143b073fc90 | Regression checks pass; no real package publication |
| D07-A2-04: reject synthesized packages, require native checks | a700b7e41f7b21344821c5ad5fadde38d8faf3b3 | Missing-dependency/stale-stage regressions pass; native lane execution pending |
| D07-A2-05: bounded repository-token dispatch rehearsal | 5eb87b6a058fd9536efc9166d04f0866e5997e28 | Parent and GITHUB_TOKEN child runs succeeded |
| D07-A2-06: propagate metadata/tool/SDK recipe errors | ac0069a70b42bd6f38aa340b2dd64f3071ec9677 | Actionlint passes; native checks must expose actual recipe failures |
| D07-A2-03 follow-up: latest-CI failure cannot reuse old success | 5908f4ed7ab9c26711d4ce32e4b643c34e7acbbf | Regression passes; earlier live rehearsal is stale for this source change |

All changes are infrastructure bootstrap repairs with queued patch releases, not claims that each has shipped. Source C++ implementation was preserved. The old successful release operation was not repeated. Intermediate commits retain application source, but their native usability/package readiness has not been proven merely by local Python tests.

Gate status: one authoritative path **implemented**; SHA/version/absent-asset guards **locally verified**; explicit repository-token dispatch **observed on 5eb87b6**; final-candidate native builds/packages, a real validated draft and public download transaction **pending**. Do not merge/tag/publish as release-ready yet.

Dependency order within A2: finish exact-candidate PR checks and final rehearsal; provision a clean Windows Qt/Inno runtime, compatible Debian and RPM distribution/package dependencies, pinned AppImage packager, macOS universal Qt/PDFium/deployment/signing recipe, and Android Qt/NDK/native activity/build configuration plus signing/emulator/bundletool. RPM currently runs on Ubuntu, whose native dependency database cannot prove a Fedora install; that is a real prerequisite, not grounds to add --nodeps. The existing Android build roots remain incomplete and no release keys were supplied. Complete native receipts for all contract entries, then verify a new immutable candidate's draft/public transaction. A1 D01 remains unfinished; the user explicitly prioritized A2 for this turn. Do not move to unrelated defects or historical backfills before this lane is honest and validated.

Local authentication findings: stored gh token invalid; existing Git Credential Manager credential can push the isolated branch and authorize API reads/dispatch in memory. No token was printed, installed, widened or changed. Evidence contains only redacted capability/results. External continuation: C:/Users/Administrator/.codex/visualizations/2026/11/14/01a1dbc5-bacb-78e3-8eac-b08117a76e4f/PDFinity-continuation.md .

The final-branch CI/rehearsal URLs and exact head SHA are recorded in the draft PR and external continuation after this documentation commit, avoiding a self-referential commit SHA. Native package receipts are still absent; no public release/draft is authorized by these test results alone.
