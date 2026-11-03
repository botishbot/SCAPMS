#pragma once
#include <QString>
#include <QStringList>
#include <QWidget>

class QLabel;
class QListWidget;
class QPushButton;

namespace scapms::gui {

struct AdminInfo {
    QString path;           // empty = no data file yet
    bool dirty = false;
    int patients = 0;
    int appointments = 0;
    int services = 0;
    int history = 0;
    QStringList undoLines;  // newest first
};

// Administration page: data file actions, demo data, undo history and a data summary.
// It only raises signals; DataCoordinator / MainWindow do the work.
class AdminPage : public QWidget {
    Q_OBJECT
public:
    explicit AdminPage(QWidget* parent = nullptr);

    void setInfo(const AdminInfo& info);

signals:
    void saveRequested();
    void saveAsRequested();
    void loadRequested();
    void exportRequested();
    void newRequested();
    void demoRequested();
    void undoRequested();

private:
    QWidget* makeCard(const QString& title, QWidget*& body);

    QLabel* m_path = nullptr;
    QLabel* m_status = nullptr;
    QListWidget* m_undoList = nullptr;
    QPushButton* m_undoBtn = nullptr;
    QLabel* m_summary = nullptr;
};

} // namespace scapms::gui
