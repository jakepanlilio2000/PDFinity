// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/DocumentModel.hpp"
#include "pdf_api/PdfTypes.hpp"
#include <QObject>
#include <memory>
#include <vector>
#include <deque>
#include <string>
#include <algorithm>

namespace pdfinity::services {

enum class AlignmentType {
    Left,
    Center,
    Right,
    Top,
    Middle,
    Bottom
};

class IContentEditCommand {
public:
    virtual ~IContentEditCommand() = default;
    virtual bool Execute() = 0;
    virtual bool Undo() = 0;
    virtual std::string GetDescription() const = 0;
};

class ContentEditingService : public QObject {
    Q_OBJECT

public:
    explicit ContentEditingService(QObject* parent = nullptr);
    ~ContentEditingService() override = default;

    // Command Pattern & Undo/Redo Engine
    bool ExecuteCommand(std::unique_ptr<IContentEditCommand> command);
    bool Undo();
    bool Redo();
    void ClearHistory();
    [[nodiscard]] bool CanUndo() const;
    [[nodiscard]] bool CanRedo() const;
    [[nodiscard]] std::string GetUndoDescription() const;
    [[nodiscard]] std::string GetRedoDescription() const;

    // Selection Management
    void SetSelectedIndices(const std::vector<int>& indices);
    [[nodiscard]] const std::vector<int>& GetSelectedIndices() const;
    void ClearSelection();
    [[nodiscard]] bool IsSelected(int index) const;

    // Clipboard (Copy & Paste for Page Objects)
    void CopyObjects(const std::vector<pdf_api::PageObjectData>& objects);
    [[nodiscard]] bool CanPaste() const;
    [[nodiscard]] const std::vector<pdf_api::PageObjectData>& GetClipboard() const;
    std::vector<int> PasteObjects(
        domain::DocumentModel& doc,
        int targetPageIndex,
        double offsetDeltaX = 20.0,
        double offsetDeltaY = -20.0
    );

    // Layout Alignment Calculations
    // Returns pair of (dx, dy) translation deltas for each object in order
    static std::vector<std::pair<double, double>> CalculateAlignment(
        AlignmentType type,
        const std::vector<pdf_api::PageObjectData>& objects
    );

    // Layout Safety & Overlap Collision Detection
    static pdf_api::LayoutCollisionWarning ValidateLayoutSafety(
        const pdf_api::PageObjectData& targetObject,
        const std::string& newText,
        double newFontSize,
        const std::vector<pdf_api::PageObjectData>& allPageObjects,
        double pageWidth = 612.0,
        double pageHeight = 792.0
    );

    // Font Capability & Subset Analysis
    static std::string AnalyzeFontCapabilities(const pdf_api::PageObjectData& object);

    // Edit Layer Distinction & Reporting
    static QString GetLayerDescription(pdf_api::ContentEditMode mode);

signals:
    void CanUndoChanged(bool canUndo);
    void CanRedoChanged(bool canRedo);
    void CommandExecuted();
    void ContentModified();
    void SelectionChanged();

private:
    std::deque<std::unique_ptr<IContentEditCommand>> m_undoStack;
    std::deque<std::unique_ptr<IContentEditCommand>> m_redoStack;
    static constexpr size_t kMaxHistorySteps = 100;

    std::vector<int> m_selectedIndices;
    std::vector<pdf_api::PageObjectData> m_clipboard;
};

// Concrete Command: Insert Text Object
class InsertTextCommand : public IContentEditCommand {
public:
    InsertTextCommand(
        std::shared_ptr<domain::DocumentModel> doc,
        int pageIndex,
        std::string text,
        double x,
        double y,
        std::string fontName,
        double fontSize,
        pdf_api::PdfColor color
    );
    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override;

private:
    std::shared_ptr<domain::DocumentModel> m_doc;
    int m_pageIndex;
    std::string m_text;
    double m_x;
    double m_y;
    std::string m_fontName;
    double m_fontSize;
    pdf_api::PdfColor m_color;
    int m_insertedIndex{-1};
};

// Concrete Command: Insert Shape Object
class InsertShapeCommand : public IContentEditCommand {
public:
    InsertShapeCommand(
        std::shared_ptr<domain::DocumentModel> doc,
        int pageIndex,
        pdf_api::PdfRect rect,
        pdf_api::PdfColor strokeColor,
        pdf_api::PdfColor fillColor,
        double strokeWidth
    );
    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override;

private:
    std::shared_ptr<domain::DocumentModel> m_doc;
    int m_pageIndex;
    pdf_api::PdfRect m_rect;
    pdf_api::PdfColor m_strokeColor;
    pdf_api::PdfColor m_fillColor;
    double m_strokeWidth;
    int m_insertedIndex{-1};
};

// Concrete Command: Delete Objects
class DeleteObjectsCommand : public IContentEditCommand {
public:
    DeleteObjectsCommand(
        std::shared_ptr<domain::DocumentModel> doc,
        int pageIndex,
        std::vector<pdf_api::PageObjectData> objectsToDelete
    );
    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override;

private:
    std::shared_ptr<domain::DocumentModel> m_doc;
    int m_pageIndex;
    std::vector<pdf_api::PageObjectData> m_objects;
};

// Concrete Command: Transform Objects (Translation and Scaling)
class TransformObjectsCommand : public IContentEditCommand {
public:
    TransformObjectsCommand(
        std::shared_ptr<domain::DocumentModel> doc,
        int pageIndex,
        std::vector<int> objectIndices,
        double dx,
        double dy,
        double scaleX = 1.0,
        double scaleY = 1.0
    );
    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override;

private:
    std::shared_ptr<domain::DocumentModel> m_doc;
    int m_pageIndex;
    std::vector<int> m_indices;
    double m_dx;
    double m_dy;
    double m_scaleX;
    double m_scaleY;
};

// Concrete Command: Modify Text Object
class ModifyTextCommand : public IContentEditCommand {
public:
    ModifyTextCommand(
        std::shared_ptr<domain::DocumentModel> doc,
        int pageIndex,
        int objectIndex,
        std::string oldText,
        double oldFontSize,
        pdf_api::PdfColor oldColor,
        std::string newText,
        double newFontSize,
        pdf_api::PdfColor newColor
    );
    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override;

private:
    std::shared_ptr<domain::DocumentModel> m_doc;
    int m_pageIndex;
    int m_objectIndex;
    std::string m_oldText;
    double m_oldFontSize;
    pdf_api::PdfColor m_oldColor;
    std::string m_newText;
    double m_newFontSize;
    pdf_api::PdfColor m_newColor;
};

// Concrete Command: Reorder Object (Z-Order)
class ReorderObjectCommand : public IContentEditCommand {
public:
    ReorderObjectCommand(
        std::shared_ptr<domain::DocumentModel> doc,
        int pageIndex,
        int fromIndex,
        int toIndex
    );
    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override;

private:
    std::shared_ptr<domain::DocumentModel> m_doc;
    int m_pageIndex;
    int m_fromIndex;
    int m_toIndex;
};

} // namespace pdfinity::services
