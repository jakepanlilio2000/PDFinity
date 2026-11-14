// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "FormService.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <algorithm>

namespace pdfinity::services {

FormService::FormService(QObject* parent) : QObject(parent) {}

FormSummary FormService::GetFormSummary(const domain::DocumentModel& doc) const {
    FormSummary summary;
    auto typeRes = doc.GetFormType();
    if (typeRes.has_value()) {
        summary.formType = typeRes.value;
    }

    auto fieldsRes = doc.GetAllFormFields();
    if (!fieldsRes.has_value()) {
        return summary;
    }

    const auto& fields = fieldsRes.value;
    summary.totalFields = static_cast<int>(fields.size());

    for (const auto& f : fields) {
        if (f.isReadOnly) summary.readOnlyFields++;
        if (f.isRequired) summary.requiredFields++;

        bool isFilled = false;
        switch (f.type) {
            case pdf_api::FormFieldType::TextField:
                summary.textFields++;
                if (!f.value.empty()) isFilled = true;
                break;
            case pdf_api::FormFieldType::CheckBox:
                summary.checkBoxes++;
                if (f.isChecked) isFilled = true;
                break;
            case pdf_api::FormFieldType::RadioButton:
                summary.radioButtons++;
                if (f.isChecked) isFilled = true;
                break;
            case pdf_api::FormFieldType::ComboBox:
            case pdf_api::FormFieldType::ListBox:
                summary.choiceFields++;
                if (!f.value.empty() || !f.selectedIndices.empty()) isFilled = true;
                break;
            default:
                break;
        }

        if (isFilled) {
            summary.filledFields++;
        }
    }

    if (summary.totalFields > 0) {
        summary.completionPercentage = (static_cast<double>(summary.filledFields) / summary.totalFields) * 100.0;
    }

    return summary;
}

bool FormService::HasXfa(const domain::DocumentModel& doc) const {
    return doc.HasXfa();
}

bool FormService::HasInteractiveForm(const domain::DocumentModel& doc) const {
    return doc.HasForm();
}

bool FormService::ValidateTextInput(const std::string& input, std::string& outSanitized, std::string& outError) {
    // 1. Length enforcement: 64 KB max
    constexpr size_t kMaxFieldLength = 65536;
    if (input.size() > kMaxFieldLength) {
        outError = "Field input exceeds maximum 64KB security threshold.";
        return false;
    }

    // 2. Control character sanitization (preserve \t, \r, \n)
    outSanitized.clear();
    outSanitized.reserve(input.size());

    for (unsigned char c : input) {
        if (c < 32 && c != '\t' && c != '\r' && c != '\n') {
            continue; // Skip dangerous control characters / null bytes
        }
        outSanitized.push_back(static_cast<char>(c));
    }

    return true;
}

pdf_api::EngineResult<void> FormService::SetFieldValue(
    domain::DocumentModel& doc,
    int pageIndex,
    int annotIndex,
    const std::string& value
) {
    std::string sanitized;
    std::string err;
    if (!ValidateTextInput(value, sanitized, err)) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::UnsupportedFeature, err);
    }

    auto res = doc.SetFormFieldValue(pageIndex, annotIndex, sanitized);
    if (res.has_value()) {
        emit FieldModified(pageIndex, annotIndex);
    }
    return res;
}

pdf_api::EngineResult<void> FormService::SetFieldChecked(
    domain::DocumentModel& doc,
    int pageIndex,
    int annotIndex,
    bool checked
) {
    auto res = doc.SetFormFieldChecked(pageIndex, annotIndex, checked);
    if (res.has_value()) {
        emit FieldModified(pageIndex, annotIndex);
    }
    return res;
}

pdf_api::EngineResult<void> FormService::SetFieldOption(
    domain::DocumentModel& doc,
    int pageIndex,
    int annotIndex,
    int optionIndex
) {
    auto res = doc.SetFormFieldOption(pageIndex, annotIndex, optionIndex);
    if (res.has_value()) {
        emit FieldModified(pageIndex, annotIndex);
    }
    return res;
}

pdf_api::EngineResult<void> FormService::ResetForm(domain::DocumentModel& doc) {
    auto res = doc.ResetFormFields();
    if (res.has_value()) {
        emit FormReset();
    }
    return res;
}

pdf_api::EngineResult<void> FormService::FlattenForm(domain::DocumentModel& doc, bool forPrint) {
    auto res = doc.FlattenDocument(forPrint);
    if (res.has_value()) {
        emit FormFlattened();
    }
    return res;
}

static const char* FieldTypeToString(pdf_api::FormFieldType type) {
    switch (type) {
        case pdf_api::FormFieldType::TextField: return "TextField";
        case pdf_api::FormFieldType::CheckBox: return "CheckBox";
        case pdf_api::FormFieldType::RadioButton: return "RadioButton";
        case pdf_api::FormFieldType::ComboBox: return "ComboBox";
        case pdf_api::FormFieldType::ListBox: return "ListBox";
        case pdf_api::FormFieldType::PushButton: return "PushButton";
        case pdf_api::FormFieldType::Signature: return "Signature";
        case pdf_api::FormFieldType::Xfa: return "XfaField";
        default: return "Unknown";
    }
}

std::string FormService::ExportFormDataToJson(const domain::DocumentModel& doc) const {
    QJsonObject rootObj;
    rootObj["documentPath"] = QString::fromStdString(doc.GetPath().string());
    rootObj["exportedAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    auto typeRes = doc.GetFormType();
    if (typeRes.has_value()) {
        switch (typeRes.value) {
            case pdf_api::FormType::AcroForm: rootObj["formType"] = "AcroForm"; break;
            case pdf_api::FormType::XfaFull: rootObj["formType"] = "XFA_Full"; break;
            case pdf_api::FormType::XfaForeground: rootObj["formType"] = "XFA_Foreground"; break;
            default: rootObj["formType"] = "None"; break;
        }
    }

    QJsonArray fieldsArray;
    auto fieldsRes = doc.GetAllFormFields();
    if (fieldsRes.has_value()) {
        for (const auto& f : fieldsRes.value) {
            QJsonObject fObj;
            fObj["name"] = QString::fromStdString(f.name);
            fObj["altName"] = QString::fromStdString(f.altName);
            fObj["page"] = f.pageIndex;
            fObj["annotIndex"] = f.annotIndex;
            fObj["type"] = FieldTypeToString(f.type);
            fObj["value"] = QString::fromStdString(f.value);
            fObj["defaultValue"] = QString::fromStdString(f.defaultValue);
            fObj["isChecked"] = f.isChecked;
            fObj["isReadOnly"] = f.isReadOnly;
            fObj["isRequired"] = f.isRequired;

            if (!f.options.empty()) {
                QJsonArray optArr;
                for (const auto& opt : f.options) {
                    optArr.append(QString::fromStdString(opt));
                }
                fObj["options"] = optArr;
            }

            fieldsArray.append(fObj);
        }
    }

    rootObj["fields"] = fieldsArray;

    QJsonDocument jsonDoc(rootObj);
    return jsonDoc.toJson(QJsonDocument::Indented).toStdString();
}

pdf_api::EngineResult<int> FormService::ImportFormDataFromJson(
    domain::DocumentModel& doc,
    const std::string& jsonString
) {
    QJsonParseError parseErr{};
    QJsonDocument jsonDoc = QJsonDocument::fromJson(QByteArray::fromStdString(jsonString), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::InvalidFormat, "Malformed JSON input: " + parseErr.errorString().toStdString());
    }

    QJsonObject root = jsonDoc.object();
    if (!root.contains("fields") || !root["fields"].isArray()) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::InvalidFormat, "JSON does not contain a valid 'fields' array");
    }

    auto existingRes = doc.GetAllFormFields();
    if (!existingRes.has_value()) {
        return pdf_api::EngineResult<int>::Fail(existingRes.error, existingRes.errorMessage);
    }

    int importedCount = 0;
    QJsonArray fieldsArray = root["fields"].toArray();

    for (const auto& item : fieldsArray) {
        if (!item.isObject()) continue;
        QJsonObject fObj = item.toObject();
        std::string name = fObj["name"].toString().toStdString();
        if (name.empty()) continue;

        // Find matching field in document by name
        for (const auto& exField : existingRes.value) {
            if (exField.name == name) {
                if (exField.type == pdf_api::FormFieldType::TextField && fObj.contains("value")) {
                    std::string val = fObj["value"].toString().toStdString();
                    if (SetFieldValue(doc, exField.pageIndex, exField.annotIndex, val).has_value()) {
                        importedCount++;
                    }
                } else if ((exField.type == pdf_api::FormFieldType::CheckBox || exField.type == pdf_api::FormFieldType::RadioButton) && fObj.contains("isChecked")) {
                    bool chk = fObj["isChecked"].toBool();
                    if (SetFieldChecked(doc, exField.pageIndex, exField.annotIndex, chk).has_value()) {
                        importedCount++;
                    }
                } else if ((exField.type == pdf_api::FormFieldType::ComboBox || exField.type == pdf_api::FormFieldType::ListBox) && fObj.contains("value")) {
                    std::string val = fObj["value"].toString().toStdString();
                    // Match option index
                    int optIdx = -1;
                    for (size_t o = 0; o < exField.options.size(); ++o) {
                        if (exField.options[o] == val) {
                            optIdx = static_cast<int>(o);
                            break;
                        }
                    }
                    if (optIdx >= 0) {
                        if (SetFieldOption(doc, exField.pageIndex, exField.annotIndex, optIdx).has_value()) {
                            importedCount++;
                        }
                    } else {
                        if (SetFieldValue(doc, exField.pageIndex, exField.annotIndex, val).has_value()) {
                            importedCount++;
                        }
                    }
                }
                break;
            }
        }
    }

    return pdf_api::EngineResult<int>::Ok(importedCount);
}

} // namespace pdfinity::services
