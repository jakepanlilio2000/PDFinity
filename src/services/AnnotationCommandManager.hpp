// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/DocumentModel.hpp"
#include <QObject>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>

namespace pdfinity::services {

class IAnnotationCommand {
public:
    virtual ~IAnnotationCommand() = default;
    virtual bool Execute() = 0;
    virtual bool Undo() = 0;
    virtual bool Redo() = 0;
    [[nodiscard]] virtual std::string GetDescription() const = 0;
};

class AddAnnotationCommand : public IAnnotationCommand {
public:
    AddAnnotationCommand(
        std::shared_ptr<domain::DocumentModel> doc,
        int pageIndex,
        pdf_api::AnnotationData data
    );

    bool Execute() override;
    bool Undo() override;
    bool Redo() override;
    [[nodiscard]] std::string GetDescription() const override;

private:
    std::shared_ptr<domain::DocumentModel> m_doc;
    int m_pageIndex{0};
    pdf_api::AnnotationData m_data;
    int m_createdIndex{-1};
};

class DeleteAnnotationCommand : public IAnnotationCommand {
public:
    DeleteAnnotationCommand(
        std::shared_ptr<domain::DocumentModel> doc,
        int pageIndex,
        int annotIndex,
        pdf_api::AnnotationData data
    );

    bool Execute() override;
    bool Undo() override;
    bool Redo() override;
    [[nodiscard]] std::string GetDescription() const override;

private:
    std::shared_ptr<domain::DocumentModel> m_doc;
    int m_pageIndex{0};
    int m_annotIndex{-1};
    pdf_api::AnnotationData m_savedData;
};

class ModifyAnnotationCommand : public IAnnotationCommand {
public:
    ModifyAnnotationCommand(
        std::shared_ptr<domain::DocumentModel> doc,
        int pageIndex,
        int annotIndex,
        pdf_api::AnnotationData oldData,
        pdf_api::AnnotationData newData
    );

    bool Execute() override;
    bool Undo() override;
    bool Redo() override;
    [[nodiscard]] std::string GetDescription() const override;

private:
    std::shared_ptr<domain::DocumentModel> m_doc;
    int m_pageIndex{0};
    int m_annotIndex{-1};
    pdf_api::AnnotationData m_oldData;
    pdf_api::AnnotationData m_newData;
};

class AnnotationCommandManager : public QObject {
    Q_OBJECT
public:
    explicit AnnotationCommandManager(QObject* parent = nullptr);
    ~AnnotationCommandManager() override = default;

    bool ExecuteCommand(std::unique_ptr<IAnnotationCommand> cmd);
    bool Undo();
    bool Redo();
    void Clear();

    [[nodiscard]] bool CanUndo() const noexcept;
    [[nodiscard]] bool CanRedo() const noexcept;
    [[nodiscard]] std::string GetUndoDescription() const;
    [[nodiscard]] std::string GetRedoDescription() const;

signals:
    void CanUndoChanged(bool canUndo);
    void CanRedoChanged(bool canRedo);
    void CommandExecuted();

private:
    void UpdateState();

    std::vector<std::unique_ptr<IAnnotationCommand>> m_undoStack;
    std::vector<std::unique_ptr<IAnnotationCommand>> m_redoStack;
    static constexpr size_t MaxUndoDepth = 100;
};

} // namespace pdfinity::services
