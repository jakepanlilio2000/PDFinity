// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include "application/AppConfig.hpp"
#include "persistence/SettingsManager.hpp"
#include "persistence/RecentFilesManager.hpp"
#include <QTemporaryDir>
#include <QFile>

using namespace pdfinity::app;
using namespace pdfinity::persistence;

class TestConfigAndPersistence : public QObject {
    Q_OBJECT

private slots:
    void testAppConfigEnums();
    void testSettingsManagerSaveLoad();
    void testRecentFilesManager();
};

void TestConfigAndPersistence::testAppConfigEnums() {
    QCOMPARE(AppConfig::ThemeModeToString(AppThemeMode::System), "System");
    QCOMPARE(AppConfig::ThemeModeToString(AppThemeMode::Light), "Light");
    QCOMPARE(AppConfig::ThemeModeToString(AppThemeMode::Dark), "Dark");

    QCOMPARE(AppConfig::StringToThemeMode("System"), AppThemeMode::System);
    QCOMPARE(AppConfig::StringToThemeMode("Light"), AppThemeMode::Light);
    QCOMPARE(AppConfig::StringToThemeMode("Dark"), AppThemeMode::Dark);
    QCOMPARE(AppConfig::StringToThemeMode("Invalid"), AppThemeMode::System);
}

void TestConfigAndPersistence::testSettingsManagerSaveLoad() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    std::filesystem::path settingsPath = tempDir.path().toStdString() + "/test_settings.json";
    SettingsManager manager(settingsPath);

    AppConfig original;
    original.themeMode = AppThemeMode::Dark;
    original.windowWidth = 1440;
    original.windowHeight = 900;
    original.isMaximized = true;
    original.logLevel = "Debug";
    original.autosaveIntervalSeconds = 600;
    original.hardwareAcceleration = false;

    QVERIFY(manager.SaveSettings(original));
    QVERIFY(std::filesystem::exists(settingsPath));

    AppConfig loaded;
    QVERIFY(manager.LoadSettings(loaded));

    QCOMPARE(loaded.themeMode, AppThemeMode::Dark);
    QCOMPARE(loaded.windowWidth, 1440);
    QCOMPARE(loaded.windowHeight, 900);
    QCOMPARE(loaded.isMaximized, true);
    QCOMPARE(QString::fromStdString(loaded.logLevel), QString("Debug"));
    QCOMPARE(loaded.autosaveIntervalSeconds, 600);
    QCOMPARE(loaded.hardwareAcceleration, false);
}

void TestConfigAndPersistence::testRecentFilesManager() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Create real dummy test files
    QString file1 = tempDir.filePath("sample1.pdf");
    QString file2 = tempDir.filePath("sample2.pdf");
    QString file3 = tempDir.filePath("sample3.pdf");

    {
        QFile f1(file1); f1.open(QIODevice::WriteOnly); f1.write("%PDF-1.7\n%%EOF"); f1.close();
        QFile f2(file2); f2.open(QIODevice::WriteOnly); f2.write("%PDF-1.7\n%%EOF"); f2.close();
        QFile f3(file3); f3.open(QIODevice::WriteOnly); f3.write("%PDF-1.7\n%%EOF"); f3.close();
    }

    std::filesystem::path storagePath = tempDir.path().toStdString() + "/recents.json";
    RecentFilesManager manager(storagePath, 2); // Max 2 entries

    QVERIFY(manager.AddFile(file1.toStdString()));
    QVERIFY(manager.AddFile(file2.toStdString()));
    QCOMPARE(manager.GetRecentFiles().size(), static_cast<size_t>(2));

    // Adding file 3 should drop file 1 (since max is 2)
    QVERIFY(manager.AddFile(file3.toStdString()));
    auto items = manager.GetRecentFiles();
    QCOMPARE(items.size(), static_cast<size_t>(2));
    QCOMPARE(QString::fromStdString(items[0].fileName), QString("sample3.pdf"));
    QCOMPARE(QString::fromStdString(items[1].fileName), QString("sample2.pdf"));

    // Reload test
    RecentFilesManager reloaded(storagePath, 2);
    QVERIFY(reloaded.Load());
    QCOMPARE(reloaded.GetRecentFiles().size(), static_cast<size_t>(2));

    // Clear test
    reloaded.Clear();
    QCOMPARE(reloaded.GetRecentFiles().size(), static_cast<size_t>(0));
}

QTEST_MAIN(TestConfigAndPersistence)
#include "test_config.moc"
