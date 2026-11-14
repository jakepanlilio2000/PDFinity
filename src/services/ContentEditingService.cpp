// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "ContentEditingService.hpp"
#include <algorithm>
#include <cmath>

namespace pdfinity::services {

ContentEditingService::ContentEditingService(QObject* parent) : QObject(parent) {}

bool ContentEditingService::ExecuteCommand(std::unique_ptr<IContentEditCommand> command) {
    if (!command) return false;

    if (command->Execute()) {
        m_undoStack.push_back(std::move(command));
        if (m_undoStack.size() > kMaxHistorySteps) {
            m_undoStack.pop_front();
        }
        m_redoStack.clear();

        emit CanUndoChanged(true);
        emit CanRedoChanged(false);
        emit CommandExecuted();
        emit ContentModified();
        return true;
    }
    return false;
}

bool ContentEditingService::Undo() {
    if (m_undoStack.empty()) return false;

    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    if (cmd->Undo()) {
        m_redoStack.push_back(std::move(cmd));
        emit CanUndoChanged(!m_undoStack.empty());
        emit CanRedoChanged(true);
        emit CommandExecuted();
        emit ContentModified();
        return true;
    }
    return false;
}

bool ContentEditingService::Redo() {
    if (m_redoStack.empty()) return false;

    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    if (cmd->Execute()) {
        m_undoStack.push_back(std::move(cmd));
        emit CanUndoChanged(true);
        emit CanRedoChanged(!m_redoStack.empty());
        emit CommandExecuted();
        emit ContentModified();
        return true;
    }
    return false;
}

void ContentEditingService::ClearHistory() {
    m_undoStack.clear();
    m_redoStack.clear();
    emit CanUndoChanged(false);
    emit CanRedoChanged(false);
}

bool ContentEditingService::CanUndo() const {
    return !m_undoStack.empty();
}

bool ContentEditingService::CanRedo() const {
    return !m_redoStack.empty();
}

std::string ContentEditingService::GetUndoDescription() const {
    if (m_undoStack.empty()) return "";
    return m_undoStack.back()->GetDescription();
}

std::string ContentEditingService::GetRedoDescription() const {
    if (m_redoStack.empty()) return "";
    return m_redoStack.back()->GetDescription();
}

void ContentEditingService::SetSelectedIndices(const std::vector<int>& indices) {
    m_selectedIndices = indices;
    emit SelectionChanged();
}

const std::vector<int>& ContentEditingService::GetSelectedIndices() const {
    return m_selectedIndices;
}

void ContentEditingService::ClearSelection() {
    m_selectedIndices.clear();
    emit SelectionChanged();
}

bool ContentEditingService::IsSelected(int index) const {
    return std::find(m_selectedIndices.begin(), m_selectedIndices.end(), index) != m_selectedIndices.end();
}

void ContentEditingService::CopyObjects(const std::vector<pdf_api::PageObjectData>& objects) {
    m_clipboard = objects;
}

bool ContentEditingService::CanPaste() const {
    return !m_clipboard.empty();
}

const std::vector<pdf_api::PageObjectData>& ContentEditingService::GetClipboard() const {
    return m_clipboard;
}

std::vector<int> ContentEditingService::PasteObjects(
    domain::DocumentModel& doc,
    int targetPageIndex,
    double offsetDeltaX,
    double offsetDeltaY
) {
    std::vector<int> pastedIndices;
    if (m_clipboard.empty() || !doc.IsValid()) return pastedIndices;

    for (const auto& obj : m_clipboard) {
        if (obj.type == pdf_api::PageObjectType::Text) {
            auto res = doc.InsertPageText(
                targetPageIndex,
                obj.text,
                obj.bounds.x + offsetDeltaX,
                obj.bounds.y + offsetDeltaY,
                obj.fontName.empty() ? "Helvetica" : obj.fontName,
                obj.fontSize,
                obj.fillColor
            );
            if (res.has_value()) {
                pastedIndices.push_back(res.value);
            }
        } else if (obj.type == pdf_api::PageObjectType::Path) {
            pdf_api::PdfRect newRect = obj.bounds;
            newRect.x += offsetDeltaX;
            newRect.y += offsetDeltaY;
            auto res = doc.InsertPageShape(
                targetPageIndex,
                newRect,
                obj.strokeColor,
                obj.fillColor,
                obj.strokeWidth
            );
            if (res.has_value()) {
                pastedIndices.push_back(res.value);
            }
        }
    }

    emit ContentModified();
    return pastedIndices;
}

std::vector<std::pair<double, double>> ContentEditingService::CalculateAlignment(
    AlignmentType type,
    const std::vector<pdf_api::PageObjectData>& objects
) {
    std::vector<std::pair<double, double>> deltas;
    if (objects.size() < 2) {
        deltas.resize(objects.size(), {0.0, 0.0});
        return deltas;
    }

    // Determine reference coordinate
    double minLeft = objects[0].bounds.x;
    double maxRight = objects[0].bounds.x + objects[0].bounds.width;
    double minBottom = objects[0].bounds.y;
    double maxTop = objects[0].bounds.y + objects[0].bounds.height;

    for (const auto& obj : objects) {
        minLeft = std::min(minLeft, obj.bounds.x);
        maxRight = std::max(maxRight, obj.bounds.x + obj.bounds.width);
        minBottom = std::min(minBottom, obj.bounds.y);
        maxTop = std::max(maxTop, obj.bounds.y + obj.bounds.height);
    }
    double centerX = (minLeft + maxRight) / 2.0;
    double middleY = (minBottom + maxTop) / 2.0;

    deltas.reserve(objects.size());
    for (const auto& obj : objects) {
        double dx = 0.0;
        double dy = 0.0;

        switch (type) {
            case AlignmentType::Left:
                dx = minLeft - obj.bounds.x;
                break;
            case AlignmentType::Center:
                dx = centerX - (obj.bounds.x + obj.bounds.width / 2.0);
                break;
            case AlignmentType::Right:
                dx = maxRight - (obj.bounds.x + obj.bounds.width);
                break;
            case AlignmentType::Top:
                dy = maxTop - (obj.bounds.y + obj.bounds.height);
                break;
            case AlignmentType::Middle:
                dy = middleY - (obj.bounds.y + obj.bounds.height / 2.0);
                break;
            case AlignmentType::Bottom:
                dy = minBottom - obj.bounds.y;
                break;
        }
        deltas.push_back({dx, dy});
    }

    return deltas;
}

pdf_api::LayoutCollisionWarning ContentEditingService::ValidateLayoutSafety(
    const pdf_api::PageObjectData& targetObject,
    const std::string& newText,
    double newFontSize,
    const std::vector<pdf_api::PageObjectData>& allPageObjects,
    double pageWidth,
    double pageHeight
) {
    pdf_api::LayoutCollisionWarning result;

    if (targetObject.type != pdf_api::PageObjectType::Text) {
        return result;
    }

    // Estimate expanded width based on character length delta and font size ratio
    double origLen = static_cast<double>(std::max<size_t>(1, targetObject.text.length()));
    double newLen = static_cast<double>(std::max<size_t>(1, newText.length()));
    double lenRatio = newLen / origLen;

    double origSize = std::max(1.0, targetObject.fontSize);
    double fontRatio = (newFontSize > 0.0) ? (newFontSize / origSize) : 1.0;

    double estimatedWidth = targetObject.bounds.width * lenRatio * fontRatio;
    double estimatedHeight = targetObject.bounds.height * fontRatio;

    pdf_api::PdfRect expBounds = targetObject.bounds;
    expBounds.width = std::max(targetObject.bounds.width, estimatedWidth);
    expBounds.height = std::max(targetObject.bounds.height, estimatedHeight);
    result.expandedBounds = expBounds;

    // 1. Margin Overflow Check
    if (expBounds.x + expBounds.width > pageWidth || expBounds.y + expBounds.height > pageHeight) {
        result.hasOverflow = true;
        result.warningMessage = "Warning: Modified text exceeds page boundaries (Page: " +
            std::to_string(static_cast<int>(pageWidth)) + "x" + std::to_string(static_cast<int>(pageHeight)) + " pt).";
    }

    // 2. Neighboring Object Collision Detection
    for (const auto& other : allPageObjects) {
        if (other.objectIndex == targetObject.objectIndex) continue;

        // Check AABB intersection
        bool xOverlap = (expBounds.x < other.bounds.x + other.bounds.width) &&
                        (expBounds.x + expBounds.width > other.bounds.x);
        bool yOverlap = (expBounds.y < other.bounds.y + other.bounds.height) &&
                        (expBounds.y + expBounds.height > other.bounds.y);

        if (xOverlap && yOverlap) {
            result.hasCollision = true;
            result.conflictingObjectIndex = other.objectIndex;
            result.warningMessage = "Layout Collision: Text expansion causes overlap with object #" +
                std::to_string(other.objectIndex) + " (" +
                (other.type == pdf_api::PageObjectType::Text ? "Text: '" + other.text.substr(0, 15) + "...'" : "Vector/Image") +
                ") at (" + std::to_string(static_cast<int>(other.bounds.x)) + ", " +
                std::to_string(static_cast<int>(other.bounds.y)) + "). PDF content stream lacks automatic text reflow.";
            break;
        }
    }

    return result;
}

std::string ContentEditingService::AnalyzeFontCapabilities(const pdf_api::PageObjectData& object) {
    if (object.type != pdf_api::PageObjectType::Text) {
        return "Non-text object.";
    }

    if (object.isSubsetFont) {
        return "Embedded Subset Font (" + object.fontName + "): Direct in-place editing is restricted "
               "to existing glyphs. Adding new characters requires font reconstruction with standard Helvetica substitution.";
    }

    if (object.isEmbeddedFont) {
        return "Fully Embedded Font (" + object.fontName + "): Editing is reliable if glyph encoding exists. "
               "Font metrics are preserved.";
    }

    return "Standard Font (" + (object.fontName.empty() ? "Helvetica" : object.fontName) +
           "): Fully editable without substitution.";
}

QString ContentEditingService::GetLayerDescription(pdf_api::ContentEditMode mode) {
    switch (mode) {
        case pdf_api::ContentEditMode::NativeContentEdit:
            return QStringLiteral("LAYER: Native Page Content Stream (Base PDF Object)");
        case pdf_api::ContentEditMode::Reconstruction:
            return QStringLiteral("LAYER: Reconstructed Content Stream (Standard Font Replacement)");
        case pdf_api::ContentEditMode::OverlayPlacement:
            return QStringLiteral("LAYER: Interactive Overlay (Annotation - Not Native Stream)");
        default:
            return QStringLiteral("LAYER: Unknown");
    }
}

// ==========================================
// Command Implementations
// ==========================================

InsertTextCommand::InsertTextCommand(
    std::shared_ptr<domain::DocumentModel> doc,
    int pageIndex,
    std::string text,
    double x,
    double y,
    std::string fontName,
    double fontSize,
    pdf_api::PdfColor color
) : m_doc(std::move(doc)),
    m_pageIndex(pageIndex),
    m_text(std::move(text)),
    m_x(x),
    m_y(y),
    m_fontName(std::move(fontName)),
    m_fontSize(fontSize),
    m_color(color) {}

bool InsertTextCommand::Execute() {
    if (!m_doc || !m_doc->IsValid()) return false;
    auto res = m_doc->InsertPageText(m_pageIndex, m_text, m_x, m_y, m_fontName, m_fontSize, m_color);
    if (res.has_value()) {
        m_insertedIndex = res.value;
        return true;
    }
    return false;
}

bool InsertTextCommand::Undo() {
    if (!m_doc || !m_doc->IsValid() || m_insertedIndex < 0) return false;
    auto res = m_doc->DeletePageObject(m_pageIndex, m_insertedIndex);
    return res.has_value();
}

std::string InsertTextCommand::GetDescription() const {
    return "Insert Native Text: '" + m_text.substr(0, 15) + "...'";
}

InsertShapeCommand::InsertShapeCommand(
    std::shared_ptr<domain::DocumentModel> doc,
    int pageIndex,
    pdf_api::PdfRect rect,
    pdf_api::PdfColor strokeColor,
    pdf_api::PdfColor fillColor,
    double strokeWidth
) : m_doc(std::move(doc)),
    m_pageIndex(pageIndex),
    m_rect(rect),
    m_strokeColor(strokeColor),
    m_fillColor(fillColor),
    m_strokeWidth(strokeWidth) {}

bool InsertShapeCommand::Execute() {
    if (!m_doc || !m_doc->IsValid()) return false;
    auto res = m_doc->InsertPageShape(m_pageIndex, m_rect, m_strokeColor, m_fillColor, m_strokeWidth);
    if (res.has_value()) {
        m_insertedIndex = res.value;
        return true;
    }
    return false;
}

bool InsertShapeCommand::Undo() {
    if (!m_doc || !m_doc->IsValid() || m_insertedIndex < 0) return false;
    auto res = m_doc->DeletePageObject(m_pageIndex, m_insertedIndex);
    return res.has_value();
}

std::string InsertShapeCommand::GetDescription() const {
    return "Insert Native Rectangle Shape";
}

DeleteObjectsCommand::DeleteObjectsCommand(
    std::shared_ptr<domain::DocumentModel> doc,
    int pageIndex,
    std::vector<pdf_api::PageObjectData> objectsToDelete
) : m_doc(std::move(doc)),
    m_pageIndex(pageIndex),
    m_objects(std::move(objectsToDelete)) {
    // Sort in descending object index order for safe sequential deletion
    std::sort(m_objects.begin(), m_objects.end(), [](const auto& a, const auto& b) {
        return a.objectIndex > b.objectIndex;
    });
}

bool DeleteObjectsCommand::Execute() {
    if (!m_doc || !m_doc->IsValid()) return false;
    for (const auto& obj : m_objects) {
        m_doc->DeletePageObject(m_pageIndex, obj.objectIndex);
    }
    return true;
}

bool DeleteObjectsCommand::Undo() {
    if (!m_doc || !m_doc->IsValid()) return false;
    // Re-insert objects
    for (const auto& obj : m_objects) {
        if (obj.type == pdf_api::PageObjectType::Text) {
            m_doc->InsertPageText(m_pageIndex, obj.text, obj.bounds.x, obj.bounds.y,
                                  obj.fontName.empty() ? "Helvetica" : obj.fontName, obj.fontSize, obj.fillColor);
        } else if (obj.type == pdf_api::PageObjectType::Path) {
            m_doc->InsertPageShape(m_pageIndex, obj.bounds, obj.strokeColor, obj.fillColor, obj.strokeWidth);
        }
    }
    return true;
}

std::string DeleteObjectsCommand::GetDescription() const {
    return "Delete " + std::to_string(m_objects.size()) + " Page Object(s)";
}

TransformObjectsCommand::TransformObjectsCommand(
    std::shared_ptr<domain::DocumentModel> doc,
    int pageIndex,
    std::vector<int> objectIndices,
    double dx,
    double dy,
    double scaleX,
    double scaleY
) : m_doc(std::move(doc)),
    m_pageIndex(pageIndex),
    m_indices(std::move(objectIndices)),
    m_dx(dx),
    m_dy(dy),
    m_scaleX(scaleX),
    m_scaleY(scaleY) {}

bool TransformObjectsCommand::Execute() {
    if (!m_doc || !m_doc->IsValid()) return false;
    for (int idx : m_indices) {
        m_doc->TransformPageObject(m_pageIndex, idx, m_dx, m_dy, m_scaleX, m_scaleY);
    }
    return true;
}

bool TransformObjectsCommand::Undo() {
    if (!m_doc || !m_doc->IsValid()) return false;
    double invScaleX = (m_scaleX != 0.0) ? (1.0 / m_scaleX) : 1.0;
    double invScaleY = (m_scaleY != 0.0) ? (1.0 / m_scaleY) : 1.0;
    for (int idx : m_indices) {
        m_doc->TransformPageObject(m_pageIndex, idx, -m_dx, -m_dy, invScaleX, invScaleY);
    }
    return true;
}

std::string TransformObjectsCommand::GetDescription() const {
    return "Move/Scale " + std::to_string(m_indices.size()) + " Object(s)";
}

ModifyTextCommand::ModifyTextCommand(
    std::shared_ptr<domain::DocumentModel> doc,
    int pageIndex,
    int objectIndex,
    std::string oldText,
    double oldFontSize,
    pdf_api::PdfColor oldColor,
    std::string newText,
    double newFontSize,
    pdf_api::PdfColor newColor
) : m_doc(std::move(doc)),
    m_pageIndex(pageIndex),
    m_objectIndex(objectIndex),
    m_oldText(std::move(oldText)),
    m_oldFontSize(oldFontSize),
    m_oldColor(oldColor),
    m_newText(std::move(newText)),
    m_newFontSize(newFontSize),
    m_newColor(newColor) {}

bool ModifyTextCommand::Execute() {
    if (!m_doc || !m_doc->IsValid()) return false;
    auto res = m_doc->UpdatePageTextObject(m_pageIndex, m_objectIndex, m_newText, m_newFontSize, m_newColor);
    return res.has_value();
}

bool ModifyTextCommand::Undo() {
    if (!m_doc || !m_doc->IsValid()) return false;
    auto res = m_doc->UpdatePageTextObject(m_pageIndex, m_objectIndex, m_oldText, m_oldFontSize, m_oldColor);
    return res.has_value();
}

std::string ModifyTextCommand::GetDescription() const {
    return "Edit Text Object #" + std::to_string(m_objectIndex);
}

ReorderObjectCommand::ReorderObjectCommand(
    std::shared_ptr<domain::DocumentModel> doc,
    int pageIndex,
    int fromIndex,
    int toIndex
) : m_doc(std::move(doc)),
    m_pageIndex(pageIndex),
    m_fromIndex(fromIndex),
    m_toIndex(toIndex) {}

bool ReorderObjectCommand::Execute() {
    if (!m_doc || !m_doc->IsValid()) return false;
    auto res = m_doc->ReorderPageObject(m_pageIndex, m_fromIndex, m_toIndex);
    return res.has_value();
}

bool ReorderObjectCommand::Undo() {
    if (!m_doc || !m_doc->IsValid()) return false;
    auto res = m_doc->ReorderPageObject(m_pageIndex, m_toIndex, m_fromIndex);
    return res.has_value();
}

std::string ReorderObjectCommand::GetDescription() const {
    return "Reorder Object Z-Order (#" + std::to_string(m_fromIndex) + " -> #" + std::to_string(m_toIndex) + ")";
}

} // namespace pdfinity::services
