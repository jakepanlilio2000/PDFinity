// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "AnnotationCommandManager.hpp"
#include "application/Logger.hpp"
#include <algorithm>

namespace pdfinity::services {

namespace {
std::string GetTypeName(pdf_api::AnnotationType type) {
    switch (type) {
        case pdf_api::AnnotationType::Highlight: return "Highlight";
        case pdf_api::AnnotationType::Underline: return "Underline";
        case pdf_api::AnnotationType::Strikeout: return "Strikeout";
        case pdf_api::AnnotationType::Squiggly: return "Squiggly";
        case pdf_api::AnnotationType::TextNote: return "Sticky Note";
        case pdf_api::AnnotationType::FreehandInk: return "Freehand Ink";
        case pdf_api::AnnotationType::Square: return "Rectangle";
        case pdf_api::AnnotationType::Circle: return "Circle";
        case pdf_api::AnnotationType::Line: return "Line";
        case pdf_api::AnnotationType::Arrow: return "Arrow";
        case pdf_api::AnnotationType::Stamp: return "Stamp";
        case pdf_api::AnnotationType::FreeText: return "Text Box";
        default: return "Annotation";
    }
}
} // namespace

// AddAnnotationCommand
AddAnnotationCommand::AddAnnotationCommand(
    std::shared_ptr<domain::DocumentModel> doc,
    int pageIndex,
    pdf_api::AnnotationData data
)
    : m_doc(std::move(doc)),
      m_pageIndex(pageIndex),
      m_data(std::move(data)) {}

bool AddAnnotationCommand::Execute() {
    if (!m_doc || !m_doc->IsValid()) return false;
    auto res = m_doc->AddAnnotation(m_pageIndex, m_data);
    if (res.has_value()) {
        m_createdIndex = res.value;
        return true;
    }
    return false;
}

bool AddAnnotationCommand::Undo() {
    if (!m_doc || !m_doc->IsValid() || m_createdIndex < 0) return false;
    auto res = m_doc->DeleteAnnotation(m_pageIndex, m_createdIndex);
    return res.has_value();
}

bool AddAnnotationCommand::Redo() {
    return Execute();
}

std::string AddAnnotationCommand::GetDescription() const {
    return "Add " + GetTypeName(m_data.type);
}

// DeleteAnnotationCommand
DeleteAnnotationCommand::DeleteAnnotationCommand(
    std::shared_ptr<domain::DocumentModel> doc,
    int pageIndex,
    int annotIndex,
    pdf_api::AnnotationData data
)
    : m_doc(std::move(doc)),
      m_pageIndex(pageIndex),
      m_annotIndex(annotIndex),
      m_savedData(std::move(data)) {}

bool DeleteAnnotationCommand::Execute() {
    if (!m_doc || !m_doc->IsValid() || m_annotIndex < 0) return false;
    auto res = m_doc->DeleteAnnotation(m_pageIndex, m_annotIndex);
    return res.has_value();
}

bool DeleteAnnotationCommand::Undo() {
    if (!m_doc || !m_doc->IsValid()) return false;
    auto res = m_doc->AddAnnotation(m_pageIndex, m_savedData);
    if (res.has_value()) {
        m_annotIndex = res.value;
        return true;
    }
    return false;
}

bool DeleteAnnotationCommand::Redo() {
    return Execute();
}

std::string DeleteAnnotationCommand::GetDescription() const {
    return "Delete " + GetTypeName(m_savedData.type);
}

// ModifyAnnotationCommand
ModifyAnnotationCommand::ModifyAnnotationCommand(
    std::shared_ptr<domain::DocumentModel> doc,
    int pageIndex,
    int annotIndex,
    pdf_api::AnnotationData oldData,
    pdf_api::AnnotationData newData
)
    : m_doc(std::move(doc)),
      m_pageIndex(pageIndex),
      m_annotIndex(annotIndex),
      m_oldData(std::move(oldData)),
      m_newData(std::move(newData)) {}

bool ModifyAnnotationCommand::Execute() {
    if (!m_doc || !m_doc->IsValid() || m_annotIndex < 0) return false;
    auto res = m_doc->UpdateAnnotation(m_pageIndex, m_annotIndex, m_newData);
    return res.has_value();
}

bool ModifyAnnotationCommand::Undo() {
    if (!m_doc || !m_doc->IsValid() || m_annotIndex < 0) return false;
    auto res = m_doc->UpdateAnnotation(m_pageIndex, m_annotIndex, m_oldData);
    return res.has_value();
}

bool ModifyAnnotationCommand::Redo() {
    return Execute();
}

std::string ModifyAnnotationCommand::GetDescription() const {
    return "Modify " + GetTypeName(m_newData.type);
}

// AnnotationCommandManager
AnnotationCommandManager::AnnotationCommandManager(QObject* parent)
    : QObject(parent) {}

bool AnnotationCommandManager::ExecuteCommand(std::unique_ptr<IAnnotationCommand> cmd) {
    if (!cmd) return false;
    if (cmd->Execute()) {
        m_undoStack.push_back(std::move(cmd));
        if (m_undoStack.size() > MaxUndoDepth) {
            m_undoStack.erase(m_undoStack.begin());
        }
        m_redoStack.clear();
        UpdateState();
        emit CommandExecuted();
        return true;
    }
    return false;
}

bool AnnotationCommandManager::Undo() {
    if (m_undoStack.empty()) return false;
    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    if (cmd->Undo()) {
        m_redoStack.push_back(std::move(cmd));
        UpdateState();
        emit CommandExecuted();
        return true;
    }
    return false;
}

bool AnnotationCommandManager::Redo() {
    if (m_redoStack.empty()) return false;
    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    if (cmd->Redo()) {
        m_undoStack.push_back(std::move(cmd));
        UpdateState();
        emit CommandExecuted();
        return true;
    }
    return false;
}

void AnnotationCommandManager::Clear() {
    m_undoStack.clear();
    m_redoStack.clear();
    UpdateState();
}

bool AnnotationCommandManager::CanUndo() const noexcept {
    return !m_undoStack.empty();
}

bool AnnotationCommandManager::CanRedo() const noexcept {
    return !m_redoStack.empty();
}

std::string AnnotationCommandManager::GetUndoDescription() const {
    if (m_undoStack.empty()) return "";
    return m_undoStack.back()->GetDescription();
}

std::string AnnotationCommandManager::GetRedoDescription() const {
    if (m_redoStack.empty()) return "";
    return m_redoStack.back()->GetDescription();
}

void AnnotationCommandManager::UpdateState() {
    emit CanUndoChanged(CanUndo());
    emit CanRedoChanged(CanRedo());
}

} // namespace pdfinity::services
