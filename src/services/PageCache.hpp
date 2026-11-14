// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QImage>
#include <mutex>
#include <list>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace pdfinity::services {

struct CacheKey {
    int pageIndex{0};
    int zoomPercent{100}; // e.g. 100 for 1.0x, 150 for 1.5x
    int dprPercent{100};  // e.g. 100 for 1.0 dpr, 200 for 2.0 dpr

    bool operator==(const CacheKey& other) const noexcept {
        return pageIndex == other.pageIndex &&
               zoomPercent == other.zoomPercent &&
               dprPercent == other.dprPercent;
    }
};

struct CacheKeyHash {
    size_t operator()(const CacheKey& k) const noexcept {
        size_t h1 = std::hash<int>{}(k.pageIndex);
        size_t h2 = std::hash<int>{}(k.zoomPercent);
        size_t h3 = std::hash<int>{}(k.dprPercent);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

class PageCache {
public:
    explicit PageCache(size_t maxSizeBytes = 128 * 1024 * 1024); // Default 128 MB bounded cache
    ~PageCache() = default;

    PageCache(const PageCache&) = delete;
    PageCache& operator=(const PageCache&) = delete;

    void Put(int pageIndex, double zoom, double dpr, const QImage& image);
    std::optional<QImage> Get(int pageIndex, double zoom, double dpr);
    bool Contains(int pageIndex, double zoom, double dpr) const;
    void Clear();

    [[nodiscard]] size_t CurrentSizeBytes() const;
    [[nodiscard]] size_t MaxSizeBytes() const;
    void SetMaxSizeBytes(size_t bytes);
    [[nodiscard]] size_t ItemCount() const;

private:
    struct CacheItem {
        CacheKey key;
        QImage image;
        size_t sizeBytes{0};
    };

    mutable std::mutex m_mutex;
    size_t m_maxSizeBytes{128 * 1024 * 1024};
    size_t m_currentSizeBytes{0};

    using ItemList = std::list<CacheItem>;
    ItemList m_items; // MRU at front, LRU at back
    std::unordered_map<CacheKey, ItemList::iterator, CacheKeyHash> m_map;
};

} // namespace pdfinity::services
