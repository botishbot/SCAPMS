#include "gui/ReportPage.h"

#include "controller/ReportGenerator.h"
#include "utils/ClinicException.h"
#include "utils/FileHandler.h"

#include <QAbstractItemView>
#include <QDate>
#include <QDateEdit>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

using controller::ReportGenerator;

namespace {
QString q(const std::string& s) { return QString::fromUtf8(s.c_str()); }

QTableWidget* makeTable(QWidget* parent, const char* name, const QStringList& headers) {
    auto* t = new QTableWidget(0, headers.size(), parent);
    t->setObjectName(name);
    t->setHorizontalHeaderLabels(headers);
    t->setSelectionMode(QAbstractItemView::NoSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setFocusPolicy(Qt::NoFocus);
    t->setShowGrid(false);
    t->setWordWrap(false);
    t->setSortingEnabled(false);
    t->verticalHeader()->setVisible(false);
    t->verticalHeader()->setDefaultSectionSize(32);
    for (int c = 0; c < headers.size(); ++c)
        t->horizontalHeader()->setSectionResizeMode(c, c == 1 ? QHeaderView::Stretch : QHeaderView::ResizeToContents);
    t->setFrameShape(QFrame::NoFrame);
    t->setMinimumHeight(235);
    return t;
}
} // namespace

ReportPage::ReportPage(const model::ClinicDatabase& database, QWidget* parent) : QWidget(parent), m_database(database) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(24, 24, 24, 24);
    outer->setSpacing(12);

    auto* head = new QHBoxLayout();
    auto* title = new QLabel(tr("Reports"), this);
    title->setObjectName("PageTitle");
    head->addWidget(title);
    head->addStretch(1);
    head->addWidget(new QLabel(tr("Daily summary for"), this));
    m_date = new QDateEdit(QDate::currentDate(), this);
    m_date->setObjectName("ReportDateEdit");
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat("yyyy-MM-dd");
    m_date->setToolTip(tr("Choose the day for the Daily Appointment Summary"));
    head->addWidget(m_date);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    refreshBtn->setToolTip(tr("Recalculate all reports"));
    auto* exportBtn = new QPushButton(tr("Export reports..."), this);
    exportBtn->setObjectName("Primary");
    exportBtn->setToolTip(tr("Save all reports as a text file"));
    head->addWidget(refreshBtn);
    head->addWidget(exportBtn);
    outer->addLayout(head);

    auto* content = new QWidget(this);
    auto* grid = new QGridLayout(content);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(16);

    // 1. Most frequently attended patients
    m_frequent = makeTable(content, "FrequentTable", {tr("#"), tr("Patient"), tr("Category"), tr("Completed")});
    grid->addWidget(makeCard(tr("1. Most frequently attended patients"), m_frequent), 0, 0);

    // 2. Patients with pending appointments
    m_pending = makeTable(content, "PendingTable", {tr("ID"), tr("Patient"), tr("Pending"), tr("Next appointment")});
    grid->addWidget(makeCard(tr("2. Patients with pending appointments"), m_pending), 0, 1);

    // 3. Category distribution
    auto* distBody = new QWidget(content);
    auto* distLay = new QVBoxLayout(distBody);
    distLay->setContentsMargins(0, 0, 0, 0);
    distLay->setSpacing(12);
    const char* barNames[3] = {"RepBarGeneral", "RepBarEmergency", "RepBarFollowUp"};
    for (int i = 0; i < 3; ++i) {
        auto* row = new QVBoxLayout();
        row->setSpacing(4);
        m_barText[i] = new QLabel(distBody);
        m_bars[i] = new QProgressBar(distBody);
        m_bars[i]->setObjectName(barNames[i]);
        m_bars[i]->setRange(0, 100);
        m_bars[i]->setTextVisible(false);
        m_bars[i]->setFixedHeight(12);
        row->addWidget(m_barText[i]);
        row->addWidget(m_bars[i]);
        distLay->addLayout(row);
    }
    distLay->addStretch(1);
    grid->addWidget(makeCard(tr("3. Patient category distribution"), distBody), 1, 0);

    // 4. Daily appointment summary
    auto* dayBody = new QWidget(content);
    auto* dayLay = new QGridLayout(dayBody);
    dayLay->setContentsMargins(0, 0, 0, 0);
    dayLay->setHorizontalSpacing(16);
    auto addStat = [&](int col, const QString& caption, QLabel*& value, const char* name) {
        value = new QLabel("0", dayBody);
        value->setObjectName(name);
        value->setProperty("big", true);
        auto* cap = new QLabel(caption, dayBody);
        cap->setObjectName("CardLabel");
        dayLay->addWidget(value, 0, col);
        dayLay->addWidget(cap, 1, col);
    };
    addStat(0, tr("Total appointments"), m_dailyTotal, "DailyTotal");
    addStat(1, tr("Completed"), m_dailyCompleted, "DailyCompleted");
    addStat(2, tr("Pending"), m_dailyPending, "DailyPending");
    addStat(3, tr("Cancelled"), m_dailyCancelled, "DailyCancelled");
    auto* dayNote = new QLabel(tr("Pending includes appointments that are overdue. Appointments of patients deleted "
                                  "since still count for their day."), dayBody);
    dayNote->setObjectName("Muted");
    dayNote->setWordWrap(true);
    dayLay->addWidget(dayNote, 2, 0, 1, 4);
    dayLay->setRowStretch(3, 1);
    grid->addWidget(makeCard(tr("4. Daily appointment summary"), dayBody), 1, 1);

    // 5. Polymorphic overview
    m_overview = new QListWidget(content);
    m_overview->setObjectName("OverviewList");
    m_overview->setProperty("panel", true);
    m_overview->setSelectionMode(QAbstractItemView::NoSelection);
    m_overview->setFocusPolicy(Qt::NoFocus);
    m_overview->setFrameShape(QFrame::NoFrame);
    m_overview->setMinimumHeight(150);
    auto* overviewCard = makeCard(tr("5. Patient overview (every patient type processed through the same PatientRecord interface)"), m_overview);
    grid->addWidget(overviewCard, 2, 0, 1, 2);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);
    outer->addWidget(scroll, 1);

    connect(refreshBtn, &QPushButton::clicked, this, &ReportPage::refresh);
    connect(m_date, &QDateEdit::dateChanged, this, [this] { refresh(); });
    connect(exportBtn, &QPushButton::clicked, this, &ReportPage::onExportClicked);
    refresh();
}

QWidget* ReportPage::makeCard(const QString& title, QWidget* body) {
    auto* card = new QFrame(this);
    card->setObjectName("Card");
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(20, 14, 20, 14);
    lay->setSpacing(8);
    auto* t = new QLabel(title, card);
    t->setObjectName("SectionTitle");
    t->setWordWrap(true);
    lay->addWidget(t);
    lay->addWidget(body, 1);
    return card;
}

void ReportPage::refresh() {
    const ReportGenerator reports(m_database);

    // 1
    const auto freq = reports.mostFrequentlyAttended(5);
    m_frequent->clearContents();
    m_frequent->setRowCount(freq.empty() ? 1 : static_cast<int>(freq.size()));
    if (freq.empty()) {
        m_frequent->setItem(0, 1, new QTableWidgetItem(tr("No completed appointments yet")));
    }
    for (int r = 0; r < static_cast<int>(freq.size()); ++r) {
        const auto& row = freq[static_cast<std::size_t>(r)];
        m_frequent->setItem(r, 0, new QTableWidgetItem(QString::number(r + 1)));
        m_frequent->setItem(r, 1, new QTableWidgetItem(QString("%1 (%2)").arg(q(row.name), q(row.id))));
        m_frequent->setItem(r, 2, new QTableWidgetItem(q(row.category)));
        auto* n = new QTableWidgetItem(QString::number(row.completed));
        n->setTextAlignment(Qt::AlignCenter);
        m_frequent->setItem(r, 3, n);
    }

    // 2
    const auto pending = reports.patientsWithPending();
    m_pending->clearContents();
    m_pending->setRowCount(pending.empty() ? 1 : static_cast<int>(pending.size()));
    if (pending.empty()) m_pending->setItem(0, 1, new QTableWidgetItem(tr("No pending appointments")));
    for (int r = 0; r < static_cast<int>(pending.size()); ++r) {
        const auto& row = pending[static_cast<std::size_t>(r)];
        m_pending->setItem(r, 0, new QTableWidgetItem(q(row.id)));
        m_pending->setItem(r, 1, new QTableWidgetItem(q(row.name)));
        auto* n = new QTableWidgetItem(QString::number(row.pending));
        n->setTextAlignment(Qt::AlignCenter);
        m_pending->setItem(r, 2, n);
        m_pending->setItem(r, 3, new QTableWidgetItem(q(row.nextDateTime)));
    }

    // 3
    const auto dist = reports.categoryDistribution();
    for (int i = 0; i < 3 && i < static_cast<int>(dist.size()); ++i) {
        const auto& row = dist[static_cast<std::size_t>(i)];
        m_barText[i]->setText(QString("%1: %2  (%3%)").arg(q(row.name)).arg(row.count).arg(row.percent));
        m_bars[i]->setValue(row.percent);
    }

    // 4
    const auto day = reports.dailySummary(m_date->date().toString("yyyy-MM-dd").toStdString());
    m_dailyTotal->setText(QString::number(day.total));
    m_dailyCompleted->setText(QString::number(day.completed));
    m_dailyPending->setText(QString::number(day.pending));
    m_dailyCancelled->setText(QString::number(day.cancelled));

    // 5
    m_overview->clear();
    const auto overview = reports.patientOverview();
    if (overview.empty()) {
        auto* item = new QListWidgetItem(tr("No patients yet"));
        item->setFlags(Qt::NoItemFlags);
        m_overview->addItem(item);
    }
    for (const auto& line : overview) m_overview->addItem(q(line));
}

void ReportPage::onExportClicked() {
    const QString suggested = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
                              "/scapms_reports_" + m_date->date().toString("yyyy-MM-dd") + ".txt";
    QString path = m_chooser ? m_chooser(suggested)
                             : QFileDialog::getSaveFileName(this, tr("Export reports"), suggested, tr("Text files (*.txt)"));
    if (path.isEmpty()) return;
    if (QFileInfo(path).suffix().isEmpty()) path += ".txt";
    try {
        const std::string text = ReportGenerator(m_database).renderText(m_date->date().toString("yyyy-MM-dd").toStdString());
        utils::FileHandler::writeTextFile(path, text);
    } catch (const utils::ClinicException& e) {
        QMessageBox::warning(this, tr("Could not export"), q(e.what()));
        return;
    } catch (...) {
        QMessageBox::warning(this, tr("Could not export"), tr("An unexpected error occurred."));
        return;
    }
    emit statusMessage(tr("Reports exported to %1").arg(QFileInfo(path).fileName()));
}

} // namespace scapms::gui
