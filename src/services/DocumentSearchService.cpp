// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "DocumentSearchService.hpp"
#include "application/Logger.hpp"

namespace pdfinity::services {

DocumentSearchService::DocumentSearchService(QObject* parent) : QObject(parent) {}

DocumentSearchService::~DocumentSearchService() {
    CancelSearch();
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait(1000);
    }
}

void DocumentSearchService::StartSearch(
    std::shared_ptr<domain::DocumentModel> doc,
    const QString& query,
    bool matchCase,
    bool matchWholeWord
) {
    CancelSearch();

    if (!doc || !doc->IsValid() || query.trimmed().isEmpty()) {
        emit SearchCompleted({});
        return;
    }

    m_isSearching = true;
    m_cancelRequested = false;
    emit SearchStarted(query);

    // Run search on a dedicated background QThread
    m_workerThread = QThread::create([this, doc = std::move(doc), q = query.toStdString(), matchCase, matchWholeWord]() {
        int totalPages = doc->GetPageCount();
        std::vector<pdf_api::TextSearchResult> allMatches;

        for (int p = 0; p < totalPages; ++p) {
            if (m_cancelRequested.load()) {
                QMetaObject::invokeMethod(this, [this]() {
                    m_isSearching = false;
                    emit SearchCancelled();
                }, Qt::QueuedConnection);
                return;
            }

            auto res = doc->SearchPageText(p, q, matchCase, matchWholeWord);
            if (res.has_value() && !res.value.empty()) {
                for (const auto& match : res.value) {
                    allMatches.push_back(match);
                    QMetaObject::invokeMethod(this, [this, match]() {
                        emit MatchFound(match);
                    }, Qt::QueuedConnection);
                }
            }

            QMetaObject::invokeMethod(this, [this, p, totalPages]() {
                emit SearchProgress(p + 1, totalPages);
            }, Qt::QueuedConnection);
        }

        QMetaObject::invokeMethod(this, [this, matches = std::move(allMatches)]() {
            m_isSearching = false;
            emit SearchCompleted(matches);
        }, Qt::QueuedConnection);
    });

    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);
    m_workerThread->start();
}

void DocumentSearchService::CancelSearch() {
    m_cancelRequested = true;
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->wait(300);
    }
}

bool DocumentSearchService::IsSearching() const {
    return m_isSearching.load();
}

} // namespace pdfinity::services
