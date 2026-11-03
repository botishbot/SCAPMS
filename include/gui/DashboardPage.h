#pragma once
#include <QStringList>
#include <QWidget>

class QLabel;
class QListWidget;
class QProgressBar;

namespace scapms::gui {

// Everything the dashboard shows. MainWindow fills it from the controllers (the GUI owns no business logic).
struct DashboardData {
    int patients = 0;
    int todayAppointments = 0;
    int pending = 0;
    int waiting = 0;
    int general = 0;
    int emergency = 0;
    int followUp = 0;
    QStringList todayLines;     // today's appointments
    QStringList queueLines;     // now serving + waiting queue
    QStringList frequentLines;  // frequently accessed patients (from the cache)
};

// Dashboard: summary cards (QGridLayout) and four panels.
class DashboardPage : public QWidget {
    Q_OBJECT
public:
    explicit DashboardPage(QWidget* parent = nullptr);

    void setData(const DashboardData& data);

private:
    QWidget* makeCard(const QString& caption, QLabel*& valueLabel);
    QWidget* makePanel(const QString& title, QListWidget*& list, const char* objectName);
    QWidget* makeDistributionPanel();
    static void fillList(QListWidget* list, const QStringList& lines, const QString& emptyText);

    QLabel* m_patients = nullptr;
    QLabel* m_today = nullptr;
    QLabel* m_pending = nullptr;
    QLabel* m_waiting = nullptr;
    QListWidget* m_todayList = nullptr;
    QListWidget* m_queueList = nullptr;
    QListWidget* m_frequentList = nullptr;
    QProgressBar* m_barGeneral = nullptr;
    QProgressBar* m_barEmergency = nullptr;
    QProgressBar* m_barFollowUp = nullptr;
    QLabel* m_countGeneral = nullptr;
    QLabel* m_countEmergency = nullptr;
    QLabel* m_countFollowUp = nullptr;
};

} // namespace scapms::gui
