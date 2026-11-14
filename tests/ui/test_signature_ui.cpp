// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QLabel>
#include <QTableWidget>
#include <QTabWidget>
#include "ui/VisualSignatureDialog.hpp"
#include "ui/DigitalSignatureDialog.hpp"
#include "ui/CertificateManagerDialog.hpp"
#include "ui/SignatureValidationDialog.hpp"
#include "ui/DocumentSecurityDialog.hpp"
#include "security/CryptoCertificateManager.hpp"

using namespace pdfinity;
using namespace pdfinity::ui;

class TestSignatureUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testVisualSignatureDialogLifecycle();
    void testCertificateManagerDialogLifecycle();
    void testDigitalSignatureDialogLifecycle();
    void testSignatureValidationDialogLifecycle();
    void testDocumentSecurityDialogLifecycle();

private:
    std::filesystem::path m_fixturesDir;
};

void TestSignatureUi::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    // Ensure test certificate is in vault
    auto pfxPath = m_fixturesDir / "test_signer.pfx";
    if (std::filesystem::exists(pfxPath)) {
        security::SecureString password("testpass123");
        security::CryptoCertificateManager::Instance().ImportPkcs12(
            pfxPath, password, "UI Test Signer"
        );
    }
}

void TestSignatureUi::testVisualSignatureDialogLifecycle() {
    VisualSignatureDialog dlg(0);
    QVERIFY(!dlg.windowTitle().isEmpty());

    // Verify 3 tabs exist: Draw, Type, Import
    auto* tabWidget = dlg.findChild<QTabWidget*>();
    QVERIFY(tabWidget != nullptr);
    QCOMPARE(tabWidget->count(), 3);

    // Verify visual warning banner is present
    bool foundBanner = false;
    for (auto* label : dlg.findChildren<QLabel*>()) {
        if (label->text().contains("VISUAL SIGNATURE ONLY")) {
            foundBanner = true;
            break;
        }
    }
    QVERIFY(foundBanner);

    auto sigData = dlg.GetSignatureData();
    // VERIFICATION TEST: Must NEVER be described or flagged as cryptographic
    QVERIFY(!sigData.isCryptographicBound);
}

void TestSignatureUi::testCertificateManagerDialogLifecycle() {
    CertificateManagerDialog dlg;
    QVERIFY(!dlg.windowTitle().isEmpty());

    auto* table = dlg.findChild<QTableWidget*>();
    QVERIFY(table != nullptr);
    QCOMPARE(table->columnCount(), 5);
}

void TestSignatureUi::testDigitalSignatureDialogLifecycle() {
    DigitalSignatureDialog dlg;
    QVERIFY(!dlg.windowTitle().isEmpty());

    // Verify cryptographic badge is present
    bool foundBadge = false;
    for (auto* label : dlg.findChildren<QLabel*>()) {
        if (label->text().contains("CRYPTOGRAPHIC DIGITAL SIGNATURE")) {
            foundBadge = true;
            break;
        }
    }
    QVERIFY(foundBadge);
}

void TestSignatureUi::testSignatureValidationDialogLifecycle() {
    std::filesystem::path dummyPath = "test_doc.pdf";

    // 1. Test Valid Result
    pdf_api::SignatureValidationResult validSig;
    validSig.hasValidDigest = true;
    validSig.isTampered = false;
    validSig.certificate.subject = "Test Valid Signer";
    validSig.certificate.issuer = "Test CA";
    validSig.certificate.validFrom = "2026-01-01";
    validSig.certificate.validTo = "2036-01-01";
    validSig.byteRange = {0, 100, 200, 300};

    SignatureValidationDialog validDlg(dummyPath, {validSig});
    bool foundValidBanner = false;
    for (auto* label : validDlg.findChildren<QLabel*>()) {
        if (label->text().contains("VALID SIGNATURES")) {
            foundValidBanner = true;
            break;
        }
    }
    QVERIFY(foundValidBanner);

    // 2. Test Tampered Result
    pdf_api::SignatureValidationResult tamperedSig;
    tamperedSig.hasValidDigest = false;
    tamperedSig.isTampered = true;
    tamperedSig.certificate.subject = "Tampered Signer";
    tamperedSig.byteRange = {0, 100, 200, 300};

    SignatureValidationDialog tamperedDlg(dummyPath, {tamperedSig});
    bool foundTamperedBanner = false;
    for (auto* label : tamperedDlg.findChildren<QLabel*>()) {
        if (label->text().contains("TAMPERING DETECTED")) {
            foundTamperedBanner = true;
            break;
        }
    }
    QVERIFY(foundTamperedBanner);
}

void TestSignatureUi::testDocumentSecurityDialogLifecycle() {
    DocumentSecurityDialog dlg;
    QVERIFY(!dlg.windowTitle().isEmpty());

    // Verify checkboxes exist
    auto checkboxes = dlg.findChildren<QCheckBox*>();
    QVERIFY(checkboxes.size() >= 5);
}

QTEST_MAIN(TestSignatureUi)
#include "test_signature_ui.moc"
