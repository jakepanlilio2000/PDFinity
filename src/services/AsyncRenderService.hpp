// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QObject>
#include <QThreadPool>
#include <QImage>
#include "PageCache.hpp"
#include "domain/DocumentModel.hpp"
#include <memory>
#include <atomic>
#include <set>
#include <mutex>

namespace pdfinity::services {

class AsyncRenderService : public QObject {
    Q_OBJECT

public:
    explicit AsyncRenderService(QObject* parent = nullptr);
    ~AsyncRenderService() override;

    PageCache& GetCache() { return m_cache; }
    const PageCache& GetCache() const { return m_cache; }

    // Request render: checks cache first, queues background job if not cached
    void RequestPageRender(
        std::shared_ptr<domain::DocumentModel> doc,
        int pageIndex,
        double zoomScale,
        double dpr,
        int priority = 0
    );

    // Cancel in-flight renders (e.g. on rapid scroll or zoom change)
    void CancelPendingRenders();
    void ClearCache();

signals:
    void PageRendered(int pageIndex, double zoomScale, const QImage& image);
    void RenderFailed(int pageIndex, double zoomScale, const QString& errorMsg);

private:
    PageCache m_cache;
    QThreadPool m_threadPool;
    std::atomic<uint64_t> m_currentEpoch{0};

    std::mutex m_pendingMutex;
    std::set<std::tuple<int, int, int>> m_inFlightKeys; // (pageIndex, zoomPercent, dprPercent)
};

} // namespace pdfinity::services
