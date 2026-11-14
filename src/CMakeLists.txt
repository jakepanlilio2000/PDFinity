# pdfinity_core library
add_library(pdfinity_core STATIC
    platform/IPlatformAdapter.hpp
    platform/WindowsPlatformAdapter.hpp
    platform/WindowsPlatformAdapter.cpp
    platform/PlatformFactory.hpp
    platform/PlatformFactory.cpp
    security/SecureString.hpp
    security/SecureString.cpp
    security/PathSanitizer.hpp
    security/PathSanitizer.cpp
    security/LogRedactor.hpp
    security/LogRedactor.cpp
    application/Logger.hpp
    application/Logger.cpp
    application/AppConfig.hpp
    application/AppConfig.cpp
    application/AppContext.hpp
    application/AppContext.cpp
    persistence/SettingsManager.hpp
    persistence/SettingsManager.cpp
    persistence/RecentFilesManager.hpp
    persistence/RecentFilesManager.cpp
    pdf_api/PdfTypes.hpp
    pdf_api/PdfTypes.cpp
    pdf_api/IPdfEngine.hpp
    domain/DocumentModel.hpp
    domain/DocumentModel.cpp
    domain/PageRangeParser.hpp
    domain/PageRangeParser.cpp
    pdf_adapters/PdfiumAdapter.hpp
    pdf_adapters/PdfiumAdapter.cpp
    services/DocumentLifecycleService.hpp
    services/DocumentLifecycleService.cpp
    services/DocumentAssemblyService.hpp
    services/DocumentAssemblyService.cpp
    services/AnnotationCommandManager.hpp
    services/AnnotationCommandManager.cpp
    services/AnnotationService.hpp
    services/AnnotationService.cpp
    services/FormService.hpp
    services/FormService.cpp
    services/ContentEditingService.hpp
    services/ContentEditingService.cpp
    services/PageCache.hpp
    services/PageCache.cpp
    services/AsyncRenderService.hpp
    services/AsyncRenderService.cpp
    services/DocumentSearchService.hpp
    services/DocumentSearchService.cpp
    security/CryptoCertificateManager.hpp
    security/CryptoCertificateManager.cpp
    services/SignatureService.hpp
    services/SignatureService.cpp
    services/DocumentEncryptionService.hpp
    services/DocumentEncryptionService.cpp
    services/SanitizationService.hpp
    services/SanitizationService.cpp
    services/RedactionService.hpp
    services/RedactionService.cpp
    services/OcrService.hpp
    services/OcrService.cpp
    services/ConversionService.hpp
    services/ConversionService.cpp
    services/OptimizationService.hpp
    services/OptimizationService.cpp
    services/TransformationJobQueue.hpp
    services/TransformationJobQueue.cpp
    services/DocumentIntelligenceService.hpp
    services/DocumentIntelligenceService.cpp
    services/ComparisonService.hpp
    services/ComparisonService.cpp
    services/PreflightService.hpp
    services/PreflightService.cpp
    services/PdfAComplianceService.hpp
    services/PdfAComplianceService.cpp
    services/AccessibilityService.hpp
    services/AccessibilityService.cpp
    domain/BatchTypes.hpp
    services/BatchProfileManager.hpp
    services/BatchProfileManager.cpp
    services/BatchJobHistory.hpp
    services/BatchJobHistory.cpp
    services/BatchProcessingService.hpp
    services/BatchProcessingService.cpp
    app/BatchCliHandler.hpp
    app/BatchCliHandler.cpp
    mcp/McpTypes.hpp
    mcp/McpSecurityPolicy.hpp
    mcp/McpSecurityPolicy.cpp
    mcp/McpToolCatalog.hpp
    mcp/McpToolCatalog.cpp
    mcp/McpServer.hpp
    mcp/McpServer.cpp
    mcp/McpStdioTransport.hpp
    mcp/McpStdioTransport.cpp
    app/McpCliHandler.hpp
    app/McpCliHandler.cpp
    remote/RemoteTypes.hpp
    remote/RemotePairingManager.hpp
    remote/RemotePairingManager.cpp
    remote/RemoteSecurityPolicy.hpp
    remote/RemoteSecurityPolicy.cpp
    remote/RemoteCommandDispatcher.hpp
    remote/RemoteCommandDispatcher.cpp
    remote/RemoteControllerServer.hpp
    remote/RemoteControllerServer.cpp
    remote/RemoteControllerClient.hpp
    remote/RemoteControllerClient.cpp
    app/RemoteCliHandler.hpp
    app/RemoteCliHandler.cpp
)

if(ANDROID)
    target_sources(pdfinity_core PRIVATE
        platform/AndroidPlatformAdapter.hpp
        platform/AndroidPlatformAdapter.cpp
    )
endif()

target_include_directories(pdfinity_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(pdfinity_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    pdfium
)

if(WIN32)
    target_link_libraries(pdfinity_core PUBLIC
        Shell32
        Ole32
        User32
        Advapi32
        Gdi32
        Crypt32
        Bcrypt
        Ncrypt
    )

    # Copy pdfium.dll to output directory for runtime execution
    add_custom_command(TARGET pdfinity_core POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/third_party/pdfium/bin/pdfium.dll"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/pdfium.dll"
    )
endif()

# pdfinity_ui library
add_library(pdfinity_ui STATIC
    services/PrintService.hpp
    services/PrintService.cpp
    ui/ThemeManager.hpp
    ui/ThemeManager.cpp
    ui/theme/ThemeTokens.hpp
    ui/theme/ThemeTokens.cpp
    ui/AboutDialog.hpp
    ui/AboutDialog.cpp
    ui/SettingsDialog.hpp
    ui/SettingsDialog.cpp
    ui/PdfCanvasWidget.hpp
    ui/PdfCanvasWidget.cpp
    ui/NavigationSidebar.hpp
    ui/NavigationSidebar.cpp
    ui/PdfSearchBar.hpp
    ui/PdfSearchBar.cpp
    ui/PdfViewerWidget.hpp
    ui/PdfViewerWidget.cpp
    ui/DocumentWorkspace.hpp
    ui/DocumentWorkspace.cpp
    ui/PageOrganizerDialog.hpp
    ui/PageOrganizerDialog.cpp
    ui/MergeDocumentsDialog.hpp
    ui/MergeDocumentsDialog.cpp
    ui/SplitDocumentDialog.hpp
    ui/SplitDocumentDialog.cpp
    ui/AnnotationPropertiesDialog.hpp
    ui/AnnotationPropertiesDialog.cpp
    ui/AnnotationToolbar.hpp
    ui/AnnotationToolbar.cpp
    ui/XfaNoticeBanner.hpp
    ui/XfaNoticeBanner.cpp
    ui/FormInspectorDialog.hpp
    ui/FormInspectorDialog.cpp
    ui/ObjectInspectorDialog.hpp
    ui/ObjectInspectorDialog.cpp
    ui/ContentEditingToolbar.hpp
    ui/ContentEditingToolbar.cpp
    ui/VisualSignatureDialog.hpp
    ui/VisualSignatureDialog.cpp
    ui/DigitalSignatureDialog.hpp
    ui/DigitalSignatureDialog.cpp
    ui/CertificateManagerDialog.hpp
    ui/CertificateManagerDialog.cpp
    ui/SignatureValidationDialog.hpp
    ui/SignatureValidationDialog.cpp
    ui/DocumentSecurityDialog.hpp
    ui/DocumentSecurityDialog.cpp
    ui/RedactionToolbar.hpp
    ui/RedactionToolbar.cpp
    ui/RedactionPreviewDialog.hpp
    ui/RedactionPreviewDialog.cpp
    ui/SanitizationDialog.hpp
    ui/SanitizationDialog.cpp
    ui/RedactionReportDialog.hpp
    ui/RedactionReportDialog.cpp
    ui/OcrStudioDialog.hpp
    ui/OcrStudioDialog.cpp
    ui/ConversionCenterDialog.hpp
    ui/ConversionCenterDialog.cpp
    ui/OptimizerDialog.hpp
    ui/OptimizerDialog.cpp
    ui/DocumentIntelligenceDialog.hpp
    ui/DocumentIntelligenceDialog.cpp
    ui/ComparisonWorkspaceDialog.hpp
    ui/ComparisonWorkspaceDialog.cpp
    ui/PreflightComplianceDialog.hpp
    ui/PreflightComplianceDialog.cpp
    ui/BatchCenterDialog.hpp
    ui/BatchCenterDialog.cpp
    ui/BatchProfileEditorDialog.hpp
    ui/BatchProfileEditorDialog.cpp
    ui/McpConfigDialog.hpp
    ui/McpConfigDialog.cpp
    ui/RemoteConfigDialog.hpp
    ui/RemoteConfigDialog.cpp
    ui/UpdateDialog.hpp
    ui/UpdateDialog.cpp
    ui/VectorIcons.hpp
    ui/VectorIcons.cpp
    ui/FloatingHudWidget.hpp
    ui/FloatingHudWidget.cpp
    ui/HomeDashboardWidget.hpp
    ui/HomeDashboardWidget.cpp
    ui/shell/StudioTabBar.hpp
    ui/shell/StudioTabBar.cpp
    ui/shell/WorkflowRibbon.hpp
    ui/shell/WorkflowRibbon.cpp
    ui/shell/LeftNavRail.hpp
    ui/shell/LeftNavRail.cpp
    ui/shell/RightInspectorPanel.hpp
    ui/shell/RightInspectorPanel.cpp
    ui/MainWindow.hpp
    ui/MainWindow.cpp
)

target_include_directories(pdfinity_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(pdfinity_ui PUBLIC
    pdfinity_core
    Qt6::Widgets
    Qt6::PrintSupport
)

# PDFinity main desktop executable (Silent GUI launch: zero terminal console)
set(PDFINITY_MAIN_SOURCES app/main.cpp)
if(WIN32)
    list(APPEND PDFINITY_MAIN_SOURCES ${PROJECT_SOURCE_DIR}/packaging/windows/version.rc)
endif()

add_executable(PDFinity
    ${PDFINITY_MAIN_SOURCES}
)

target_link_libraries(PDFinity PRIVATE
    pdfinity_ui
    pdfinity_core
    Qt6::Widgets
)

if(WIN32)
    set_target_properties(PDFinity PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    if(MSVC)
        target_link_options(PDFinity PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
    elseif(MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_link_options(PDFinity PRIVATE -mwindows)
    endif()
endif()

if(APPLE)
    set_target_properties(PDFinity PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/packaging/macos/bundle.plist"
    )
endif()

# PDFinity companion console executable
set(PDFINITY_CLI_SOURCES app/cli_main.cpp)
if(WIN32)
    list(APPEND PDFINITY_CLI_SOURCES ${PROJECT_SOURCE_DIR}/packaging/windows/version.rc)
endif()

add_executable(pdfinity-cli
    ${PDFINITY_CLI_SOURCES}
)

target_link_libraries(pdfinity-cli PRIVATE
    pdfinity_ui
    pdfinity_core
    Qt6::Widgets
)

set_target_properties(pdfinity-cli PROPERTIES
    WIN32_EXECUTABLE FALSE
    OUTPUT_NAME "pdfinity-cli"
)

if(WIN32 AND MSVC)
    target_link_options(pdfinity-cli PRIVATE /SUBSYSTEM:CONSOLE)
endif()

# Installation rules
install(TARGETS PDFinity pdfinity-cli pdfinity_ui pdfinity_core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    BUNDLE DESTINATION .
)

if(UNIX AND NOT APPLE AND NOT ANDROID)
    # Desktop entries
    install(FILES "${PROJECT_SOURCE_DIR}/packaging/linux/com.stacksyncsolutions.pdfinity.desktop"
        DESTINATION share/applications
    )
    install(FILES "${PROJECT_SOURCE_DIR}/packaging/linux/com.stacksyncsolutions.pdfinity.desktop"
        DESTINATION share/applications
        RENAME pdfinity.desktop
    )

    # AppStream Metainfo
    install(FILES "${PROJECT_SOURCE_DIR}/packaging/linux/com.stacksyncsolutions.pdfinity.metainfo.xml"
        DESTINATION share/metainfo
    )

    # Icons
    install(FILES "${PROJECT_SOURCE_DIR}/packaging/resources/pdfinity.svg"
        DESTINATION share/icons/hicolor/scalable/apps
        RENAME com.stacksyncsolutions.pdfinity.svg
    )
    install(FILES "${PROJECT_SOURCE_DIR}/packaging/resources/pdfinity.svg"
        DESTINATION share/icons/hicolor/scalable/apps
        RENAME pdfinity.svg
    )

    # Licenses and docs
    install(FILES "${PROJECT_SOURCE_DIR}/LICENSE"
        DESTINATION share/licenses/pdfinity
    )
    install(FILES "${PROJECT_SOURCE_DIR}/README.md" "${PROJECT_SOURCE_DIR}/THIRD_PARTY_LICENSES.md" "${PROJECT_SOURCE_DIR}/NOTICE"
        DESTINATION share/doc/pdfinity
    )
endif()
