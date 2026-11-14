// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/DocumentModel.hpp"
#include <QObject>
#include <memory>
#include <string>
#include <vector>

namespace pdfinity::services {

struct FormSummary {
    pdf_api::FormType formType{pdf_api::FormType::None};
    int totalFields{0};
    int filledFields{0};
    int readOnlyFields{0};
    int requiredFields{0};
    int textFields{0};
    int checkBoxes{0};
    int radioButtons{0};
    int choiceFields{0};
    double completionPercentage{0.0};
};

class FormService : public QObject {
    Q_OBJECT
public:
    explicit FormService(QObject* parent = nullptr);
    ~FormService() override = default;

    // Form Inspection & Statistics
    [[nodiscard]] FormSummary GetFormSummary(const domain::DocumentModel& doc) const;
    [[nodiscard]] bool HasXfa(const domain::DocumentModel& doc) const;
    [[nodiscard]] bool HasInteractiveForm(const domain::DocumentModel& doc) const;

    // Input Validation & Sanitization (Hostile Input Defense)
    [[nodiscard]] static bool ValidateTextInput(const std::string& input, std::string& outSanitized, std::string& outError);

    // Form Field Manipulation
    pdf_api::EngineResult<void> SetFieldValue(
        domain::DocumentModel& doc,
        int pageIndex,
        int annotIndex,
        const std::string& value
    );

    pdf_api::EngineResult<void> SetFieldChecked(
        domain::DocumentModel& doc,
        int pageIndex,
        int annotIndex,
        bool checked
    );

    pdf_api::EngineResult<void> SetFieldOption(
        domain::DocumentModel& doc,
        int pageIndex,
        int annotIndex,
        int optionIndex
    );

    pdf_api::EngineResult<void> ResetForm(domain::DocumentModel& doc);
    pdf_api::EngineResult<void> FlattenForm(domain::DocumentModel& doc, bool forPrint = false);

    // Form Data Export / Import (JSON representation)
    [[nodiscard]] std::string ExportFormDataToJson(const domain::DocumentModel& doc) const;
    pdf_api::EngineResult<int> ImportFormDataFromJson(domain::DocumentModel& doc, const std::string& jsonString);

signals:
    void FieldModified(int pageIndex, int annotIndex);
    void FormReset();
    void FormFlattened();
};

} // namespace pdfinity::services
