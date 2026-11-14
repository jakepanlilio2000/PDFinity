// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "BatchProfileManager.hpp"
#include "application/AppContext.hpp"
#include "application/Logger.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <algorithm>

namespace pdfinity::services {

namespace {

QJsonObject OperationToJson(const domain::BatchOperation& op) {
    QJsonObject obj;
    obj["type"] = static_cast<int>(op.type);
    obj["name"] = QString::fromStdString(op.name);

    QJsonObject params;
    for (const auto& [k, v] : op.parameters) {
        params[QString::fromStdString(k)] = QString::fromStdString(v);
    }
    obj["parameters"] = params;
    return obj;
}

domain::BatchOperation OperationFromJson(const QJsonObject& obj) {
    domain::BatchOperation op;
    op.type = static_cast<domain::BatchOperationType>(obj["type"].toInt());
    op.name = obj["name"].toString().toStdString();

    QJsonObject params = obj["parameters"].toObject();
    for (auto it = params.begin(); it != params.end(); ++it) {
        op.parameters[it.key().toStdString()] = it.value().toString().toStdString();
    }
    return op;
}

QJsonObject ProfileToJson(const domain::BatchProfile& profile) {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(profile.id);
    obj["name"] = QString::fromStdString(profile.name);
    obj["description"] = QString::fromStdString(profile.description);
    obj["isBuiltIn"] = profile.isBuiltIn;

    QJsonArray ops;
    for (const auto& op : profile.operations) {
        ops.append(OperationToJson(op));
    }
    obj["operations"] = ops;
    return obj;
}

domain::BatchProfile ProfileFromJson(const QJsonObject& obj) {
    domain::BatchProfile profile;
    profile.id = obj["id"].toString().toStdString();
    profile.name = obj["name"].toString().toStdString();
    profile.description = obj["description"].toString().toStdString();
    profile.isBuiltIn = obj["isBuiltIn"].toBool();

    QJsonArray ops = obj["operations"].toArray();
    for (const auto& opVal : ops) {
        profile.operations.push_back(OperationFromJson(opVal.toObject()));
    }
    return profile;
}

} // namespace

BatchProfileManager& BatchProfileManager::Instance() {
    static BatchProfileManager instance;
    return instance;
}

BatchProfileManager::BatchProfileManager() {
    InitializeBuiltInProfiles();
    LoadCustomProfiles();
}

void BatchProfileManager::InitializeBuiltInProfiles() {
    m_builtInProfiles.clear();

    // 1. PDF/A Archival & Sanitization
    {
        domain::BatchProfile p;
        p.id = "builtin_pdfa_archival";
        p.name = "PDF/A Archival & Metadata Sanitization";
        p.description = "Converts document to ISO 19005-1 (PDF/A-1b) compliance and sanitizes metadata, hidden layers, and scripts.";
        p.isBuiltIn = true;

        domain::BatchOperation opPdfA;
        opPdfA.type = domain::BatchOperationType::ConvertToPdfA;
        opPdfA.name = "Convert to PDF/A-1b";
        opPdfA.parameters = {
            {"profile", "PdfA1b"},
            {"embedMissingMetadata", "true"},
            {"stripProhibitedActions", "true"},
            {"injectOutputIntent", "true"}
        };
        p.operations.push_back(opPdfA);

        domain::BatchOperation opSanitize;
        opSanitize.type = domain::BatchOperationType::Sanitize;
        opSanitize.name = "Sanitize Metadata & Hidden Elements";
        opSanitize.parameters = {{"profile", "Balanced"}};
        p.operations.push_back(opSanitize);

        m_builtInProfiles.push_back(p);
    }

    // 2. Web Optimization
    {
        domain::BatchProfile p;
        p.id = "builtin_web_optimize";
        p.name = "Web Optimization (96 DPI Downsample)";
        p.description = "Downsamples images to 96 DPI, optimizes streams, and strips comments for web and email distribution.";
        p.isBuiltIn = true;

        domain::BatchOperation op;
        op.type = domain::BatchOperationType::Optimize;
        op.name = "Screen Optimization Preset";
        op.parameters = {
            {"preset", "Screen"},
            {"targetDpi", "96"},
            {"jpegQuality", "75"},
            {"stripMetadata", "true"}
        };
        p.operations.push_back(op);

        m_builtInProfiles.push_back(p);
    }

    // 3. Prepress Optimization
    {
        domain::BatchProfile p;
        p.id = "builtin_prepress_optimize";
        p.name = "Prepress Optimization (300 DPI Lossless)";
        p.description = "Retains 300 DPI raster resolution, applies lossless stream compression, and preserves print metadata.";
        p.isBuiltIn = true;

        domain::BatchOperation op;
        op.type = domain::BatchOperationType::Optimize;
        op.name = "Prepress Optimization Preset";
        op.parameters = {
            {"preset", "Prepress"},
            {"targetDpi", "300"},
            {"jpegQuality", "95"},
            {"stripMetadata", "false"}
        };
        p.operations.push_back(op);

        m_builtInProfiles.push_back(p);
    }

    // 4. Searchable OCR
    {
        domain::BatchProfile p;
        p.id = "builtin_searchable_ocr";
        p.name = "Searchable PDF Generation (OCR)";
        p.description = "Performs offline optical character recognition and injects invisible text layer for searchability.";
        p.isBuiltIn = true;

        domain::BatchOperation op;
        op.type = domain::BatchOperationType::Ocr;
        op.name = "Tesseract OCR Layer Generation";
        op.parameters = {
            {"language", "eng"},
            {"dpi", "150"}
        };
        p.operations.push_back(op);

        m_builtInProfiles.push_back(p);
    }

    // 5. Full Archival Pipeline
    {
        domain::BatchProfile p;
        p.id = "builtin_full_archival";
        p.name = "Full Archival Pipeline (OCR + PDF/A + Optimize)";
        p.description = "Complete archival workflow: OCR text injection, PDF/A-1b standards conversion, and balanced compression.";
        p.isBuiltIn = true;

        domain::BatchOperation opOcr;
        opOcr.type = domain::BatchOperationType::Ocr;
        opOcr.name = "OCR Text Layer";
        opOcr.parameters = {{"language", "eng"}, {"dpi", "150"}};
        p.operations.push_back(opOcr);

        domain::BatchOperation opPdfA;
        opPdfA.type = domain::BatchOperationType::ConvertToPdfA;
        opPdfA.name = "Convert to PDF/A-1b";
        opPdfA.parameters = {{"profile", "PdfA1b"}, {"embedMissingMetadata", "true"}, {"injectOutputIntent", "true"}};
        p.operations.push_back(opPdfA);

        domain::BatchOperation opOpt;
        opOpt.type = domain::BatchOperationType::Optimize;
        opOpt.name = "Recommended Compression";
        opOpt.parameters = {{"preset", "Recommended"}, {"targetDpi", "150"}};
        p.operations.push_back(opOpt);

        m_builtInProfiles.push_back(p);
    }

    // 6. High-Res Image Export
    {
        domain::BatchProfile p;
        p.id = "builtin_export_images";
        p.name = "High-Resolution Image Export (PNG 300 DPI)";
        p.description = "Renders all pages of each document as high-resolution 300 DPI PNG images.";
        p.isBuiltIn = true;

        domain::BatchOperation op;
        op.type = domain::BatchOperationType::ExportImages;
        op.name = "Export Pages as PNG";
        op.parameters = {{"format", "PNG"}, {"dpi", "300"}};
        p.operations.push_back(op);

        m_builtInProfiles.push_back(p);
    }

    // 7. Plain Text Extraction
    {
        domain::BatchProfile p;
        p.id = "builtin_export_text";
        p.name = "Plain Text Extraction (UTF-8)";
        p.description = "Extracts all textual content into structured UTF-8 text files.";
        p.isBuiltIn = true;

        domain::BatchOperation op;
        op.type = domain::BatchOperationType::ExportText;
        op.name = "Export Text to UTF-8";
        op.parameters = {};
        p.operations.push_back(op);

        m_builtInProfiles.push_back(p);
    }
}

std::filesystem::path BatchProfileManager::GetCustomProfilesFilePath() const {
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appData);
    return std::filesystem::path(appData.toStdString()) / "batch_profiles.json";
}

void BatchProfileManager::LoadCustomProfiles() {
    m_customProfiles.clear();
    auto path = GetCustomProfilesFilePath();
    if (!std::filesystem::exists(path)) return;

    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) return;

    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        if (item.isObject()) {
            auto profile = ProfileFromJson(item.toObject());
            profile.isBuiltIn = false;
            m_customProfiles.push_back(profile);
        }
    }
}

void BatchProfileManager::PersistCustomProfiles() {
    auto path = GetCustomProfilesFilePath();
    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        LOG_WARN("BatchProfiles", "Failed to open custom profiles file for writing: " + path.string());
        return;
    }

    QJsonArray arr;
    for (const auto& p : m_customProfiles) {
        arr.append(ProfileToJson(p));
    }

    QJsonDocument doc(arr);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

std::vector<domain::BatchProfile> BatchProfileManager::GetAllProfiles() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<domain::BatchProfile> all = m_builtInProfiles;
    all.insert(all.end(), m_customProfiles.begin(), m_customProfiles.end());
    return all;
}

std::vector<domain::BatchProfile> BatchProfileManager::GetBuiltInProfiles() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_builtInProfiles;
}

std::vector<domain::BatchProfile> BatchProfileManager::GetCustomProfiles() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_customProfiles;
}

std::optional<domain::BatchProfile> BatchProfileManager::FindProfile(const std::string& idOrName) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& p : m_builtInProfiles) {
        if (p.id == idOrName || p.name == idOrName) {
            return p;
        }
    }

    for (const auto& p : m_customProfiles) {
        if (p.id == idOrName || p.name == idOrName) {
            return p;
        }
    }

    return std::nullopt;
}

bool BatchProfileManager::SaveCustomProfile(const domain::BatchProfile& profile) {
    std::lock_guard<std::mutex> lock(m_mutex);
    domain::BatchProfile copy = profile;
    copy.isBuiltIn = false;

    if (copy.id.empty()) {
        copy.id = "custom_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    }

    auto it = std::find_if(m_customProfiles.begin(), m_customProfiles.end(), [&](const domain::BatchProfile& p) {
        return p.id == copy.id;
    });

    if (it != m_customProfiles.end()) {
        *it = copy;
    } else {
        m_customProfiles.push_back(copy);
    }

    PersistCustomProfiles();
    return true;
}

bool BatchProfileManager::DeleteCustomProfile(const std::string& profileId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::remove_if(m_customProfiles.begin(), m_customProfiles.end(), [&](const domain::BatchProfile& p) {
        return p.id == profileId;
    });

    if (it != m_customProfiles.end()) {
        m_customProfiles.erase(it, m_customProfiles.end());
        PersistCustomProfiles();
        return true;
    }
    return false;
}

bool BatchProfileManager::ExportProfileToJson(const domain::BatchProfile& profile, const std::filesystem::path& destination) const {
    QFile file(QString::fromStdString(destination.string()));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QJsonObject obj = ProfileToJson(profile);
    QJsonDocument doc(obj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

std::optional<domain::BatchProfile> BatchProfileManager::ImportProfileFromJson(const std::filesystem::path& sourcePath) {
    QFile file(QString::fromStdString(sourcePath.string()));
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return std::nullopt;

    auto profile = ProfileFromJson(doc.object());
    profile.isBuiltIn = false;
    SaveCustomProfile(profile);
    return profile;
}

void BatchProfileManager::ResetToDefaults() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_customProfiles.clear();
    PersistCustomProfiles();
    InitializeBuiltInProfiles();
}

} // namespace pdfinity::services
