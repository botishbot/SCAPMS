#include "gui/AppointmentPage.h"

#include "gui/ServiceDialog.h"
#include "utils/ClinicException.h"
#include "utils/Validation.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
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

using controller::AppointmentRow;
using utils::ClinicException;

namespace {

enum Column { ColId, ColPatient, ColService, ColWhen, ColStatus, ColReason, ColCount };

QString q(const std::string& s) { return QString::fromUtf8(s.c_str()); }

struct StatusLook { QColor bg; QColor fg; bool strike; };

// Custom presentation of appointment status (colour and text together).
StatusLook lookFor(const QString& status) {
    if (status == "Overdue") return {QColor("#FDE0DC"), QColor("#B42318"), false};
    if (status == "Completed") return {QColor("#D1FADF"), QColor("#05603A"), false};
    if (status == "Cancelled") return {QColor("#EEF2F4"), QColor("#6B7A84"), true};
    return {QColor("#FEF0C7"), QColor("#93370D"), false};  // Pending
}

} // namespace

AppointmentPage::AppointmentPage(controller::ClinicManager& patients,
                                 controller::AppointmentController& appointments, QWidget* parent, Clock clock)
    : QWidget(parent), m_patients(patients), m_appointments(appointments), m_clock(std::move(clock)) {
    buildUi();
    refresh();

    // Timer event: re-evaluate "Overdue" once a minute while the page is open.
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &AppointmentPage::onClockTick);
    m_timer->start(60 * 1000);
}

std::string AppointmentPage::now() const {
    if (m_clock) return m_clock();
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm").toStdString();
}

void AppointmentPage::buildUi() {
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // ---------------- left: header + table ----------------
    auto* left = new QVBoxLayout();
    left->setSpacing(12);
    auto* head = new QHBoxLayout();
    auto* title = new QLabel(tr("Appointments"), this);
    title->setObjectName("PageTitle");
    head->addWidget(title);
    head->addStretch(1);
    head->addWidget(new QLabel(tr("Show"), this));
    m_filter = new QComboBox(this);
    m_filter->setObjectName("StatusFilter");
    m_filter->addItems({tr("All"), tr("Pending"), tr("Overdue"), tr("Completed"), tr("Cancelled")});
    m_filter->setToolTip(tr("Filter the table by appointment status"));
    connect(m_filter, &QComboBox::currentIndexChanged, this, [this] { fillTable(); });
    head->addWidget(m_filter);
    left->addLayout(head);

    // shown when the list is limited to one patient (opened from the Search & Sort page)
    m_filterBanner = new QFrame(this);
    m_filterBanner->setObjectName("FilterBanner");
    auto* bannerLay = new QHBoxLayout(m_filterBanner);
    bannerLay->setContentsMargins(12, 6, 12, 6);
    m_filterLabel = new QLabel(m_filterBanner);
    auto* showAllBtn = new QPushButton(tr("Show all appointments"), m_filterBanner);
    bannerLay->addWidget(m_filterLabel, 1);
    bannerLay->addWidget(showAllBtn);
    m_filterBanner->hide();
    left->addWidget(m_filterBanner);
    connect(showAllBtn, &QPushButton::clicked, this, [this] { m_patientFilter.clear(); fillTable(); });

    m_table = new QTableWidget(0, ColCount, this);
    m_table->setObjectName("AppointmentTable");
    m_table->setHorizontalHeaderLabels({tr("ID"), tr("Patient"), tr("Service"), tr("Date & time"), tr("Status"),
                                        tr("Reason")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setWordWrap(false);
    m_table->setSortingEnabled(false);  // rows are ordered by the application, not by Qt's header sort
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(40);
    m_table->horizontalHeader()->setSectionResizeMode(ColReason, QHeaderView::Stretch);
    for (int c : {ColId, ColPatient, ColService, ColWhen, ColStatus})
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &AppointmentPage::onSelectionChanged);
    left->addWidget(m_table, 1);
    root->addLayout(left, 1);

    // ---------------- right: booking card ----------------
    auto* card = new QFrame(this);
    card->setObjectName("Card");
    card->setMinimumWidth(340);
    card->setMaximumWidth(360);
    auto* cardLay = new QVBoxLayout(card);
    cardLay->setContentsMargins(20, 18, 20, 18);
    cardLay->setSpacing(12);

    auto* formTitle = new QLabel(tr("Schedule appointment"), card);
    formTitle->setObjectName("SectionTitle");
    cardLay->addWidget(formTitle);

    m_hint = new QLabel(card);
    m_hint->setObjectName("HintLabel");
    m_hint->setWordWrap(true);
    m_hint->hide();
    cardLay->addWidget(m_hint);

    m_errorLabel = new QLabel(card);
    m_errorLabel->setObjectName("ErrorLabel");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    cardLay->addWidget(m_errorLabel);

    auto* form = new QFormLayout();
    form->setVerticalSpacing(10);
    m_patient = new QComboBox(card);
    m_patient->setObjectName("PatientCombo");
    m_patient->setToolTip(tr("Patient to book"));
    m_service = new QComboBox(card);
    m_service->setObjectName("ServiceCombo");
    m_service->setToolTip(tr("Only active services are listed"));
    m_when = new QDateTimeEdit(card);
    m_when->setObjectName("WhenEdit");
    m_when->setCalendarPopup(true);
    m_when->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_when->setToolTip(tr("Appointment date and time. Each patient type applies its own booking rule."));
    m_reason = new QLineEdit(card);
    m_reason->setObjectName("ReasonEdit");
    m_reason->setPlaceholderText(tr("Reason (optional)"));
    form->addRow(tr("Patient"), m_patient);
    form->addRow(tr("Service"), m_service);
    form->addRow(tr("When"), m_when);
    form->addRow(tr("Reason"), m_reason);
    cardLay->addLayout(form);

    m_scheduleBtn = new QPushButton(tr("Schedule appointment"), card);
    m_scheduleBtn->setObjectName("Primary");
    m_scheduleBtn->setToolTip(tr("Book the appointment"));
    m_manageBtn = new QPushButton(tr("Manage services..."), card);
    m_manageBtn->setToolTip(tr("Add, activate or deactivate clinic services"));
    cardLay->addWidget(m_scheduleBtn);
    cardLay->addWidget(m_manageBtn);

    auto* line = new QFrame(card);
    line->setFrameShape(QFrame::HLine);
    line->setObjectName("Divider");
    cardLay->addWidget(line);

    auto* selTitle = new QLabel(tr("Selected appointment"), card);
    selTitle->setObjectName("SectionTitle");
    cardLay->addWidget(selTitle);
    m_selectedInfo = new QLabel(card);
    m_selectedInfo->setObjectName("Muted");
    m_selectedInfo->setWordWrap(true);
    cardLay->addWidget(m_selectedInfo);

    m_completeBtn = new QPushButton(tr("Mark completed"), card);
    m_completeBtn->setToolTip(tr("The patient was seen. You can add an outcome note."));
    m_cancelBtn = new QPushButton(tr("Cancel appointment"), card);
    m_cancelBtn->setToolTip(tr("Keep the record but mark it Cancelled"));
    m_removeBtn = new QPushButton(tr("Remove"), card);
    m_removeBtn->setObjectName("Danger");
    m_removeBtn->setToolTip(tr("Delete a pending appointment entirely (entered by mistake)"));
    auto* actions = new QHBoxLayout();
    actions->addWidget(m_cancelBtn);
    actions->addWidget(m_removeBtn);
    cardLay->addWidget(m_completeBtn);
    cardLay->addLayout(actions);
    cardLay->addStretch(1);
    root->addWidget(card, 0);

    connect(m_scheduleBtn, &QPushButton::clicked, this, &AppointmentPage::onScheduleClicked);
    connect(m_manageBtn, &QPushButton::clicked, this, &AppointmentPage::onManageServicesClicked);
    connect(m_completeBtn, &QPushButton::clicked, this, &AppointmentPage::onCompleteClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &AppointmentPage::onCancelClicked);
    connect(m_removeBtn, &QPushButton::clicked, this, &AppointmentPage::onRemoveClicked);

    // default booking time: next full hour
    QDateTime next = QDateTime::currentDateTime().addSecs(3600);
    next.setTime(QTime(next.time().hour(), 0));
    m_when->setDateTime(next);
}

// ---------------------------------------------------------------- refresh
void AppointmentPage::refresh() {
    fillPatientCombo();
    fillServiceCombo();
    fillTable();
    checkReminders();
}

void AppointmentPage::fillPatientCombo() {
    const QVariant keep = m_patient->currentData();
    m_patient->clear();
    for (const auto& p : m_patients.patients()) {  // base-class pointers: category name is a virtual call
        m_patient->addItem(QString("%1 (%2)").arg(q(p->name()), q(p->id())), q(p->id()));
        m_patient->setItemData(m_patient->count() - 1, q(p->categoryName()) + tr(" patient"), Qt::ToolTipRole);
    }
    const int idx = m_patient->findData(keep);
    if (idx >= 0) m_patient->setCurrentIndex(idx);
}

void AppointmentPage::fillServiceCombo() {
    const QVariant keep = m_service->currentData();
    m_service->clear();
    for (const auto& s : m_appointments.activeServices()) m_service->addItem(q(s.name), q(s.id));
    const int idx = m_service->findData(keep);
    if (idx >= 0) m_service->setCurrentIndex(idx);

    // empty states: tell the user what to do next
    const bool noPatients = m_patient->count() == 0;
    const bool noServices = m_service->count() == 0;
    if (noPatients)
        m_hint->setText(tr("No patients yet. Register a patient on the Patients page first."));
    else if (noServices)
        m_hint->setText(tr("No services yet. Click \"Manage services...\" to add one."));
    m_hint->setVisible(noPatients || noServices);
    m_scheduleBtn->setEnabled(!noPatients && !noServices);
}

void AppointmentPage::showPatient(const QString& patientId) {
    m_patientFilter = patientId;
    m_filter->setCurrentIndex(0);  // all statuses
    const int idx = m_patient->findData(patientId);
    if (idx >= 0) m_patient->setCurrentIndex(idx);
    m_selectedId.clear();
    fillTable();
}

void AppointmentPage::updateFilterBanner() {
    if (m_patientFilter.isEmpty()) {
        m_filterBanner->hide();
        return;
    }
    try {
        const auto& p = m_patients.getPatient(m_patientFilter.toStdString());
        m_filterLabel->setText(tr("Showing the appointments of <b>%1</b> (%2). Use the form on the right to schedule another one.")
                                   .arg(q(p.name()).toHtmlEscaped(), q(p.id())));
        m_filterBanner->show();
    } catch (const ClinicException&) {  // the patient was deleted
        m_patientFilter.clear();
        m_filterBanner->hide();
    }
}

void AppointmentPage::fillTable() {
    updateFilterBanner();
    const std::string nowStr = now();
    const auto rows = m_appointments.allAppointments(nowStr);
    const QString filter = m_filter->currentIndex() == 0 ? QString() : m_filter->currentText();

    m_table->blockSignals(true);
    m_table->clearContents();
    m_table->setRowCount(0);
    int shown = 0, reselect = -1;
    for (const AppointmentRow& row : rows) {
        const QString status = q(row.displayStatus);
        if (!filter.isEmpty() && status != filter) continue;
        const auto& a = row.appointment;
        if (!m_patientFilter.isEmpty() && q(a.patientId()) != m_patientFilter) continue;
        const StatusLook look = lookFor(status);

        const int r = shown++;
        m_table->setRowCount(shown);
        auto put = [&](int col, const QString& text, bool badge = false) {
            auto* item = new QTableWidgetItem(text);
            if (badge) {
                item->setBackground(QBrush(look.bg));
                item->setForeground(QBrush(look.fg));
                QFont f = item->font();
                f.setBold(true);
                f.setStrikeOut(look.strike);
                item->setFont(f);
                item->setTextAlignment(Qt::AlignCenter);
            } else if (status == "Cancelled") {
                item->setForeground(QBrush(QColor("#8A97A0")));
            }
            m_table->setItem(r, col, item);
        };
        put(ColId, q(a.id()));
        m_table->item(r, ColId)->setData(Qt::UserRole, q(a.id()));
        put(ColPatient, q(a.patientNameSnapshot()));
        put(ColService, q(a.serviceNameSnapshot()));
        put(ColWhen, q(a.dateTime()));
        put(ColStatus, status, true);
        put(ColReason, q(a.reason()));
        if (q(a.id()) == m_selectedId) reselect = r;
    }
    if (reselect >= 0) m_table->selectRow(reselect);
    m_table->blockSignals(false);

    if (reselect < 0) { m_selectedId.clear(); m_selectedStatus.clear(); }
    else m_selectedStatus = m_table->item(reselect, ColStatus)->text();
    updateActionButtons();
}

void AppointmentPage::onClockTick() {
    fillTable();
    checkReminders();
}

// Timer event: tell staff about pending appointments that start within the next few minutes.
// Each appointment is announced once.
void AppointmentPage::checkReminders() {
    const std::string nowStr = now();
    QStringList lines;
    for (const AppointmentRow& row : m_appointments.allAppointments(nowStr)) {
        const auto& a = row.appointment;
        if (a.status() != model::AppointmentStatus::Pending || row.overdue) continue;
        const long minutes = utils::validation::minutesBetween(nowStr, a.dateTime());
        if (minutes < 0 || minutes > kReminderMinutes) continue;
        const QString key = q(a.id()) + "|" + q(a.dateTime());
        if (m_reminded.contains(key)) continue;
        m_reminded.insert(key);
        lines << tr("%1 - %2 at %3 (in %4 min)").arg(q(a.patientNameSnapshot()), q(a.serviceNameSnapshot()),
                                                       q(a.dateTime()).mid(11)).arg(minutes);
    }
    if (!lines.isEmpty()) emit statusMessage(tr("Reminder: ") + lines.join("; "));
}

// ---------------------------------------------------------------- selection
void AppointmentPage::onSelectionChanged() {
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        m_selectedId.clear();
        m_selectedStatus.clear();
    } else {
        const int r = rows.first().row();
        m_selectedId = m_table->item(r, ColId)->data(Qt::UserRole).toString();
        m_selectedStatus = m_table->item(r, ColStatus)->text();
    }
    updateActionButtons();
}

void AppointmentPage::updateActionButtons() {
    const bool hasSel = !m_selectedId.isEmpty();
    const bool pending = hasSel && (m_selectedStatus == "Pending" || m_selectedStatus == "Overdue");
    m_completeBtn->setEnabled(pending);
    m_cancelBtn->setEnabled(pending);
    m_removeBtn->setEnabled(pending);

    if (!hasSel) {
        m_selectedInfo->setText(tr("Select an appointment in the table to complete, cancel or remove it."));
        return;
    }
    const int r = m_table->selectionModel()->selectedRows().first().row();
    QString info = tr("%1: %2, %3 at %4 (%5)")
                       .arg(m_selectedId, m_table->item(r, ColPatient)->text(), m_table->item(r, ColService)->text(),
                            m_table->item(r, ColWhen)->text(), m_selectedStatus);
    if (!pending) info += tr("\nCompleted and cancelled appointments are kept as history.");
    m_selectedInfo->setText(info);
}

// ---------------------------------------------------------------- actions
void AppointmentPage::onScheduleClicked() {
    const std::string patientId = m_patient->currentData().toString().toStdString();
    const std::string serviceId = m_service->currentData().toString().toStdString();
    const std::string when = m_when->dateTime().toString("yyyy-MM-dd HH:mm").toStdString();
    const std::string reason = m_reason->text().toStdString();

    // Same time, different service: not an error, so ask the user.
    if (m_appointments.hasSameTimeConflict(patientId, serviceId, when)) {
        const auto answer = QMessageBox::question(
            this, tr("Same time already booked"),
            tr("This patient already has another appointment at %1.\n\nSchedule this one anyway?")
                .arg(q(when)),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) return;
    }

    std::string newId;
    if (!runGuarded([&] { newId = m_appointments.schedule(patientId, serviceId, when, reason, now()); })) return;
    m_reason->clear();
    m_selectedId = q(newId);
    emit appointmentsChanged();
    emit statusMessage(tr("Scheduled appointment %1").arg(q(newId)));
    refresh();
}

void AppointmentPage::onCancelClicked() {
    if (m_selectedId.isEmpty()) return;
    const auto answer = QMessageBox::question(
        this, tr("Cancel appointment"),
        tr("Cancel appointment %1?\n\nIt stays in the list marked as Cancelled.").arg(m_selectedId),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;
    if (!runGuarded([&] { m_appointments.cancel(m_selectedId.toStdString()); })) return;
    emit appointmentsChanged();
    emit statusMessage(tr("Cancelled appointment %1").arg(m_selectedId));
    refresh();
}

void AppointmentPage::onCompleteClicked() {
    if (m_selectedId.isEmpty()) return;
    bool ok = false;
    const QString note = QInputDialog::getText(this, tr("Mark completed"),
                                               tr("Outcome note for %1 (optional):").arg(m_selectedId),
                                               QLineEdit::Normal, QString(), &ok);
    if (!ok) return;
    if (!runGuarded([&] { m_appointments.complete(m_selectedId.toStdString(), note.toStdString()); })) return;
    emit appointmentsChanged();
    emit statusMessage(tr("Completed appointment %1").arg(m_selectedId));
    refresh();
}

void AppointmentPage::onRemoveClicked() {
    if (m_selectedId.isEmpty()) return;
    const auto answer = QMessageBox::question(
        this, tr("Remove appointment"),
        tr("Remove appointment %1 completely?\n\nUse this for bookings entered by mistake. "
           "To keep a record, cancel it instead.").arg(m_selectedId),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;
    if (!runGuarded([&] { m_appointments.remove(m_selectedId.toStdString()); })) return;
    emit appointmentsChanged();
    emit statusMessage(tr("Removed appointment %1").arg(m_selectedId));
    m_selectedId.clear();
    refresh();
}

void AppointmentPage::onManageServicesClicked() {
    ServiceDialog dialog(m_appointments, this);
    dialog.exec();
    if (dialog.changed()) {
        emit appointmentsChanged();  // services are saved data too
        refresh();
    } else {
        fillServiceCombo();
    }
}

bool AppointmentPage::runGuarded(const std::function<void()>& action) {
    QString message;
    try {
        action();
        m_errorLabel->hide();
        return true;
    } catch (const ClinicException& e) {
        message = q(e.what());
    } catch (const std::exception& e) {
        message = tr("Unexpected error: %1. No changes were made.").arg(q(e.what()));
    } catch (...) {
        message = tr("An unknown error occurred. No changes were made.");
    }
    m_errorLabel->setText(message);
    m_errorLabel->show();
    QMessageBox::warning(this, tr("Cannot complete this action"), message);
    return false;
}

} // namespace scapms::gui
