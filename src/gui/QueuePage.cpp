#include "gui/QueuePage.h"

#include "gui/TableSupport.h"
#include "utils/ClinicException.h"
#include "utils/Validation.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace scapms::gui {

using model::PatientCategory;
using model::PatientRecord;
using utils::ClinicException;

namespace {

enum Column { ColPos, ColPatient, ColCategory, ColPriority, ColService, ColJoined, ColWaiting, ColCount };

QString q(const std::string& s) { return QString::fromUtf8(s.c_str()); }

struct Look { QColor rowBg; QColor badgeBg; QColor badgeFg; };
Look lookFor(PatientCategory c) {
    switch (c) {
        case PatientCategory::Emergency: return {QColor("#FEF3F2"), QColor("#FDE0DC"), QColor("#B42318")};
        case PatientCategory::FollowUp:  return {QColor("#FFFFFF"), QColor("#DBEAFE"), QColor("#1D4ED8")};
        case PatientCategory::General:   return {QColor("#FFFFFF"), QColor("#E8EEF1"), QColor("#3C4A53")};
    }
    return {QColor("#FFFFFF"), QColor("#E8EEF1"), QColor("#3C4A53")};
}

} // namespace

QueuePage::QueuePage(controller::ClinicManager& patients, controller::AppointmentController& appointments,
                     controller::QueueManager& queue, QWidget* parent, Clock clock)
    : QWidget(parent), m_patients(patients), m_appointments(appointments), m_queue(queue), m_clock(std::move(clock)) {
    buildUi();
    refresh();

    // Timer event: update waiting times and alert staff about long waits every 30 seconds.
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &QueuePage::onClockTick);
    m_timer->start(30 * 1000);
}

std::string QueuePage::now() const {
    if (m_clock) return m_clock();
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm").toStdString();
}

void QueuePage::buildUi() {
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // ---------------- left ----------------
    auto* left = new QVBoxLayout();
    left->setSpacing(12);
    auto* title = new QLabel(tr("Waiting Queue"), this);
    title->setObjectName("PageTitle");
    left->addWidget(title);

    auto* serving = new QFrame(this);
    serving->setObjectName("Card");
    auto* servLay = new QVBoxLayout(serving);
    servLay->setContentsMargins(20, 14, 20, 14);
    servLay->setSpacing(4);
    auto* servCaption = new QLabel(tr("NOW SERVING"), serving);
    servCaption->setObjectName("CardLabel");
    m_servingName = new QLabel(serving);
    m_servingName->setObjectName("ServingName");
    m_servingDetail = new QLabel(serving);
    m_servingDetail->setObjectName("Muted");
    m_servingDetail->setWordWrap(true);
    auto* servButtons = new QHBoxLayout();
    m_completeBtn = new QPushButton(tr("Complete consultation"), serving);
    m_completeBtn->setObjectName("Primary");
    m_completeBtn->setToolTip(tr("Mark the appointment completed and free the consulting room"));
    m_releaseBtn = new QPushButton(tr("Release"), serving);
    m_releaseBtn->setToolTip(tr("Stop serving without completing. The appointment stays pending."));
    servButtons->addWidget(m_completeBtn);
    servButtons->addWidget(m_releaseBtn);
    servButtons->addStretch(1);
    servLay->addWidget(servCaption);
    servLay->addWidget(m_servingName);
    servLay->addWidget(m_servingDetail);
    servLay->addLayout(servButtons);
    left->addWidget(serving);

    auto* waitingTitle = new QLabel(tr("Waiting (first in, first out)"), this);
    waitingTitle->setObjectName("SectionTitle");
    left->addWidget(waitingTitle);

    m_table = new QTableWidget(0, ColCount, this);
    m_table->setObjectName("QueueTable");
    m_table->setHorizontalHeaderLabels({tr("#"), tr("Patient"), tr("Category"), tr("Priority"), tr("Service"),
                                        tr("Joined"), tr("Waiting")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setWordWrap(false);
    m_table->setSortingEnabled(false);  // the queue order is first in, first out and must not change
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(40);
    configureResizableTable(m_table, {55, 170, 110, 105, 140, 145, 100},
                            {48, 130, 90, 90, 110, 120, 85});
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &QueuePage::onSelectionChanged);
    left->addWidget(m_table, 1);

    auto* fifo = new QLabel(tr("Emergency patients are highlighted but are not moved ahead: the queue stays first in, first out."), this);
    fifo->setObjectName("Muted");
    fifo->setWordWrap(true);
    left->addWidget(fifo);
    root->addLayout(left, 1);

    // ---------------- right ----------------
    auto* card = new QFrame(this);
    card->setObjectName("Card");
    card->setMinimumWidth(320);
    card->setMaximumWidth(335);
    auto* cardLay = new QVBoxLayout(card);
    cardLay->setContentsMargins(20, 18, 20, 18);
    cardLay->setSpacing(12);

    auto* addTitle = new QLabel(tr("Add to queue"), card);
    addTitle->setObjectName("SectionTitle");
    cardLay->addWidget(addTitle);

    m_hint = new QLabel(card);
    m_hint->setObjectName("HintLabel");
    m_hint->setWordWrap(true);
    m_hint->hide();
    cardLay->addWidget(m_hint);

    auto* form = new QFormLayout();
    form->setVerticalSpacing(10);
    m_patientCombo = new QComboBox(card);
    m_patientCombo->setObjectName("QueuePatientCombo");
    m_patientCombo->setToolTip(tr("Patients who have a pending appointment and are not already in the queue"));
    m_appointmentCombo = new QComboBox(card);
    m_appointmentCombo->setObjectName("QueueAppointmentCombo");
    m_appointmentCombo->setToolTip(tr("The pending appointment this visit is for"));
    form->addRow(tr("Patient"), m_patientCombo);
    form->addRow(tr("Appointment"), m_appointmentCombo);
    cardLay->addLayout(form);

    m_addBtn = new QPushButton(tr("Add to queue"), card);
    m_addBtn->setToolTip(tr("The patient has arrived and joins the end of the queue"));
    m_callBtn = new QPushButton(tr("Call next patient"), card);
    m_callBtn->setObjectName("Primary");
    m_callBtn->setToolTip(tr("Call the patient at the front of the queue"));
    m_removeBtn = new QPushButton(tr("Remove selected from queue"), card);
    m_removeBtn->setToolTip(tr("The selected patient leaves the queue. Their appointment is not changed."));
    cardLay->addWidget(m_addBtn);

    auto* line = new QFrame(card);
    line->setFrameShape(QFrame::HLine);
    line->setObjectName("Divider");
    cardLay->addWidget(line);
    cardLay->addWidget(m_callBtn);
    cardLay->addWidget(m_removeBtn);
    cardLay->addStretch(1);
    root->addWidget(card, 0);

    connect(m_patientCombo, &QComboBox::currentIndexChanged, this, &QueuePage::onPatientComboChanged);
    connect(m_addBtn, &QPushButton::clicked, this, &QueuePage::onAddClicked);
    connect(m_callBtn, &QPushButton::clicked, this, &QueuePage::onCallNextClicked);
    connect(m_removeBtn, &QPushButton::clicked, this, &QueuePage::onRemoveClicked);
    connect(m_completeBtn, &QPushButton::clicked, this, &QueuePage::onCompleteClicked);
    connect(m_releaseBtn, &QPushButton::clicked, this, &QueuePage::onReleaseClicked);
}

// ---------------------------------------------------------------- refresh
void QueuePage::refresh() {
    fillPatientCombo();
    fillServing();
    fillTable();
    updateButtons();
}

void QueuePage::fillPatientCombo() {
    const QVariant keep = m_patientCombo->currentData();
    m_patientCombo->blockSignals(true);
    m_patientCombo->clear();
    for (const auto& p : m_patients.patients()) {  // base-class pointers
        if (!p->hasPendingAppointments() || m_queue.isWaiting(p->id()) || m_queue.isBeingServed(p->id())) continue;
        m_patientCombo->addItem(QString("%1 (%2)").arg(q(p->name()), q(p->id())), q(p->id()));
        m_patientCombo->setItemData(m_patientCombo->count() - 1, q(p->categoryName()) + tr(" patient"), Qt::ToolTipRole);
    }
    const int idx = m_patientCombo->findData(keep);
    if (idx >= 0) m_patientCombo->setCurrentIndex(idx);
    m_patientCombo->blockSignals(false);
    fillAppointmentCombo();

    if (m_patients.patientCount() == 0)
        m_hint->setText(tr("No patients yet. Register a patient and schedule an appointment first."));
    else if (m_patientCombo->count() == 0)
        m_hint->setText(tr("No one to add. A patient needs a pending appointment (see Appointments) and must not already be in the queue."));
    m_hint->setVisible(m_patientCombo->count() == 0);
}

void QueuePage::fillAppointmentCombo() {
    m_appointmentCombo->clear();
    const std::string patientId = m_patientCombo->currentData().toString().toStdString();
    if (patientId.empty()) return;
    int best = 0;
    std::string bestTime;
    for (const auto& a : m_patients.getPatient(patientId).appointments()) {
        if (a.status() != model::AppointmentStatus::Pending) continue;
        m_appointmentCombo->addItem(QString("%1 - %2 - %3").arg(q(a.id()), q(a.serviceNameSnapshot()), q(a.dateTime())),
                                    q(a.id()));
        if (bestTime.empty() || a.dateTime() < bestTime) {  // preselect the earliest pending appointment
            bestTime = a.dateTime();
            best = m_appointmentCombo->count() - 1;
        }
    }
    m_appointmentCombo->setCurrentIndex(best);
}

void QueuePage::onPatientComboChanged() { fillAppointmentCombo(); updateButtons(); }

void QueuePage::fillServing() {
    const auto& serving = m_queue.nowServing();
    if (!serving) {
        m_servingName->setText(tr("No patient is being served"));
        m_servingDetail->setText(tr("Use \"Call next patient\" to bring in the next person."));
        return;
    }
    try {
        const PatientRecord& p = m_patients.getPatient(serving->patientId);
        const auto* appt = p.findAppointment(serving->appointmentId);
        m_servingName->setText(QString("%1 (%2)").arg(q(p.name()), q(p.id())));
        m_servingDetail->setText(tr("%1 - %2 | %3 | appointment %4")
                                     .arg(q(p.categoryName()), q(p.priorityLabel()),
                                          appt ? q(appt->serviceNameSnapshot()) : QString("?"), q(serving->appointmentId)));
    } catch (const ClinicException&) {
        m_servingName->setText(tr("Unknown patient"));
        m_servingDetail->clear();
    }
}

void QueuePage::fillTable() {
    const std::string nowStr = now();
    const auto entries = m_queue.waiting();
    int longWaits = 0;

    m_table->blockSignals(true);
    m_table->clearContents();
    m_table->setRowCount(static_cast<int>(entries.size()));
    int reselect = -1;
    for (int r = 0; r < static_cast<int>(entries.size()); ++r) {
        const auto& e = entries[static_cast<std::size_t>(r)];
        const PatientRecord& p = m_patients.getPatient(e.patientId);  // base-class reference
        const Look look = lookFor(p.category());
        const auto* appt = p.findAppointment(e.appointmentId);

        long minutes = 0;
        try { minutes = utils::validation::minutesBetween(e.joinedAt, nowStr); } catch (const ClinicException&) {}
        if (minutes < 0) minutes = 0;
        if (minutes >= kLongWaitMinutes) ++longWaits;

        auto put = [&](int col, const QString& text, bool badge = false, bool center = false) {
            auto* item = new QTableWidgetItem(text);
            item->setBackground(badge ? QBrush(look.badgeBg) : QBrush(look.rowBg));
            if (badge) {
                item->setForeground(QBrush(look.badgeFg));
                QFont f = item->font();
                f.setBold(true);
                item->setFont(f);
            }
            if (center) item->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(r, col, item);
        };
        put(ColPos, QString::number(r + 1), false, true);
        m_table->item(r, ColPos)->setData(Qt::UserRole, q(e.patientId));
        put(ColPatient, q(p.name()));
        m_table->item(r, ColPatient)->setToolTip(q(p.id()));
        put(ColCategory, q(p.categoryName()), true, true);
        put(ColPriority, q(p.priorityLabel()), true, true);
        put(ColService, appt ? q(appt->serviceNameSnapshot()) : QString("-"));
        put(ColJoined, q(e.joinedAt).mid(11), false, true);  // show HH:MM only
        put(ColWaiting, tr("%1 min").arg(minutes), true, true);

        // waiting-time colour: amber from 15 min, red from 30 min (text as well as colour)
        auto* w = m_table->item(r, ColWaiting);
        if (minutes >= kLongWaitMinutes) { w->setBackground(QBrush(QColor("#FDE0DC"))); w->setForeground(QBrush(QColor("#B42318"))); }
        else if (minutes >= kWarnMinutes) { w->setBackground(QBrush(QColor("#FEF0C7"))); w->setForeground(QBrush(QColor("#93370D"))); }
        else { w->setBackground(QBrush(QColor("#E8EEF1"))); w->setForeground(QBrush(QColor("#3C4A53"))); }

        if (q(e.patientId) == m_selectedPatientId) reselect = r;
    }
    if (reselect >= 0) m_table->selectRow(reselect);
    m_table->blockSignals(false);
    if (reselect < 0) m_selectedPatientId.clear();

    // alert staff once when the number of long waiters changes
    if (longWaits > 0 && longWaits != m_lastAlertCount)
        emit statusMessage(tr("%1 patient(s) have waited %2 minutes or more").arg(longWaits).arg(kLongWaitMinutes));
    m_lastAlertCount = longWaits;
}

void QueuePage::updateButtons() {
    const bool serving = m_queue.nowServing().has_value();
    m_completeBtn->setEnabled(serving);
    m_releaseBtn->setEnabled(serving);
    m_addBtn->setEnabled(m_patientCombo->count() > 0 && m_appointmentCombo->count() > 0);
    m_callBtn->setEnabled(m_queue.size() > 0 && !serving);
    m_callBtn->setToolTip(serving ? tr("Finish or release the current consultation first")
                                  : tr("Call the patient at the front of the queue"));
    m_removeBtn->setEnabled(!m_selectedPatientId.isEmpty());
}

void QueuePage::onSelectionChanged() {
    const auto rows = m_table->selectionModel()->selectedRows();
    m_selectedPatientId = rows.isEmpty() ? QString()
                                         : m_table->item(rows.first().row(), ColPos)->data(Qt::UserRole).toString();
    updateButtons();
}

void QueuePage::onClockTick() { fillTable(); }

// ---------------------------------------------------------------- actions
void QueuePage::onAddClicked() {
    const std::string appointmentId = m_appointmentCombo->currentData().toString().toStdString();
    if (!runGuarded([&] { m_queue.join(appointmentId, now()); })) return;
    emit queueChanged();
    emit statusMessage(tr("Added to the waiting queue"));
    refresh();
}

void QueuePage::onCallNextClicked() {
    controller::QueueEntry entry;
    if (!runGuarded([&] { entry = m_queue.callNext(); })) return;
    emit queueChanged();
    emit statusMessage(tr("Called %1").arg(q(entry.patientId)));
    refresh();
}

void QueuePage::onRemoveClicked() {
    if (m_selectedPatientId.isEmpty()) return;
    const std::string id = m_selectedPatientId.toStdString();
    QString name;
    if (!runGuarded([&] { name = q(m_patients.getPatient(id).name()); })) return;
    const auto answer = QMessageBox::question(
        this, tr("Remove from queue"),
        tr("Remove %1 from the waiting queue?\n\nTheir appointment is not changed.").arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;
    if (!runGuarded([&] { m_queue.remove(id); })) return;
    m_selectedPatientId.clear();
    emit queueChanged();
    emit statusMessage(tr("Removed %1 from the queue").arg(name));
    refresh();
}

void QueuePage::onCompleteClicked() {
    const auto& serving = m_queue.nowServing();
    if (!serving) return;
    const std::string appointmentId = serving->appointmentId;
    bool ok = false;
    const QString note = QInputDialog::getText(this, tr("Complete consultation"),
                                               tr("Outcome note for %1 (optional):").arg(q(appointmentId)),
                                               QLineEdit::Normal, QString(), &ok);
    if (!ok) return;
    // completing closes the appointment, which also clears "now serving" via the controller callback
    if (!runGuarded([&] { m_appointments.complete(appointmentId, note.toStdString()); })) return;
    emit appointmentsChanged();
    emit statusMessage(tr("Consultation completed"));
    refresh();
}

void QueuePage::onReleaseClicked() {
    if (!runGuarded([&] { m_queue.release(); })) return;
    emit queueChanged();
    emit statusMessage(tr("Released. The appointment is still pending."));
    refresh();
}

bool QueuePage::runGuarded(const std::function<void()>& action) {
    QString message;
    try {
        action();
        return true;
    } catch (const ClinicException& e) {
        message = q(e.what());
    } catch (const std::exception& e) {
        message = tr("Unexpected error: %1. No changes were made.").arg(q(e.what()));
    } catch (...) {
        message = tr("An unknown error occurred. No changes were made.");
    }
    QMessageBox::warning(this, tr("Cannot complete this action"), message);
    return false;
}

} // namespace scapms::gui
