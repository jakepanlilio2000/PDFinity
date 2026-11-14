// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "PageCache.hpp"
#include <cmath>

namespace pdfinity::services {

namespace {
CacheKey MakeKey(int pageIndex, double zoom, double dpr) {
    return CacheKey{
        pageIndex,
        static_cast<int>(std::round(zoom * 100.0)),
        static_cast<int>(std::round(dpr * 100.0))
    };
}
} // namespace

PageCache::PageCache(size_t maxSizeBytes) : m_maxSizeBytes(maxSizeBytes) {}

void PageCache::Put(int pageIndex, double zoom, double dpr, const QImage& image) {
    if (image.isNull()) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    CacheKey key = MakeKey(pageIndex, zoom, dpr);
    size_t imgBytes = static_cast<size_t>(image.sizeInBytes());

    // If already exists, remove it first
    auto it = m_map.find(key);
    if (it != m_map.end()) {
        m_currentSizeBytes -= it->second->sizeBytes;
        m_items.erase(it->second);
        m_map.erase(it);
    }

    // Evict LRU items until space available
    while (m_currentSizeBytes + imgBytes > m_maxSizeBytes && !m_items.empty()) {
        auto last = std::prev(m_items.end());
        m_currentSizeBytes -= last->sizeBytes;
        m_map.erase(last->key);
        m_items.pop_back();
    }

    // Add to front (MRU)
    m_items.push_front(CacheItem{key, image, imgBytes});
    m_map[key] = m_items.begin();
    m_currentSizeBytes += imgBytes;
}

std::optional<QImage> PageCache::Get(int pageIndex, double zoom, double dpr) {
    std::lock_guard<std::mutex> lock(m_mutex);
    CacheKey key = MakeKey(pageIndex, zoom, dpr);

    auto it = m_map.find(key);
    if (it == m_map.end()) {
        return std::nullopt;
    }

    // Move accessed item to front (MRU)
    m_items.splice(m_items.begin(), m_items, it->second);
    return it->second->image;
}

bool PageCache::Contains(int pageIndex, double zoom, double dpr) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    CacheKey key = MakeKey(pageIndex, zoom, dpr);
    return m_map.find(key) != m_map.end();
}

void PageCache::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_items.clear();
    m_map.clear();
    m_currentSizeBytes = 0;
}

size_t PageCache::CurrentSizeBytes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentSizeBytes;
}

size_t PageCache::MaxSizeBytes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_maxSizeBytes;
}

void PageCache::SetMaxSizeBytes(size_t bytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxSizeBytes = bytes;
    while (m_currentSizeBytes > m_maxSizeBytes && !m_items.empty()) {
        auto last = std::prev(m_items.end());
        m_currentSizeBytes -= last->sizeBytes;
        m_map.erase(last->key);
        m_items.pop_back();
    }
}

size_t PageCache::ItemCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_items.size();
}

} // namespace pdfinity::services
