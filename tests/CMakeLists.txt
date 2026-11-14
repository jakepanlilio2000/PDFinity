# Define source directory for test fixtures access
add_compile_definitions(PDFINITY_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

# Unit test: Platform & Security
add_executable(test_platform
    unit/test_platform.cpp
)
target_link_libraries(test_platform PRIVATE
    pdfinity_core
    Qt6::Test
)
add_test(NAME PlatformSecurityTests COMMAND test_platform)

# Unit test: Config & Persistence
add_executable(test_config
    unit/test_config.cpp
)
target_link_libraries(test_config PRIVATE
    pdfinity_core
    Qt6::Test
)
add_test(NAME ConfigPersistenceTests COMMAND test_config)

# UI Smoke test: Application Shell
add_executable(test_shell_smoke
    ui/test_shell_smoke.cpp
)
target_link_libraries(test_shell_smoke PRIVATE
    pdfinity_ui
    pdfinity_core
    Qt6::Test
    Qt6::Widgets
)
add_test(NAME ShellSmokeTests COMMAND test_shell_smoke)

# Integration test: PDF Engine Adapter & Document Lifecycle
add_executable(test_pdf_engine
    integration/test_pdf_engine.cpp
)
target_link_libraries(test_pdf_engine PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME PdfEngineTests COMMAND test_pdf_engine)

# Unit test: Page Cache LRU & Bounded Memory
add_executable(test_page_cache
    unit/test_page_cache.cpp
)
target_link_libraries(test_page_cache PRIVATE
    pdfinity_core
    Qt6::Test
    Qt6::Gui
)
add_test(NAME PageCacheTests COMMAND test_page_cache)

# Integration test: PDF Text Extraction & Search
add_executable(test_pdf_search
    integration/test_pdf_search.cpp
)
target_link_libraries(test_pdf_search PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME PdfSearchTests COMMAND test_pdf_search)

# Integration test: PDF Bookmarks / Outline Hierarchy
add_executable(test_pdf_bookmarks
    integration/test_pdf_bookmarks.cpp
)
target_link_libraries(test_pdf_bookmarks PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME PdfBookmarksTests COMMAND test_pdf_bookmarks)

# UI Stress & Regression test: Large Document Viewer
add_executable(test_viewer_stress
    ui/test_viewer_stress.cpp
)
target_link_libraries(test_viewer_stress PRIVATE
    pdfinity_ui
    pdfinity_core
    Qt6::Test
    Qt6::Widgets
    pdfium
)
add_test(NAME ViewerStressTests COMMAND test_viewer_stress)

# Unit test: Page Range Parser
add_executable(test_page_range_parser
    unit/test_page_range_parser.cpp
)
target_link_libraries(test_page_range_parser PRIVATE
    pdfinity_core
    Qt6::Test
)
add_test(NAME PageRangeParserTests COMMAND test_page_range_parser)

# Integration test: Document Assembly (Merge, Split, Extract, Reorder)
add_executable(test_document_assembly
    integration/test_document_assembly.cpp
)
target_link_libraries(test_document_assembly PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME DocumentAssemblyTests COMMAND test_document_assembly)

# UI test: Page Organizer, Merge & Split Dialogs
add_executable(test_page_organizer
    ui/test_page_organizer.cpp
)
target_link_libraries(test_page_organizer PRIVATE
    pdfinity_ui
    pdfinity_core
    Qt6::Test
    Qt6::Widgets
    pdfium
)
add_test(NAME PageOrganizerTests COMMAND test_page_organizer)

# Unit test: Annotation Command Stack (Undo/Redo)
add_executable(test_annotation_command
    unit/test_annotation_command.cpp
)
target_link_libraries(test_annotation_command PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME AnnotationCommandTests COMMAND test_annotation_command)

# Integration test: Annotation Persistence & Independent Appearance
add_executable(test_annotation_persistence
    integration/test_annotation_persistence.cpp
)
target_link_libraries(test_annotation_persistence PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME AnnotationPersistenceTests COMMAND test_annotation_persistence)

# UI test: Annotation Studio Toolbar, Properties & Sidebar Tree
add_executable(test_annotation_ui
    ui/test_annotation_ui.cpp
)
target_link_libraries(test_annotation_ui PRIVATE
    pdfinity_ui
    pdfinity_core
    Qt6::Test
    Qt6::Widgets
    pdfium
)
add_test(NAME AnnotationUiTests COMMAND test_annotation_ui)

# Unit test: Form Service (Sanitization, Validation, JSON Export/Import)
add_executable(test_form_service
    unit/test_form_service.cpp
)
target_link_libraries(test_form_service PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME FormServiceTests COMMAND test_form_service)

# Integration test: AcroForm & XFA Persistence, Filling, Reset, Flattening
add_executable(test_form_persistence
    integration/test_form_persistence.cpp
)
target_link_libraries(test_form_persistence PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME FormPersistenceTests COMMAND test_form_persistence)

# UI test: Form Inspector & Field Properties Dialog, XFA Banner
add_executable(test_form_ui
    ui/test_form_ui.cpp
)
target_link_libraries(test_form_ui PRIVATE
    pdfinity_ui
    pdfinity_core
    Qt6::Test
    Qt6::Widgets
    pdfium
)
add_test(NAME FormUiTests COMMAND test_form_ui)

# Content Editing Tests

# Unit test: Content Editing Service (Layout Collision, Alignment, Font Analysis, Undo/Redo)
add_executable(test_content_editing_service
    unit/test_content_editing_service.cpp
)
target_link_libraries(test_content_editing_service PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME ContentEditingServiceTests COMMAND test_content_editing_service)

# Integration test: Content Editing Persistence (Native Stream Text, Shape, Image, Transform, Delete, Z-Order)
add_executable(test_content_editing_persistence
    integration/test_content_editing_persistence.cpp
)
target_link_libraries(test_content_editing_persistence PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME ContentEditingPersistenceTests COMMAND test_content_editing_persistence)

# UI test: Object Inspector Dialog, Content Toolbar, Layer Badges
add_executable(test_content_editing_ui
    ui/test_content_editing_ui.cpp
)
target_link_libraries(test_content_editing_ui PRIVATE
    pdfinity_ui
    pdfinity_core
    Qt6::Test
    Qt6::Widgets
    pdfium
)
add_test(NAME ContentEditingUiTests COMMAND test_content_editing_ui)

# Digital Signatures, PKCS#12, Verification, and Document Security Tests

# Unit test: Crypto Certificate Manager (PKCS#12, DPAPI Vault, Thumbprint, Sign/Verify)
add_executable(test_crypto_cert_manager
    unit/test_crypto_cert_manager.cpp
)
target_link_libraries(test_crypto_cert_manager PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME CryptoCertManagerTests COMMAND test_crypto_cert_manager)

# Unit test: Signature Service (Incremental Signing, Tamper Detection, Reports)
add_executable(test_signature_service
    unit/test_signature_service.cpp
)
target_link_libraries(test_signature_service PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME SignatureServiceTests COMMAND test_signature_service)

# Unit test: Encryption Service (Permissions Mask, Standard Encryption, Password Protection)
add_executable(test_encryption_service
    unit/test_encryption_service.cpp
)
target_link_libraries(test_encryption_service PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME EncryptionServiceTests COMMAND test_encryption_service)

# Integration test: Signature Persistence & Independent Validator (Visual Appearance vs Cryptographic, OpenSSL verification)
add_executable(test_signature_persistence
    integration/test_signature_persistence.cpp
)
target_link_libraries(test_signature_persistence PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME SignaturePersistenceTests COMMAND test_signature_persistence)

# UI test: Visual & Digital Signature Dialogs, Certificate Manager, Validation Dialog
add_executable(test_signature_ui
    ui/test_signature_ui.cpp
)
target_link_libraries(test_signature_ui PRIVATE
    pdfinity_ui
    pdfinity_core
    Qt6::Test
    Qt6::Widgets
    pdfium
)
add_test(NAME SignatureUiTests COMMAND test_signature_ui)

# Unit test - Redaction Service
add_executable(test_redaction_service
    unit/test_redaction_service.cpp
)
target_link_libraries(test_redaction_service PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME RedactionServiceTests COMMAND test_redaction_service)

# Unit test - Sanitization Service
add_executable(test_sanitization_service
    unit/test_sanitization_service.cpp
)
target_link_libraries(test_sanitization_service PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME SanitizationServiceTests COMMAND test_sanitization_service)

# Integration test - Multi-method Adversarial Redaction & Verification
add_executable(test_redaction_verification
    integration/test_redaction_verification.cpp
)
target_link_libraries(test_redaction_verification PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME RedactionVerificationTests COMMAND test_redaction_verification)

# UI test - Redaction Toolbar, Preview, Sanitization, and Report Dialogs
add_executable(test_redaction_ui
    ui/test_redaction_ui.cpp
)
target_link_libraries(test_redaction_ui PRIVATE
    pdfinity_ui
    pdfinity_core
    Qt6::Test
    Qt6::Widgets
    pdfium
)
add_test(NAME RedactionUiTests COMMAND test_redaction_ui)

# Unit test - OCR Service & Searchable PDF
add_executable(test_ocr_service
    unit/test_ocr_service.cpp
)
target_link_libraries(test_ocr_service PRIVATE
    pdfinity_core
    Qt6::Test
    Qt6::Gui
    pdfium
)
add_test(NAME OcrServiceTests COMMAND test_ocr_service)

# Unit test - Conversion Service (Image2Pdf, Pdf2Image, Pdf2Text, Pdf2Html, Office Detection)
add_executable(test_conversion_service
    unit/test_conversion_service.cpp
)
target_link_libraries(test_conversion_service PRIVATE
    pdfinity_core
    Qt6::Test
    Qt6::Gui
    pdfium
)
add_test(NAME ConversionServiceTests COMMAND test_conversion_service)

# Unit test - Optimization Service (Compression Presets, Downsampling, Validation)
add_executable(test_optimization_service
    unit/test_optimization_service.cpp
)
target_link_libraries(test_optimization_service PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME OptimizationServiceTests COMMAND test_optimization_service)

# Integration test - Transformation Pipeline & Batch Queue
add_executable(test_transformation_pipeline
    integration/test_transformation_pipeline.cpp
)
target_link_libraries(test_transformation_pipeline PRIVATE
    pdfinity_core
    Qt6::Test
    Qt6::Gui
    pdfium
)
add_test(NAME TransformationPipelineTests COMMAND test_transformation_pipeline)

# UI test - OCR Studio, Conversion Center, and Optimizer Dialogs
add_executable(test_transformation_ui
    ui/test_transformation_ui.cpp
)
target_link_libraries(test_transformation_ui PRIVATE
    pdfinity_ui
    pdfinity_core
    Qt6::Test
    Qt6::Widgets
    pdfium
)
add_test(NAME TransformationUiTests COMMAND test_transformation_ui)

# Unit test - Document Intelligence Service (Metadata, Fonts, Images, Boxes, Attachments)
add_executable(test_document_intelligence
    unit/test_document_intelligence.cpp
)
target_link_libraries(test_document_intelligence PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME DocumentIntelligenceTests COMMAND test_document_intelligence)

# Unit test - Comparison Service (Visual Diff, Text Diff, Full Comparison)
add_executable(test_comparison_service
    unit/test_comparison_service.cpp
)
target_link_libraries(test_comparison_service PRIVATE
    pdfinity_core
    Qt6::Test
    Qt6::Gui
    pdfium
)
add_test(NAME ComparisonServiceTests COMMAND test_comparison_service)

# Unit test - Preflight Inspection Service (Fonts, Images, Boxes, Encryption)
add_executable(test_preflight_service
    unit/test_preflight_service.cpp
)
target_link_libraries(test_preflight_service PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME PreflightServiceTests COMMAND test_preflight_service)

# Unit test - PDF/A Conformance Service (ISO 19005 Validation & Conversion)
add_executable(test_pdfa_compliance
    unit/test_pdfa_compliance.cpp
)
target_link_libraries(test_pdfa_compliance PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME PdfAComplianceTests COMMAND test_pdfa_compliance)

# Unit test - Accessibility & Tagged PDF Inspection
add_executable(test_accessibility_service
    unit/test_accessibility_service.cpp
)
target_link_libraries(test_accessibility_service PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME AccessibilityServiceTests COMMAND test_accessibility_service)

# UI test - Intelligence, Comparison Workspace, and Preflight Compliance Dialogs
add_executable(test_compliance_ui
    ui/test_compliance_ui.cpp
)
target_link_libraries(test_compliance_ui PRIVATE
    pdfinity_ui
    pdfinity_core
    Qt6::Test
    Qt6::Widgets
    pdfium
)
add_test(NAME ComplianceUiTests COMMAND test_compliance_ui)

# Unit test - Batch Profiles & Configuration Manager
add_executable(test_batch_profile
    unit/test_batch_profile.cpp
)
target_link_libraries(test_batch_profile PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME BatchProfileTests COMMAND test_batch_profile)

# Unit test - Batch Processing Service (Queue, Chains, Collisions, Resume, Reports)
add_executable(test_batch_service
    unit/test_batch_service.cpp
)
target_link_libraries(test_batch_service PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME BatchServiceTests COMMAND test_batch_service)

# Integration test - Batch Automation CLI Commands & Parameter Validation
add_executable(test_batch_cli
    integration/test_batch_cli.cpp
)
target_link_libraries(test_batch_cli PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME BatchCliTests COMMAND test_batch_cli)

# UI test - Batch Processing Center & Batch Profile Editor Dialogs
add_executable(test_batch_ui
    ui/test_batch_ui.cpp
)
target_link_libraries(test_batch_ui PRIVATE
    pdfinity_ui
    pdfinity_core
    Qt6::Test
    Qt6::Widgets
    pdfium
)
add_test(NAME BatchUiTests COMMAND test_batch_ui)

# Security test - Model Context Protocol (MCP) Permissions, Traversal, Boundaries & Redaction
add_executable(test_mcp_security
    security/test_mcp_security.cpp
)
target_link_libraries(test_mcp_security PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME McpSecurityTests COMMAND test_mcp_security)

# Unit test - Model Context Protocol (MCP) Server Protocol, Schemas & Tool Execution
add_executable(test_mcp_server
    unit/test_mcp_server.cpp
)
target_link_libraries(test_mcp_server PRIVATE
    pdfinity_core
    Qt6::Test
    pdfium
)
add_test(NAME McpServerTests COMMAND test_mcp_server)

# UI test - Model Context Protocol (MCP) Management & Consent Dialog
add_executable(test_mcp_ui
    ui/test_mcp_ui.cpp
)
target_link_libraries(test_mcp_ui PRIVATE
    pdfinity_ui
    pdfinity_core
    Qt6::Test
    Qt6::Widgets
    pdfium
)
add_test(NAME McpUiTests COMMAND test_mcp_ui)

# Security test - Remote Controller Pairing, Auth, Replay Defense, Rate Limits & Revocation
add_executable(test_remote_pair
    security/test_remote_pair.cpp
)
target_link_libraries(test_remote_pair PRIVATE
    pdfinity_core
    Qt6::Test
    Qt6::Network
    pdfium
)
add_test(NAME RemotePairTests COMMAND test_remote_pair)

# Unit test - Remote Controller Server Protocol, Navigation, Commands & Allowlist
add_executable(test_remote_server
    unit/test_remote_server.cpp
)
target_link_libraries(test_remote_server PRIVATE
    pdfinity_core
    Qt6::Test
    Qt6::Network
    pdfium
)
add_test(NAME RemoteServerTests COMMAND test_remote_server)

# UI test - Remote Controller Host Dialog, PIN UI, Emergency Disable
add_executable(test_remote_ui
    ui/test_remote_ui.cpp
)
target_link_libraries(test_remote_ui PRIVATE
    pdfinity_ui
    pdfinity_core
    Qt6::Test
    Qt6::Widgets
    Qt6::Network
    pdfium
)
add_test(NAME RemoteUiTests COMMAND test_remote_ui)

# Security & Fuzzing - Parser Boundaries, Malformed JSON, Hostile Paths, Parameter Fuzzing
add_executable(test_fuzz_boundaries
    security/test_fuzz_boundaries.cpp
)
target_link_libraries(test_fuzz_boundaries PRIVATE
    pdfinity_core
    Qt6::Test
    Qt6::Network
    pdfium
)
add_test(NAME FuzzBoundaryTests COMMAND test_fuzz_boundaries)
