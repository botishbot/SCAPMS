#pragma once
#include "model/ClinicDatabase.h"

#include <QString>
#include <QWidget>
#include <functional>

class QDateEdit;
class QLabel;
class QListWidget;
class QProgressBar;
class QTableWidget;

namespace scapms::gui {

// Reports page: five reports calculated fresh from the data every time they are shown.
class ReportPage : public QWidget {
    Q_OBJECT
public:
    using PathChooser = std::function<QString(const QString& suggestedPath)>;  // lets tests skip the real dialog

    explicit ReportPage(const model::ClinicDatabase& database, QWidget* parent = nullptr);

    void refresh();
    void setPathChooser(PathChooser chooser) { m_chooser = std::move(chooser); }

signals:
    void statusMessage(const QString&);

private slots:
    void onExportClicked();

private:
    QWidget* makeCard(const QString& title, QWidget* body);

    const model::ClinicDatabase& m_database;
    PathChooser m_chooser;
    QDateEdit* m_date = nullptr;
    QTableWidget* m_frequent = nullptr;
    QTableWidget* m_pending = nullptr;
    QProgressBar* m_bars[3] = {nullptr, nullptr, nullptr};
    QLabel* m_barText[3] = {nullptr, nullptr, nullptr};
    QLabel* m_dailyTotal = nullptr;
    QLabel* m_dailyCompleted = nullptr;
    QLabel* m_dailyPending = nullptr;
    QLabel* m_dailyCancelled = nullptr;
    QListWidget* m_overview = nullptr;
};

} // namespace scapms::gui
