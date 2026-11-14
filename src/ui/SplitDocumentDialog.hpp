// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "services/DocumentAssemblyService.hpp"
#include <QDialog>
#include <filesystem>
#include <memory>
#include <vector>

class QRadioButton;
class QSpinBox;
class QLineEdit;
class QPushButton;
class QLabel;

namespace pdfinity::ui {

class SplitDocumentDialog : public QDialog {
    Q_OBJECT
public:
    explicit SplitDocumentDialog(
        std::shared_ptr<pdf_api::IPdfEngine> engine,
        const std::filesystem::path& docPath = {},
        QWidget* parent = nullptr
    );
    ~SplitDocumentDialog() override = default;

    [[nodiscard]] const std::vector<std::filesystem::path>& GetGeneratedFiles() const noexcept {
        return m_generatedFiles;
    }

private slots:
    void OnBrowseSource();
    void OnBrowseOutputFolder();
    void OnSplitModeChanged();
    void OnSplitClicked();

private:
    void SetupUi();
    void UpdateSourceDocInfo();

    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
    std::unique_ptr<services::DocumentAssemblyService> m_assemblyService;
    std::filesystem::path m_sourcePath;
    int m_totalPages{0};
    bool m_hasBookmarks{false};
    std::vector<std::filesystem::path> m_generatedFiles;

    QLineEdit* m_sourcePathEdit{nullptr};
    QLabel* m_sourceInfoLabel{nullptr};

    QRadioButton* m_burstRadio{nullptr};
    QRadioButton* m_everyNRadio{nullptr};
    QSpinBox* m_pagesPerChunkSpin{nullptr};
    QRadioButton* m_customRangesRadio{nullptr};
    QLineEdit* m_customRangesEdit{nullptr};
    QRadioButton* m_bookmarksRadio{nullptr};

    QLineEdit* m_outputDirEdit{nullptr};
    QLineEdit* m_prefixEdit{nullptr};
    QPushButton* m_splitBtn{nullptr};
};

} // namespace pdfinity::ui
