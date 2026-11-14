// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QObject>
#include <QThread>
#include "domain/DocumentModel.hpp"
#include <memory>
#include <atomic>

namespace pdfinity::services {

class DocumentSearchService : public QObject {
    Q_OBJECT

public:
    explicit DocumentSearchService(QObject* parent = nullptr);
    ~DocumentSearchService() override;

    void StartSearch(
        std::shared_ptr<domain::DocumentModel> doc,
        const QString& query,
        bool matchCase = false,
        bool matchWholeWord = false
    );

    void CancelSearch();
    [[nodiscard]] bool IsSearching() const;

signals:
    void SearchStarted(const QString& query);
    void MatchFound(const pdf_api::TextSearchResult& match);
    void SearchProgress(int pagesSearched, int totalPages);
    void SearchCompleted(const std::vector<pdf_api::TextSearchResult>& allMatches);
    void SearchCancelled();

private:
    std::atomic<bool> m_isSearching{false};
    std::atomic<bool> m_cancelRequested{false};
    QThread* m_workerThread{nullptr};
};

} // namespace pdfinity::services
