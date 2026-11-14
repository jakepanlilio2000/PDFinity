// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/DocumentModel.hpp"
#include "services/DocumentAssemblyService.hpp"
#include <QDialog>
#include <QPixmap>
#include <vector>
#include <filesystem>
#include <memory>

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QLabel;
class QLineEdit;

namespace pdfinity::ui {

struct OrganizerPageItem {
    int originalPageIndex{-1}; // -1 if blank
    int rotationDegrees{0};    // 0, 90, 180, 270
    double width{595.0};
    double height{842.0};
    bool isBlank{false};
    QPixmap baseThumbnail;
};

class PageOrganizerDialog : public QDialog {
    Q_OBJECT
public:
    explicit PageOrganizerDialog(
        std::shared_ptr<pdf_api::IPdfEngine> engine,
        const std::filesystem::path& docPath,
        QWidget* parent = nullptr
    );
    ~PageOrganizerDialog() override = default;

    [[nodiscard]] const std::filesystem::path& GetResultOutputPath() const noexcept {
        return m_savedOutputPath;
    }

private slots:
    void OnRotateCw();
    void OnRotateCcw();
    void OnDeletePages();
    void OnDuplicatePages();
    void OnInsertBlankPage();
    void OnMoveLeft();
    void OnMoveRight();
    void OnReverseOrder();
    void OnSelectAll();
    void OnSelectNone();
    void OnSelectOdd();
    void OnSelectEven();
    void OnSelectByRange();
    void OnExtractSelected();
    void OnApplySaveAs();
    void OnItemSelectionChanged();

private:
    void SetupUi();
    void LoadDocumentPages();
    void RefreshListItems();
    void UpdateSelectionStatus();
    QPixmap RenderThumbnail(int pageIndex, int rotation);
    QPixmap CreateBlankThumbnail(int rotation);

    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
    std::filesystem::path m_docPath;
    std::filesystem::path m_savedOutputPath;
    std::unique_ptr<domain::DocumentModel> m_docModel;

    std::vector<OrganizerPageItem> m_pages;

    QListWidget* m_pageListWidget{nullptr};
    QLabel* m_statusLabel{nullptr};

    QPushButton* m_rotateCwBtn{nullptr};
    QPushButton* m_rotateCcwBtn{nullptr};
    QPushButton* m_deleteBtn{nullptr};
    QPushButton* m_duplicateBtn{nullptr};
    QPushButton* m_insertBlankBtn{nullptr};
    QPushButton* m_moveLeftBtn{nullptr};
    QPushButton* m_moveRightBtn{nullptr};
    QPushButton* m_reverseBtn{nullptr};
    QPushButton* m_extractBtn{nullptr};
    QPushButton* m_saveAsBtn{nullptr};
};

} // namespace pdfinity::ui
