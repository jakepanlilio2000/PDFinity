// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "AsyncRenderService.hpp"
#include "application/Logger.hpp"
#include <QRunnable>
#include <QPointer>
#include <cmath>

namespace pdfinity::services {

namespace {
class RenderTask : public QRunnable {
public:
    RenderTask(
        QPointer<AsyncRenderService> service,
        std::shared_ptr<domain::DocumentModel> doc,
        int pageIndex,
        double zoomScale,
        double dpr,
        uint64_t epoch
    ) : m_service(service),
        m_doc(std::move(doc)),
        m_pageIndex(pageIndex),
        m_zoomScale(zoomScale),
        m_dpr(dpr),
        m_epoch(epoch) {
        setAutoDelete(true);
    }

    void run() override {
        if (!m_service || !m_doc || !m_doc->IsValid()) return;

        pdf_api::RenderOptions opts;
        opts.scale = m_zoomScale * m_dpr;
        opts.dpi = static_cast<int>(std::round(96.0 * m_dpr));
        opts.renderAnnotations = true;
        opts.renderFormFields = true;

        auto res = m_doc->RenderPage(m_pageIndex, opts);
        if (!res.has_value()) {
            QMetaObject::invokeMethod(m_service, [service = m_service, p = m_pageIndex, z = m_zoomScale, err = QString::fromStdString(res.errorMessage)]() {
                if (service) emit service->RenderFailed(p, z, err);
            }, Qt::QueuedConnection);
            return;
        }

        const auto& raw = res.value;
        // Construct QImage from raw BGRA data and detach/copy for thread safety
        QImage rendered(
            raw.pixels.data(),
            raw.width,
            raw.height,
            raw.stride,
            QImage::Format_ARGB32_Premultiplied
        );
        rendered.setDevicePixelRatio(m_dpr);
        QImage imageCopy = rendered.copy(); // Owns its own memory

        QMetaObject::invokeMethod(m_service, [service = m_service, p = m_pageIndex, z = m_zoomScale, dpr = m_dpr, img = std::move(imageCopy), epoch = m_epoch]() {
            if (!service) return;
            // Cache it
            service->GetCache().Put(p, z, dpr, img);
            emit service->PageRendered(p, z, img);
        }, Qt::QueuedConnection);
    }

private:
    QPointer<AsyncRenderService> m_service;
    std::shared_ptr<domain::DocumentModel> m_doc;
    int m_pageIndex{0};
    double m_zoomScale{1.0};
    double m_dpr{1.0};
    uint64_t m_epoch{0};
};
} // namespace

AsyncRenderService::AsyncRenderService(QObject* parent) : QObject(parent) {
    m_threadPool.setMaxThreadCount(std::max(2, QThread::idealThreadCount() - 1));
}

AsyncRenderService::~AsyncRenderService() {
    CancelPendingRenders();
    m_threadPool.waitForDone(500);
}

void AsyncRenderService::RequestPageRender(
    std::shared_ptr<domain::DocumentModel> doc,
    int pageIndex,
    double zoomScale,
    double dpr,
    int priority
) {
    if (!doc || !doc->IsValid() || pageIndex < 0 || pageIndex >= doc->GetPageCount()) {
        return;
    }

    // Check LRU cache first
    auto cached = m_cache.Get(pageIndex, zoomScale, dpr);
    if (cached.has_value()) {
        emit PageRendered(pageIndex, zoomScale, cached.value());
        return;
    }

    // Queue render task on thread pool
    auto* task = new RenderTask(this, std::move(doc), pageIndex, zoomScale, dpr, m_currentEpoch.load());
    m_threadPool.start(task, priority);
}

void AsyncRenderService::CancelPendingRenders() {
    m_currentEpoch.fetch_add(1);
    m_threadPool.clear(); // Clears unstarted tasks from queue
}

void AsyncRenderService::ClearCache() {
    m_cache.Clear();
}

} // namespace pdfinity::services
