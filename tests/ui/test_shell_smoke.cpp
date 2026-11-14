// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QPushButton>
#include <QLabel>
#include <filesystem>
#include "ui/MainWindow.hpp"
#include "ui/PdfViewerWidget.hpp"
#include "ui/shell/LeftNavRail.hpp"
#include "ui/shell/RightInspectorPanel.hpp"
#include "ui/shell/WorkflowRibbon.hpp"
#include "ui/ThemeManager.hpp"
#include "ui/AboutDialog.hpp"
#include "ui/SettingsDialog.hpp"
#include "ui/UpdateDialog.hpp"
#include "ui/viewer/FloatingCanvasHud.hpp"
#include "ui/viewer/CanvasRulerOverlay.hpp"
#include "ui/viewer/StudioToast.hpp"
#include "application/AppContext.hpp"

using namespace pdfinity::ui;
using namespace pdfinity::app;

class TestShellSmoke : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testMainWindowCreation();
    void testOpenAndCloseDocumentTab();
    void testThemeSwitching();
    void testDialogs();
    void testWorkflowRibbonWiring();
    void testLeftNavRailPageSync();
    void testShellSplitterPersistence();
    void testRightInspectorSelectionSync();
    void testFloatingCanvasHudAndRulers();
};

void TestShellSmoke::initTestCase() {
    QVERIFY(AppContext::Instance().Initialize());
}

void TestShellSmoke::cleanupTestCase() {
    AppContext::Instance().Shutdown();
}

void TestShellSmoke::testMainWindowCreation() {
    MainWindow window;
    QVERIFY(!window.windowTitle().isEmpty());
    QVERIFY(window.centralWidget() != nullptr);
    QVERIFY(window.menuBar() != nullptr);
    QVERIFY(window.statusBar() != nullptr);
}

void TestShellSmoke::testOpenAndCloseDocumentTab() {
    MainWindow window;

    // Use authentic ordinary PDF fixture
    std::filesystem::path fixturePath = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures" / "ordinary.pdf";
    QVERIFY(std::filesystem::exists(fixturePath));

    // Open file
    QVERIFY(window.OpenFileByPath(QString::fromStdString(fixturePath.string())));

    // Close window cleanly
    window.close();
}

void TestShellSmoke::testThemeSwitching() {
    MainWindow window;

    // Test Light theme
    AppContext::Instance().GetConfig().themeMode = AppThemeMode::Light;
    ThemeManager::ApplyTheme(AppThemeMode::Light);

    // Test Dark theme
    AppContext::Instance().GetConfig().themeMode = AppThemeMode::Dark;
    ThemeManager::ApplyTheme(AppThemeMode::Dark);

    // Test System theme
    AppContext::Instance().GetConfig().themeMode = AppThemeMode::System;
    ThemeManager::ApplyTheme(AppThemeMode::System);
}

void TestShellSmoke::testDialogs() {
    MainWindow window;

    AboutDialog about(&window);
    QVERIFY(!about.windowTitle().isEmpty());

    SettingsDialog settings(&window);
    QVERIFY(!settings.windowTitle().isEmpty());

    UpdateDialog updateDialog(&window);
    QVERIFY(!updateDialog.windowTitle().isEmpty());
    auto* btnCheck = updateDialog.findChild<QPushButton*>("btnCheckNow");
    QVERIFY(btnCheck != nullptr);
    btnCheck->click();
    auto* statusLabel = updateDialog.findChild<QLabel*>("statusLabel");
    QVERIFY(statusLabel != nullptr);
    QVERIFY(statusLabel->text().contains("latest certified build"));
}

void TestShellSmoke::testWorkflowRibbonWiring() {
    MainWindow window;
    auto* ribbon = window.findChild<WorkflowRibbon*>();
    QVERIFY(ribbon != nullptr);

    ribbon->SetActiveDocumentAvailable(false);

    std::filesystem::path fixturePath = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures" / "ordinary.pdf";
    QVERIFY(std::filesystem::exists(fixturePath));
    QVERIFY(window.OpenFileByPath(QString::fromStdString(fixturePath.string())));

    // Emit Comment and View mode triggers to verify active wiring
    emit ribbon->CommentHighlightClicked();
    emit ribbon->CommentUnderlineClicked();
    emit ribbon->CommentStrikeoutClicked();
    emit ribbon->CommentNoteClicked();
    emit ribbon->ViewPanClicked();
    emit ribbon->ViewSelectClicked();

    window.close();
}

void TestShellSmoke::testLeftNavRailPageSync() {
    MainWindow window;
    std::filesystem::path fixturePath = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures" / "ordinary.pdf";
    QVERIFY(std::filesystem::exists(fixturePath));
    QVERIFY(window.OpenFileByPath(QString::fromStdString(fixturePath.string())));

    auto* viewer = window.findChild<PdfViewerWidget*>();
    QVERIFY(viewer != nullptr);

    auto* rail = viewer->GetNavigationRail();
    QVERIFY(rail != nullptr);
    QVERIFY(rail->IsDrawerExpanded());

    viewer->GoToPage(0);
    rail->SetActivePage(0);

    rail->SetDrawerExpanded(false);
    QVERIFY(!rail->IsDrawerExpanded());
    rail->SetDrawerExpanded(true);
    QVERIFY(rail->IsDrawerExpanded());

    window.close();
}

void TestShellSmoke::testShellSplitterPersistence() {
    MainWindow window;
    std::filesystem::path fixturePath = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures" / "ordinary.pdf";
    QVERIFY(std::filesystem::exists(fixturePath));
    QVERIFY(window.OpenFileByPath(QString::fromStdString(fixturePath.string())));

    auto* viewer = window.findChild<PdfViewerWidget*>();
    QVERIFY(viewer != nullptr);

    auto sizes = viewer->GetSplitterSizes();
    QVERIFY(!sizes.empty());

    auto& config = AppContext::Instance().GetConfig();
    config.leftRailDrawerExpanded = false;
    config.rightInspectorVisible = false;
    viewer->ApplySettingsState();

    auto* rail = viewer->GetNavigationRail();
    QVERIFY(rail != nullptr);
    QVERIFY(!rail->IsDrawerExpanded());

    auto* insp = viewer->GetInspector();
    QVERIFY(insp != nullptr);
    QVERIFY(!insp->isVisible());

    config.leftRailDrawerExpanded = true;
    config.rightInspectorVisible = true;
    viewer->ApplySettingsState();
    QVERIFY(rail->IsDrawerExpanded());
    QVERIFY(insp->isVisible());

    window.close();
}

void TestShellSmoke::testRightInspectorSelectionSync() {
    MainWindow window;
    std::filesystem::path fixturePath = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures" / "ordinary.pdf";
    QVERIFY(std::filesystem::exists(fixturePath));
    QVERIFY(window.OpenFileByPath(QString::fromStdString(fixturePath.string())));

    auto* viewer = window.findChild<PdfViewerWidget*>();
    QVERIFY(viewer != nullptr);

    auto* insp = viewer->GetInspector();
    QVERIFY(insp != nullptr);

    insp->SetAllToolsMode();

    SelectionPropertiesInfo info;
    info.objectType = "Highlight";
    info.x = 50.0;
    info.y = 100.0;
    info.width = 200.0;
    info.height = 15.0;
    info.strokeColor = QColor(220, 38, 38);
    info.strokeWidth = 3.0;
    info.opacity = 0.85;
    info.author = "Auditor";
    info.contents = "Important review finding";

    insp->SetSelectionProperties(info);
    insp->ClearSelection();

    window.close();
}

void TestShellSmoke::testFloatingCanvasHudAndRulers() {
    MainWindow window;
    std::filesystem::path fixturePath = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures" / "ordinary.pdf";
    QVERIFY(std::filesystem::exists(fixturePath));
    QVERIFY(window.OpenFileByPath(QString::fromStdString(fixturePath.string())));

    auto* viewer = window.findChild<PdfViewerWidget*>();
    QVERIFY(viewer != nullptr);

    auto* hud = viewer->GetCanvasHud();
    QVERIFY(hud != nullptr);

    // Verify pointer modes
    viewer->SetPointerMode(CanvasPointerMode::HandPan);
    QCOMPARE(viewer->GetPointerMode(), CanvasPointerMode::HandPan);
    viewer->SetPointerMode(CanvasPointerMode::Select);
    QCOMPARE(viewer->GetPointerMode(), CanvasPointerMode::Select);

    // Verify rulers and snap guides toggles
    viewer->SetRulersVisible(true);
    QVERIFY(viewer->AreRulersVisible());
    viewer->SetSnapGuidesVisible(true);
    QVERIFY(viewer->AreSnapGuidesVisible());
    viewer->SetRulersVisible(false);
    QVERIFY(!viewer->AreRulersVisible());

    // Verify HUD controls & wake-up
    hud->WakeUp();
    hud->UpdatePageInfo(0, 1);
    hud->UpdateZoom(1.25);
    hud->UpdateZoom(1.18); // Verify non-preset editable zoom [BUG-U4-01]

    // Verify closest snap guide logic [BUG-U4-02]
    CanvasRulerOverlay overlay;
    overlay.SetSnapGuidesVisible(true);
    overlay.AddGuide(true, 100.0);
    overlay.AddGuide(true, 105.0);
    QCOMPARE(overlay.Snap(104.0, true, 5.0), 105.0);

    // Verify StudioToast creation & auto-dismiss
    StudioToast::Show(&window, "HUD Verified", "Elevated viewport HUD test passed.", ToastType::Success, 500);

    window.close();
}

QTEST_MAIN(TestShellSmoke)
#include "test_shell_smoke.moc"
