// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "services/DocumentAssemblyService.hpp"
#include <QDialog>
#include <filesystem>
#include <memory>
#include <vector>
#include <algorithm>

class QTableWidget;
class QPushButton;
class QLineEdit;

namespace pdfinity::ui {

class MergeDocumentsDialog : public QDialog {
    Q_OBJECT
public:
    explicit MergeDocumentsDialog(
        std::shared_ptr<pdf_api::IPdfEngine> engine,
        QWidget* parent = nullptr
    );
    ~MergeDocumentsDialog() override = default;

    [[nodiscard]] const std::filesystem::path& GetResultOutputPath() const noexcept {
        return m_savedOutputPath;
    }

private slots:
    void OnAddFiles();
    void OnRemoveSelected();
    void OnMoveUp();
    void OnMoveDown();
    void OnClearAll();
    void OnBrowseOutput();
    void OnMergeClicked();
    void OnSelectionChanged();

private:
    void SetupUi();
    void UpdateControlsState();
    int GetPdfPageCount(const std::filesystem::path& path);

    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
    std::unique_ptr<services::DocumentAssemblyService> m_assemblyService;
    std::filesystem::path m_savedOutputPath;

    QTableWidget* m_fileTable{nullptr};
    QLineEdit* m_outputPathEdit{nullptr};
    QPushButton* m_removeBtn{nullptr};
    QPushButton* m_moveUpBtn{nullptr};
    QPushButton* m_moveDownBtn{nullptr};
    QPushButton* m_mergeBtn{nullptr};
};

} // namespace pdfinity::ui
