// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include "services/PageCache.hpp"

using namespace pdfinity::services;

class TestPageCache : public QObject {
    Q_OBJECT

private slots:
    void testPutAndGet();
    void testZoomAndDprSeparation();
    void testEvictionPolicy();
    void testClear();
};

void TestPageCache::testPutAndGet() {
    PageCache cache(10 * 1024 * 1024); // 10 MB

    QImage img(200, 200, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::red);

    cache.Put(0, 1.0, 1.0, img);
    QVERIFY(cache.Contains(0, 1.0, 1.0));
    QCOMPARE(cache.ItemCount(), 1ULL);

    auto retrieved = cache.Get(0, 1.0, 1.0);
    QVERIFY(retrieved.has_value());
    QCOMPARE(retrieved.value().size(), QSize(200, 200));

    // Non-existent page
    QVERIFY(!cache.Get(1, 1.0, 1.0).has_value());
}

void TestPageCache::testZoomAndDprSeparation() {
    PageCache cache(10 * 1024 * 1024);

    QImage img1(100, 100, QImage::Format_ARGB32_Premultiplied);
    QImage img2(150, 150, QImage::Format_ARGB32_Premultiplied);

    cache.Put(0, 1.0, 1.0, img1);
    cache.Put(0, 1.5, 1.0, img2);

    QCOMPARE(cache.ItemCount(), 2ULL);

    auto r1 = cache.Get(0, 1.0, 1.0);
    auto r2 = cache.Get(0, 1.5, 1.0);

    QVERIFY(r1.has_value());
    QVERIFY(r2.has_value());
    QCOMPARE(r1.value().size(), QSize(100, 100));
    QCOMPARE(r2.value().size(), QSize(150, 150));
}

void TestPageCache::testEvictionPolicy() {
    // Each 100x100 ARGB32 image is 100 * 100 * 4 = 40,000 bytes (~39 KB)
    // Set limit to 90,000 bytes (room for ~2 images)
    PageCache cache(90000);

    QImage img1(100, 100, QImage::Format_ARGB32_Premultiplied);
    QImage img2(100, 100, QImage::Format_ARGB32_Premultiplied);
    QImage img3(100, 100, QImage::Format_ARGB32_Premultiplied);

    cache.Put(0, 1.0, 1.0, img1);
    cache.Put(1, 1.0, 1.0, img2);
    QCOMPARE(cache.ItemCount(), 2ULL);

    // Adding 3rd image must evict LRU (page 0)
    cache.Put(2, 1.0, 1.0, img3);
    QCOMPARE(cache.ItemCount(), 2ULL);

    QVERIFY(!cache.Contains(0, 1.0, 1.0)); // Evicted!
    QVERIFY(cache.Contains(1, 1.0, 1.0));
    QVERIFY(cache.Contains(2, 1.0, 1.0));
    QVERIFY(cache.CurrentSizeBytes() <= cache.MaxSizeBytes());
}

void TestPageCache::testClear() {
    PageCache cache(10 * 1024 * 1024);

    QImage img(100, 100, QImage::Format_ARGB32_Premultiplied);
    cache.Put(0, 1.0, 1.0, img);
    cache.Put(1, 1.0, 1.0, img);
    QCOMPARE(cache.ItemCount(), 2ULL);

    cache.Clear();
    QCOMPARE(cache.ItemCount(), 0ULL);
    QCOMPARE(cache.CurrentSizeBytes(), 0ULL);
    QVERIFY(!cache.Contains(0, 1.0, 1.0));
}

QTEST_MAIN(TestPageCache)
#include "test_page_cache.moc"
