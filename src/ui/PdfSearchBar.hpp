// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>

namespace pdfinity::ui {

class PdfSearchBar : public QWidget {
    Q_OBJECT

public:
    explicit PdfSearchBar(QWidget* parent = nullptr);
    ~PdfSearchBar() override = default;

    void OpenSearch();
    void CloseSearch();
    void SetMatchInfo(int currentMatch, int totalMatches);

signals:
    void SearchRequested(const QString& query, bool matchCase, bool matchWholeWord);
    void FindNextRequested();
    void FindPrevRequested();
    void SearchClosed();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void OnSearchTriggered();

private:
    void SetupUi();

    QLineEdit* m_inputField{nullptr};
    QLabel* m_matchCountLabel{nullptr};
    QPushButton* m_prevBtn{nullptr};
    QPushButton* m_nextBtn{nullptr};
    QCheckBox* m_caseCheck{nullptr};
    QCheckBox* m_wordCheck{nullptr};
    QPushButton* m_closeBtn{nullptr};
};

} // namespace pdfinity::ui
