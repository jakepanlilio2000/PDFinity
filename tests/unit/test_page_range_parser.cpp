// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include "domain/PageRangeParser.hpp"

using namespace pdfinity::domain;

class TestPageRangeParser : public QObject {
    Q_OBJECT

private slots:
    void testBasicNumbers();
    void testKeywords();
    void testRanges();
    void testDescendingAndOpenRanges();
    void testMixedRanges();
    void testOutOfBoundsAndInvalid();
    void testFormatting();
};

void TestPageRangeParser::testBasicNumbers() {
    auto res = PageRangeParser::Parse("1, 2, 3", 5);
    QVERIFY(res.has_value());
    QCOMPARE(res.value, (std::vector<int>{0, 1, 2}));

    auto res2 = PageRangeParser::Parse("5", 5);
    QVERIFY(res2.has_value());
    QCOMPARE(res2.value, (std::vector<int>{4}));
}

void TestPageRangeParser::testKeywords() {
    auto allRes = PageRangeParser::Parse("all", 4);
    QVERIFY(allRes.has_value());
    QCOMPARE(allRes.value, (std::vector<int>{0, 1, 2, 3}));

    auto emptyRes = PageRangeParser::Parse("", 3);
    QVERIFY(emptyRes.has_value());
    QCOMPARE(emptyRes.value, (std::vector<int>{0, 1, 2}));

    auto oddRes = PageRangeParser::Parse("odd", 5);
    QVERIFY(oddRes.has_value());
    QCOMPARE(oddRes.value, (std::vector<int>{0, 2, 4}));

    auto evenRes = PageRangeParser::Parse("even", 5);
    QVERIFY(evenRes.has_value());
    QCOMPARE(evenRes.value, (std::vector<int>{1, 3}));

    auto revRes = PageRangeParser::Parse("reverse", 4);
    QVERIFY(revRes.has_value());
    QCOMPARE(revRes.value, (std::vector<int>{3, 2, 1, 0}));
}

void TestPageRangeParser::testRanges() {
    auto res = PageRangeParser::Parse("1-3", 5);
    QVERIFY(res.has_value());
    QCOMPARE(res.value, (std::vector<int>{0, 1, 2}));

    auto res2 = PageRangeParser::Parse("2-4, 5", 5);
    QVERIFY(res2.has_value());
    QCOMPARE(res2.value, (std::vector<int>{1, 2, 3, 4}));
}

void TestPageRangeParser::testDescendingAndOpenRanges() {
    // Open-ended start: "-3" -> 1 through 3
    auto openStart = PageRangeParser::Parse("-3", 5);
    QVERIFY(openStart.has_value());
    QCOMPARE(openStart.value, (std::vector<int>{0, 1, 2}));

    // Open-ended end: "4-" -> 4 through 5
    auto openEnd = PageRangeParser::Parse("4-", 5);
    QVERIFY(openEnd.has_value());
    QCOMPARE(openEnd.value, (std::vector<int>{3, 4}));

    // Descending range: "4-2"
    auto descRes = PageRangeParser::Parse("4-2", 5);
    QVERIFY(descRes.has_value());
    QCOMPARE(descRes.value, (std::vector<int>{3, 2, 1}));
}

void TestPageRangeParser::testMixedRanges() {
    auto res = PageRangeParser::Parse("1-2, 4, 6-7", 8);
    QVERIFY(res.has_value());
    QCOMPARE(res.value, (std::vector<int>{0, 1, 3, 5, 6}));

    // Semicolon separator also supported
    auto resSemi = PageRangeParser::Parse("1; 3; 5", 5);
    QVERIFY(resSemi.has_value());
    QCOMPARE(resSemi.value, (std::vector<int>{0, 2, 4}));
}

void TestPageRangeParser::testOutOfBoundsAndInvalid() {
    // 0 is invalid for 1-based human pages
    auto zeroRes = PageRangeParser::Parse("0", 5);
    QVERIFY(!zeroRes.has_value());

    // Beyond total pages
    auto outRes = PageRangeParser::Parse("6", 5);
    QVERIFY(!outRes.has_value());

    auto outRange = PageRangeParser::Parse("2-7", 5);
    QVERIFY(!outRange.has_value());

    // Bad non-numeric syntax
    auto badRes = PageRangeParser::Parse("foo-bar", 5);
    QVERIFY(!badRes.has_value());

    // Invalid total pages
    auto badTotal = PageRangeParser::Parse("all", 0);
    QVERIFY(!badTotal.has_value());
}

void TestPageRangeParser::testFormatting() {
    std::vector<int> indices = {0, 1, 2, 4, 7, 8, 9};
    std::string formatted = PageRangeParser::Format(indices);
    QCOMPARE(QString::fromStdString(formatted), QString("1-3, 5, 8-10"));

    std::vector<int> single = {3};
    QCOMPARE(QString::fromStdString(PageRangeParser::Format(single)), QString("4"));

    std::vector<int> empty = {};
    QCOMPARE(QString::fromStdString(PageRangeParser::Format(empty)), QString(""));
}

QTEST_MAIN(TestPageRangeParser)
#include "test_page_range_parser.moc"
