#pragma once
#include "controller/AppointmentController.h"
#include "controller/ClinicManager.h"
#include "controller/PatientCache.h"
#include "controller/QueueManager.h"
#include "controller/UndoManager.h"
#include "model/ClinicDatabase.h"
#include "utils/IDGenerator.h"

#include <QMainWindow>

class QCloseEvent;

class QListWidget;
class QStackedWidget;
class QLineEdit;
class QPushButton;
class QLabel;
class QAction;
class QTimer;

namespace scapms::gui {

class DashboardPage;
class PatientPage;
class AppointmentPage;
class QueuePage;
class SearchPage;
class ReportPage;
class AdminPage;
class DataCoordinator;

// Main window: sidebar navigation (QHBoxLayout) + stacked pages (QStackedWidget)
// + top bar with Undo / Save / search (QVBoxLayout). Business logic lives in
// controllers, not here.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    void setDirty(bool dirty);
    int currentPage() const;

protected:
    void closeEvent(QCloseEvent* event) override;  // asks about unsaved changes

private slots:
    void onNavigationChanged(int row);
    void onSearchTextChanged(const QString& text);
    void onUndoClicked();
    void onPatientsChanged();
    void onAppointmentsChanged();
    void onPatientViewed(const QString& patientId);
    void onDataReplaced(bool markDirty);

private:
    void buildMenus();
    void buildContent();
    void updateTitle();
    void refreshDashboard();
    void refreshAll();
    void updateUndoControls();
    void refreshAdmin();

    QListWidget* m_sidebar = nullptr;
    QStackedWidget* m_pages = nullptr;
    QLineEdit* m_searchBox = nullptr;
    QPushButton* m_undoButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    QLabel* m_statusPage = nullptr;
    QLabel* m_statusSaved = nullptr;
    DashboardPage* m_dashboard = nullptr;
    PatientPage* m_patientPage = nullptr;
    AppointmentPage* m_appointmentPage = nullptr;
    QueuePage* m_queuePage = nullptr;
    SearchPage* m_searchPage = nullptr;
    ReportPage* m_reportPage = nullptr;
    AdminPage* m_adminPage = nullptr;
    DataCoordinator* m_coordinator = nullptr;
    QTimer* m_dashboardTimer = nullptr;
    QAction* m_undoAction = nullptr;

    // Application data and controllers (declared in dependency order)
    scapms::model::ClinicDatabase m_database;
    scapms::utils::IDGenerator m_idGenerator;
    scapms::controller::UndoManager m_undo{m_database};
    scapms::controller::ClinicManager m_manager{m_database, m_idGenerator, m_undo};
    scapms::controller::AppointmentController m_appointmentController{m_database, m_idGenerator, m_undo};
    scapms::controller::QueueManager m_queue{m_database};
    scapms::controller::PatientCache m_cache;
    bool m_dirty = false;
};

} // namespace scapms::gui
