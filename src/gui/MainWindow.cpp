#include "gui/MainWindow.h"

#include "gui/DashboardPage.h"
#include "gui/AdminPage.h"
#include "gui/AppointmentPage.h"
#include "gui/DataCoordinator.h"
#include "gui/PatientPage.h"
#include "gui/QueuePage.h"
#include "gui/ReportPage.h"
#include "gui/SearchPage.h"
#include "utils/ClinicException.h"
#include "utils/Validation.h"

#include <QAction>
#include <QCloseEvent>
#include <QFileInfo>
#include <QDate>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QFontMetrics>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>

namespace scapms::gui {

namespace {
// Sidebar pages, in the same order as the pages in the QStackedWidget.
const char* const kNav[] = {"Dashboard", "Patients", "Appointments", "Waiting Queue", "Search & Sort", "Reports", "Administration"};
constexpr int kNavCount = static_cast<int>(sizeof(kNav) / sizeof(kNav[0]));
} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    resize(1280, 800);
    m_coordinator = new DataCoordinator(m_database, m_idGenerator, m_undo, m_queue, m_cache, this, this);
    m_coordinator->setDirtyProvider([this] { return m_dirty; });
    buildMenus();
    buildContent();

    m_statusPage = new QLabel(tr("Dashboard"), this);
    m_statusSaved = new QLabel(tr("Not saved yet"), this);
    statusBar()->addWidget(m_statusPage, 1);
    statusBar()->addPermanentWidget(m_statusSaved);

    updateTitle();
    refreshAll();
    m_coordinator->loadStartupFile();  // open the last saved data, if any
}

void MainWindow::buildMenus() {
    auto* file = menuBar()->addMenu(tr("&File"));

    auto* actNew = file->addAction(tr("&New"));
    actNew->setShortcut(QKeySequence::New);  // Ctrl+N
    connect(actNew, &QAction::triggered, this, [this] { m_coordinator->newData(); });

    auto* actLoad = file->addAction(tr("&Load..."));
    actLoad->setShortcut(QKeySequence::Open);  // Ctrl+O
    connect(actLoad, &QAction::triggered, this, [this] { m_coordinator->load(); });

    auto* actSave = file->addAction(tr("&Save"));
    actSave->setShortcut(QKeySequence::Save);  // Ctrl+S
    connect(actSave, &QAction::triggered, this, [this] { m_coordinator->save(); });

    auto* actSaveAs = file->addAction(tr("Save &As..."));
    actSaveAs->setShortcut(QKeySequence::SaveAs);  // Ctrl+Shift+S
    connect(actSaveAs, &QAction::triggered, this, [this] { m_coordinator->saveAs(); });

    auto* actExport = file->addAction(tr("&Export patients (CSV)..."));
    connect(actExport, &QAction::triggered, this, [this] { m_coordinator->exportPatientsCsv(); });

    file->addSeparator();
    auto* actExit = file->addAction(tr("E&xit"));
    actExit->setShortcut(QKeySequence::Quit);  // Ctrl+Q
    connect(actExit, &QAction::triggered, this, &QWidget::close);

    auto* edit = menuBar()->addMenu(tr("&Edit"));
    m_undoAction = edit->addAction(tr("&Undo last administrative operation"));
    m_undoAction->setShortcut(QKeySequence::Undo);  // Ctrl+Z
    m_undoAction->setEnabled(false);
    connect(m_undoAction, &QAction::triggered, this, &MainWindow::onUndoClicked);

    auto* actFind = edit->addAction(tr("&Find"));
    actFind->setShortcut(QKeySequence::Find);  // Ctrl+F
    connect(actFind, &QAction::triggered, this, [this] {
        m_searchBox->setFocus();
        m_searchBox->selectAll();
    });
}

void MainWindow::buildContent() {
    auto* central = new QWidget(this);
    auto* outer = new QHBoxLayout(central);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Left: title + sidebar navigation
    auto* left = new QWidget(central);
    left->setFixedWidth(220);
    left->setStyleSheet("background:#12343B;");
    auto* leftLay = new QVBoxLayout(left);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->setSpacing(0);
    auto* appTitle = new QLabel(tr("SCAPMS"), left);
    appTitle->setObjectName("AppTitle");
    auto* appSub = new QLabel(tr("Smart Clinic Management"), left);
    appSub->setObjectName("AppSubtitle");
    m_sidebar = new QListWidget(left);
    m_sidebar->setObjectName("Sidebar");
    m_sidebar->setFocusPolicy(Qt::NoFocus);
    for (const char* label : kNav) m_sidebar->addItem(tr(label));
    leftLay->addWidget(appTitle);
    leftLay->addWidget(appSub);
    leftLay->addWidget(m_sidebar, 1);

    // Right: top bar + stacked pages
    auto* right = new QWidget(central);
    right->setObjectName("Content");
    auto* rightLay = new QVBoxLayout(right);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->setSpacing(0);

    auto* topBar = new QFrame(right);
    topBar->setObjectName("TopBar");
    auto* topLay = new QHBoxLayout(topBar);
    topLay->setContentsMargins(24, 12, 24, 12);
    topLay->setSpacing(8);

    m_searchBox = new QLineEdit(topBar);
    m_searchBox->setPlaceholderText(tr("Search patients by name or ID (Ctrl+F)"));
    m_searchBox->setClearButtonEnabled(true);
    m_searchBox->setMinimumWidth(320);
    m_searchBox->setToolTip(tr("Type a patient name or ID. Full search options are on the Search & Sort page."));
    connect(m_searchBox, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);

    m_undoButton = new QPushButton(tr("Undo"), topBar);
    m_undoButton->setEnabled(false);  // enabled when the undo stack has an entry
    m_undoButton->setToolTip(tr("Undo the last administrative operation (Ctrl+Z)"));
    connect(m_undoButton, &QPushButton::clicked, this, &MainWindow::onUndoClicked);

    m_saveButton = new QPushButton(tr("Save"), topBar);
    m_saveButton->setObjectName("Primary");
    m_saveButton->setToolTip(tr("Save all data to the JSON file (Ctrl+S)"));
    connect(m_saveButton, &QPushButton::clicked, this, [this] { m_coordinator->save(); });

    topLay->addWidget(m_searchBox);
    topLay->addStretch(1);
    topLay->addWidget(m_undoButton);
    topLay->addWidget(m_saveButton);

    m_pages = new QStackedWidget(right);
    m_dashboard = new DashboardPage(m_pages);
    m_pages->addWidget(m_dashboard);
    m_patientPage = new PatientPage(m_manager, m_pages);
    m_pages->addWidget(m_patientPage);
    m_appointmentPage = new AppointmentPage(m_manager, m_appointmentController, m_pages);
    m_pages->addWidget(m_appointmentPage);
    m_queuePage = new QueuePage(m_manager, m_appointmentController, m_queue, m_pages);
    m_pages->addWidget(m_queuePage);
    m_searchPage = new SearchPage(m_manager, m_pages);
    m_pages->addWidget(m_searchPage);
    m_reportPage = new ReportPage(m_database, m_pages);
    m_pages->addWidget(m_reportPage);
    m_adminPage = new AdminPage(m_pages);
    m_pages->addWidget(m_adminPage);
    connect(m_reportPage, &ReportPage::statusMessage, this,
            [this](const QString& text) { statusBar()->showMessage(text, 5000); });
    connect(m_adminPage, &AdminPage::saveRequested, this, [this] { m_coordinator->save(); });
    connect(m_adminPage, &AdminPage::saveAsRequested, this, [this] { m_coordinator->saveAs(); });
    connect(m_adminPage, &AdminPage::loadRequested, this, [this] { m_coordinator->load(); });
    connect(m_adminPage, &AdminPage::exportRequested, this, [this] { m_coordinator->exportPatientsCsv(); });
    connect(m_adminPage, &AdminPage::newRequested, this, [this] { m_coordinator->newData(); });
    connect(m_adminPage, &AdminPage::demoRequested, this, [this] { m_coordinator->loadDemo(); });
    connect(m_adminPage, &AdminPage::undoRequested, this, &MainWindow::onUndoClicked);

    // Whole-data workflows: after a load / new / demo everything on screen must be rebuilt.
    connect(m_coordinator, &DataCoordinator::dataReplaced, this, &MainWindow::onDataReplaced);
    connect(m_coordinator, &DataCoordinator::dataSaved, this, [this] {
        setDirty(false);
        m_statusSaved->setText(tr("Saved %1").arg(QTime::currentTime().toString("HH:mm:ss")));
    });
    connect(m_coordinator, &DataCoordinator::pathChanged, this, [this] { updateTitle(); refreshAdmin(); });
    connect(m_coordinator, &DataCoordinator::statusMessage, this,
            [this](const QString& text) { statusBar()->showMessage(text, 6000); });
    connect(m_patientPage, &PatientPage::patientsChanged, this, &MainWindow::onPatientsChanged);
    connect(m_appointmentPage, &AppointmentPage::appointmentsChanged, this, &MainWindow::onAppointmentsChanged);
    connect(m_appointmentPage, &AppointmentPage::statusMessage, this,
            [this](const QString& text) { statusBar()->showMessage(text, 4000); });
    connect(m_queuePage, &QueuePage::queueChanged, this, &MainWindow::refreshDashboard);  // queue is not saved data
    connect(m_queuePage, &QueuePage::appointmentsChanged, this, &MainWindow::onAppointmentsChanged);
    connect(m_queuePage, &QueuePage::statusMessage, this,
            [this](const QString& text) { statusBar()->showMessage(text, 6000); });

    connect(m_searchPage, &SearchPage::patientViewed, this, &MainWindow::onPatientViewed);
    connect(m_patientPage, &PatientPage::patientViewed, this, &MainWindow::onPatientViewed);
    connect(m_searchPage, &SearchPage::statusMessage, this,
            [this](const QString& text) { statusBar()->showMessage(text, 5000); });
    // The cache holds non-owning pointers: forget a patient just before the object is deleted or replaced.
    m_database.setInvalidationCallback([this](const std::string& id) { m_cache.invalidate(id); });

    // Timer event: refresh dashboard statistics regularly (e.g. when the day changes).
    m_dashboardTimer = new QTimer(this);
    connect(m_dashboardTimer, &QTimer::timeout, this, &MainWindow::refreshDashboard);
    m_dashboardTimer->start(30 * 1000);

    // A cancelled / completed / removed appointment must leave the waiting queue.
    m_appointmentController.setAppointmentClosedCallback(
        [this](const std::string& id) { m_queue.onAppointmentClosed(id); });
    // Keep the Undo button and menu text in step with the undo stack.
    m_undo.setChangedCallback([this] { updateUndoControls(); });
    connect(m_patientPage, &PatientPage::statusMessage, this,
            [this](const QString& text) { statusBar()->showMessage(text, 4000); });

    rightLay->addWidget(topBar);
    rightLay->addWidget(m_pages, 1);

    outer->addWidget(left);
    outer->addWidget(right, 1);
    setCentralWidget(central);

    connect(m_sidebar, &QListWidget::currentRowChanged, this, &MainWindow::onNavigationChanged);
    m_sidebar->setCurrentRow(0);
}

void MainWindow::setDirty(bool dirty) {
    m_dirty = dirty;
    updateTitle();
    refreshAdmin();
}

void MainWindow::onDataReplaced(bool markDirty) {
    setDirty(markDirty);
    m_statusSaved->setText(markDirty ? tr("Not saved yet") : tr("All changes saved"));
    refreshAll();
    updateUndoControls();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_coordinator->confirmSaveOrDiscard()) event->accept();
    else event->ignore();  // Cancel was chosen
}

void MainWindow::refreshAdmin() {
    if (!m_adminPage) return;
    AdminInfo info;
    info.path = m_coordinator ? m_coordinator->currentPath() : QString();
    info.dirty = m_dirty;
    info.patients = m_manager.patientCount();
    for (const auto& p : m_database.patients()) info.appointments += static_cast<int>(p->appointments().size());
    info.services = static_cast<int>(m_database.services().size());
    info.history = static_cast<int>(m_database.history().size());
    for (const auto& d : m_undo.descriptions()) info.undoLines << QString::fromUtf8(d.c_str());
    m_adminPage->setInfo(info);
}

void MainWindow::onPatientsChanged() {
    setDirty(true);
    m_appointmentPage->refresh();  // patient list, names and the table may have changed
    m_queuePage->refresh();
    m_searchPage->refresh();       // rebuilds its list when patient objects changed
    m_reportPage->refresh();
    refreshDashboard();
}

void MainWindow::onAppointmentsChanged() {
    setDirty(true);
    m_patientPage->refresh();      // appointment counts in the patient table
    m_appointmentPage->refresh();
    m_queuePage->refresh();        // the queue may have lost an entry
    m_searchPage->refresh();       // appointment counts
    m_reportPage->refresh();
    refreshDashboard();
}

void MainWindow::refreshAll() {
    m_patientPage->refresh();
    m_appointmentPage->refresh();
    m_queuePage->refresh();
    m_searchPage->refresh();
    m_reportPage->refresh();
    refreshAdmin();
    refreshDashboard();
}

void MainWindow::onPatientViewed(const QString& patientId) {
    try {
        m_cache.recordAccess(m_manager.getPatient(patientId.toStdString()));
    } catch (...) {
        return;  // the patient disappeared in the meantime; nothing to record
    }
    refreshDashboard();
}

void MainWindow::updateUndoControls() {
    const bool can = m_undo.canUndo();
    const QString full = QString::fromUtf8(m_undo.nextDescription().c_str());
    m_undoButton->setEnabled(can);
    m_undoAction->setEnabled(can);
    if (can) {
        const QFontMetrics fm(m_undoButton->font());
        m_undoButton->setText(tr("Undo: %1").arg(fm.elidedText(full, Qt::ElideRight, 240)));
        m_undoButton->setToolTip(tr("Undo: %1 (Ctrl+Z)").arg(full));
        m_undoAction->setText(tr("&Undo: %1").arg(full));
    } else {
        m_undoButton->setText(tr("Undo"));
        m_undoButton->setToolTip(tr("Nothing to undo"));
        m_undoAction->setText(tr("&Undo last administrative operation"));
    }
    refreshAdmin();  // the undo history list on the Administration page
}

void MainWindow::refreshDashboard() {
    using scapms::model::PatientCategory;
    const QDateTime nowTime = QDateTime::currentDateTime();
    const std::string today = nowTime.date().toString("yyyy-MM-dd").toStdString();
    const std::string now = nowTime.toString("yyyy-MM-dd HH:mm").toStdString();
    auto q = [](const std::string& s) { return QString::fromUtf8(s.c_str()); };

    DashboardData d;
    d.patients = m_manager.patientCount();
    d.todayAppointments = m_appointmentController.countOnDate(today);
    d.pending = m_manager.pendingAppointmentCount();
    d.waiting = static_cast<int>(m_queue.size());
    d.general = m_manager.countByCategory(PatientCategory::General);
    d.emergency = m_manager.countByCategory(PatientCategory::Emergency);
    d.followUp = m_manager.countByCategory(PatientCategory::FollowUp);

    // today's appointments, earliest first (the controller already returns them in time order)
    int more = 0;
    for (const auto& row : m_appointmentController.allAppointments(now)) {
        const auto& a = row.appointment;
        if (scapms::utils::validation::datePart(a.dateTime()) != today ||
            a.status() == scapms::model::AppointmentStatus::Cancelled)
            continue;
        if (d.todayLines.size() >= 6) { ++more; continue; }
        d.todayLines << QString("%1   %2 - %3  (%4)").arg(q(a.dateTime()).mid(11), q(a.patientNameSnapshot()),
                                                         q(a.serviceNameSnapshot()), q(row.displayStatus));
    }
    if (more > 0) d.todayLines << tr("+ %1 more").arg(more);

    // queue: who is being served, then the first few waiting
    if (m_queue.nowServing()) {
        try {
            d.queueLines << tr("Now serving: %1").arg(q(m_manager.getPatient(m_queue.nowServing()->patientId).name()));
        } catch (...) {}
    }
    int pos = 0;
    for (const auto& e : m_queue.waiting()) {
        if (++pos > 5) { d.queueLines << tr("+ %1 more waiting").arg(static_cast<int>(m_queue.size()) - 5); break; }
        try {
            const auto& p = m_manager.getPatient(e.patientId);
            d.queueLines << QString("%1. %2 (%3)").arg(pos).arg(q(p.name()), q(p.categoryName()));
        } catch (...) {}
    }

    // frequently accessed patients from the 10-slot cache (most used first)
    int shown = 0;
    for (const auto& entry : m_cache.mostAccessed()) {
        if (++shown > 6) break;
        d.frequentLines << tr("%1 (%2) - %3 view(s)").arg(q(entry.patient->name()), q(entry.patient->id())).arg(entry.hits);
    }
    m_dashboard->setData(d);
}

int MainWindow::currentPage() const { return m_pages->currentIndex(); }

void MainWindow::updateTitle() {
    const QString path = m_coordinator ? m_coordinator->currentPath() : QString();
    const QString name = path.isEmpty() ? tr("Untitled") : QFileInfo(path).fileName();
    setWindowTitle(QString("SCAPMS - %1%2").arg(name, m_dirty ? QString::fromUtf8(" \u25CF") : QString()));
}

void MainWindow::onNavigationChanged(int row) {
    if (row < 0) return;
    m_pages->setCurrentIndex(row);
    if (m_statusPage) m_statusPage->setText(m_sidebar->item(row)->text());
}

void MainWindow::onUndoClicked() {
    if (!m_undo.canUndo()) {
        statusBar()->showMessage(tr("Nothing to undo"), 3000);
        return;
    }
    QString message;
    try {
        const std::string what = m_undo.undoLast();
        setDirty(true);
        refreshAll();
        statusBar()->showMessage(tr("Undone: %1").arg(QString::fromUtf8(what.c_str())), 5000);
        return;
    } catch (const scapms::utils::ClinicException& e) {
        message = QString::fromUtf8(e.what());
    } catch (const std::exception& e) {
        message = tr("Unexpected error: %1").arg(e.what());
    } catch (...) {
        message = tr("An unknown error occurred while undoing.");
    }
    refreshAll();  // a discarded entry may still have changed what is on screen
    QMessageBox::warning(this, tr("Cannot undo"), message);
}

void MainWindow::onSearchTextChanged(const QString& text) {
    // Typing in the top-bar search box shows the results on the Search & Sort page.
    if (!text.trimmed().isEmpty()) m_sidebar->setCurrentRow(4);
    m_searchPage->quickSearch(text);
}

} // namespace scapms::gui
