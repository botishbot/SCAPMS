#include "gui/PatientPage.h"

#include "gui/TableSupport.h"
#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "utils/ClinicException.h"
#include "utils/Validation.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

using controller::PatientInput;
using model::PatientCategory;
using model::PatientRecord;

namespace {

enum Column { ColId, ColName, ColAge, ColCategory, ColPriority, ColPhone, ColEmail, ColAppointments, ColCount };

struct Look { QColor rowBg; QColor badgeBg; QColor badgeFg; };

// Visual distinction by patient category (custom table presentation).
Look lookFor(PatientCategory c) {
    switch (c) {
        case PatientCategory::Emergency: return {QColor("#FEF3F2"), QColor("#FDE0DC"), QColor("#B42318")};
        case PatientCategory::FollowUp:  return {QColor("#FFFFFF"), QColor("#DBEAFE"), QColor("#1D4ED8")};
        case PatientCategory::General:   return {QColor("#FFFFFF"), QColor("#E8EEF1"), QColor("#3C4A53")};
    }
    return {QColor("#FFFFFF"), QColor("#E8EEF1"), QColor("#3C4A53")};
}

QString q(const std::string& s) { return QString::fromUtf8(s.c_str()); }

} // namespace

PatientPage::PatientPage(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), m_manager(manager) {
    buildUi();
    refresh();
}

void PatientPage::buildUi() {
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // ---- left: title + table ----
    auto* left = new QVBoxLayout();
    left->setSpacing(12);
    auto* title = new QLabel(tr("Patients"), this);
    title->setObjectName("PageTitle");
    left->addWidget(title);

    m_table = new QTableWidget(0, ColCount, this);
    m_table->setObjectName("PatientTable");
    m_table->setHorizontalHeaderLabels({tr("ID"), tr("Name"), tr("Age"), tr("Category"), tr("Priority"),
                                        tr("Phone"), tr("Email"), tr("Appts")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(false);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(40);
    m_table->setSortingEnabled(false);  // sorting is done by SortEngine, not by Qt's header sort
    // All columns remain user-resizable; the shared helper remembers each width.
    configureResizableTable(m_table, {104, 190, 66, 112, 100, 145, 190, 74},
                            {88, 140, 58, 96, 90, 115, 130, 64});
    m_table->setToolTip(tr("Select a patient to view or edit their details."));
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &PatientPage::onSelectionChanged);
    left->addWidget(m_table, 1);
    root->addLayout(left, 1);

    // ---- right: details card ----
    auto* card = new QFrame(this);
    card->setObjectName("Card");
    card->setMinimumWidth(330);
    card->setMaximumWidth(340);
    auto* cardLay = new QVBoxLayout(card);
    cardLay->setContentsMargins(20, 18, 20, 18);
    cardLay->setSpacing(12);

    m_formTitle = new QLabel(card);
    m_formTitle->setObjectName("SectionTitle");
    cardLay->addWidget(m_formTitle);

    m_errorLabel = new QLabel(card);
    m_errorLabel->setObjectName("ErrorLabel");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    cardLay->addWidget(m_errorLabel);

    m_form = new QFormLayout();
    m_form->setLabelAlignment(Qt::AlignLeft);
    m_form->setVerticalSpacing(10);

    m_category = new QComboBox(card);
    m_category->setObjectName("CategoryCombo");
    m_category->addItem(tr("General"), static_cast<int>(PatientCategory::General));
    m_category->addItem(tr("Emergency"), static_cast<int>(PatientCategory::Emergency));
    m_category->addItem(tr("Follow-Up"), static_cast<int>(PatientCategory::FollowUp));
    m_category->setToolTip(tr("Patient type. Fixed after registration; delete and re-register to change it."));
    connect(m_category, &QComboBox::currentIndexChanged, this, &PatientPage::onCategoryChanged);

    m_name = new QLineEdit(card);
    m_name->setObjectName("NameEdit");
    m_name->setPlaceholderText(tr("Full name"));
    m_age = new QLineEdit(card);
    m_age->setObjectName("AgeEdit");
    m_age->setPlaceholderText(tr("0 - 120"));
    m_age->setToolTip(tr("Whole number from 0 to 120."));
    m_phone = new QLineEdit(card);
    m_phone->setObjectName("PhoneEdit");
    m_phone->setPlaceholderText(tr("e.g. 0803 555 0101"));
    m_email = new QLineEdit(card);
    m_email->setObjectName("EmailEdit");
    m_email->setPlaceholderText(tr("name@example.com"));
    m_email->setToolTip(tr("Enter a phone number, an email address, or both."));

    m_triage = new QComboBox(card);
    m_triage->setObjectName("TriageCombo");
    for (int level = 1; level <= 3; ++level)
        m_triage->addItem(QString("%1 - %2").arg(level).arg(model::EmergencyPatient::triageName(level)), level);
    m_triage->setCurrentIndex(1);
    m_prevVisit = new QDateEdit(card);
    m_prevVisit->setObjectName("PrevVisitEdit");
    m_prevVisit->setCalendarPopup(true);
    m_prevVisit->setDisplayFormat("yyyy-MM-dd");
    m_prevVisit->setMaximumDate(QDate::currentDate());
    m_prevVisit->setDate(QDate::currentDate().addMonths(-1));
    m_reason = new QLineEdit(card);
    m_reason->setObjectName("ReasonEdit");
    m_reason->setPlaceholderText(tr("Reason for follow-up (optional)"));

    m_form->addRow(tr("Category"), m_category);
    m_form->addRow(tr("Name"), m_name);
    m_form->addRow(tr("Age"), m_age);
    m_form->addRow(tr("Phone"), m_phone);
    m_form->addRow(tr("Email"), m_email);
    m_form->addRow(tr("Triage level"), m_triage);          // shown for Emergency only
    m_form->addRow(tr("Previous visit"), m_prevVisit);     // shown for Follow-Up only
    m_form->addRow(tr("Follow-up reason"), m_reason);      // shown for Follow-Up only
    cardLay->addLayout(m_form);

    m_registerBtn = new QPushButton(tr("Register patient"), card);
    m_registerBtn->setObjectName("Primary");
    m_registerBtn->setToolTip(tr("Add this patient to the clinic records"));
    m_saveBtn = new QPushButton(tr("Save changes"), card);
    m_saveBtn->setObjectName("Primary");
    m_saveBtn->setToolTip(tr("Update the selected patient"));
    m_deleteBtn = new QPushButton(tr("Delete"), card);
    m_deleteBtn->setObjectName("Danger");
    m_deleteBtn->setToolTip(tr("Delete the selected patient (blocked while appointments are pending)"));
    m_clearBtn = new QPushButton(tr("New patient"), card);
    m_clearBtn->setToolTip(tr("Clear the form to register another patient"));

    auto* row1 = new QHBoxLayout();
    row1->addWidget(m_registerBtn);
    row1->addWidget(m_saveBtn);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_clearBtn);
    row2->addWidget(m_deleteBtn);
    cardLay->addLayout(row1);
    cardLay->addLayout(row2);
    cardLay->addStretch(1);
    root->addWidget(card, 0);

    connect(m_registerBtn, &QPushButton::clicked, this, &PatientPage::onRegisterClicked);
    connect(m_saveBtn, &QPushButton::clicked, this, &PatientPage::onSaveChangesClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &PatientPage::onDeleteClicked);
    connect(m_clearBtn, &QPushButton::clicked, this, &PatientPage::onClearClicked);

    clearForm();
}

// ---------------------------------------------------------------- table
void PatientPage::refresh() {
    fillTable();
    // keep the selection if the patient still exists, otherwise go back to "register" mode
    if (!m_editingId.isEmpty()) {
        bool found = false;
        for (int r = 0; r < m_table->rowCount(); ++r) {
            if (m_table->item(r, ColId)->data(Qt::UserRole).toString() == m_editingId) {
                m_table->blockSignals(true);
                m_table->selectRow(r);
                m_table->blockSignals(false);
                found = true;
                break;
            }
        }
        if (!found) clearForm();
    }
    updateButtons();
}

void PatientPage::selectPatient(const QString& patientId) {
    for (int r = 0; r < m_table->rowCount(); ++r) {
        if (m_table->item(r, ColId)->data(Qt::UserRole).toString() == patientId) {
            m_table->scrollToItem(m_table->item(r, ColId));
            if (m_editingId != patientId) m_table->selectRow(r);  // loads the form through onSelectionChanged
            return;
        }
    }
}

void PatientPage::fillTable() {
    const auto& list = m_manager.patients();
    m_table->blockSignals(true);
    m_table->clearContents();
    m_table->setRowCount(static_cast<int>(list.size()));
    for (int r = 0; r < static_cast<int>(list.size()); ++r) {
        const PatientRecord& p = *list[static_cast<std::size_t>(r)];  // base-class reference: polymorphic calls below
        const Look look = lookFor(p.category());

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
        put(ColId, q(p.id()));
        m_table->item(r, ColId)->setData(Qt::UserRole, q(p.id()));
        put(ColName, q(p.name()), false, true);
        put(ColAge, QString::number(p.age()), false, true);
        put(ColCategory, q(p.categoryName()), true, true);
        put(ColPriority, q(p.priorityLabel()), true, true);
        put(ColPhone, q(p.phone()), false, true);
        put(ColEmail, q(p.email()), false, true);
        m_table->item(r, ColPhone)->setToolTip(q(p.phone()));
        m_table->item(r, ColEmail)->setToolTip(q(p.email()));
        put(ColAppointments, QString::number(p.appointments().size()), false, true);
    }
    m_table->blockSignals(false);
}

void PatientPage::onSelectionChanged() {
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    const QString id = m_table->item(rows.first().row(), ColId)->data(Qt::UserRole).toString();
    if (runGuarded([&] { loadIntoForm(m_manager.getPatient(id.toStdString())); })) emit patientViewed(id);
}

// ---------------------------------------------------------------- form
void PatientPage::loadIntoForm(const PatientRecord& p) {
    clearError();
    m_editingId = q(p.id());
    m_category->blockSignals(true);
    m_category->setCurrentIndex(m_category->findData(static_cast<int>(p.category())));
    m_category->blockSignals(false);
    m_name->setText(q(p.name()));
    m_age->setText(QString::number(p.age()));
    m_phone->setText(q(p.phone()));
    m_email->setText(q(p.email()));

    if (const auto* e = dynamic_cast<const model::EmergencyPatient*>(&p))
        m_triage->setCurrentIndex(m_triage->findData(e->triageLevel()));
    if (const auto* f = dynamic_cast<const model::FollowUpPatient*>(&p)) {
        m_prevVisit->setDate(QDate::fromString(q(f->previousVisitDate()), "yyyy-MM-dd"));
        m_reason->setText(q(f->followUpReason()));
    }
    updateDynamicFields();
    updateButtons();
}

void PatientPage::clearForm() {
    m_editingId.clear();
    m_table->blockSignals(true);
    m_table->clearSelection();
    m_table->blockSignals(false);
    m_category->blockSignals(true);
    m_category->setCurrentIndex(0);
    m_category->blockSignals(false);
    m_name->clear();
    m_age->clear();
    m_phone->clear();
    m_email->clear();
    m_triage->setCurrentIndex(1);
    m_prevVisit->setDate(QDate::currentDate().addMonths(-1));
    m_reason->clear();
    clearError();
    updateDynamicFields();
    updateButtons();
}

void PatientPage::onCategoryChanged() { updateDynamicFields(); }

// Dynamic components: show only the fields that belong to the selected patient type.
void PatientPage::updateDynamicFields() {
    const auto cat = static_cast<PatientCategory>(m_category->currentData().toInt());
    m_form->setRowVisible(m_triage, cat == PatientCategory::Emergency);
    m_form->setRowVisible(m_prevVisit, cat == PatientCategory::FollowUp);
    m_form->setRowVisible(m_reason, cat == PatientCategory::FollowUp);
}

void PatientPage::updateButtons() {
    const bool editing = !m_editingId.isEmpty();
    m_formTitle->setText(editing ? tr("Edit patient %1").arg(m_editingId) : tr("Register new patient"));
    m_registerBtn->setVisible(!editing);
    m_saveBtn->setVisible(editing);
    m_deleteBtn->setEnabled(editing);
    m_category->setEnabled(!editing);  // category is locked after registration
}

PatientInput PatientPage::readForm() const {
    PatientInput in;
    in.category = static_cast<PatientCategory>(m_category->currentData().toInt());
    in.name = m_name->text().toStdString();
    in.age = utils::validation::parseAge(m_age->text().toStdString());  // throws for "abc", 150, ...
    in.phone = m_phone->text().toStdString();
    in.email = m_email->text().toStdString();
    in.triageLevel = m_triage->currentData().toInt();
    in.previousVisitDate = m_prevVisit->date().toString("yyyy-MM-dd").toStdString();
    in.followUpReason = m_reason->text().toStdString();
    return in;
}

// ---------------------------------------------------------------- actions
void PatientPage::onRegisterClicked() {
    std::string newId;
    if (!runGuarded([&] { newId = m_manager.registerPatient(readForm()); })) return;
    emit patientsChanged();
    emit statusMessage(tr("Registered patient %1").arg(q(newId)));
    clearForm();
    fillTable();
}

void PatientPage::onSaveChangesClicked() {
    if (m_editingId.isEmpty()) return;
    if (!runGuarded([&] { m_manager.editPatient(m_editingId.toStdString(), readForm()); })) return;
    emit patientsChanged();
    emit statusMessage(tr("Saved changes to %1").arg(m_editingId));
    fillTable();
    refresh();
}

void PatientPage::onDeleteClicked() {
    if (m_editingId.isEmpty()) return;
    const QString id = m_editingId;
    QString name;
    if (!runGuarded([&] { name = q(m_manager.getPatient(id.toStdString()).name()); })) return;

    const auto answer = QMessageBox::question(
        this, tr("Delete patient"),
        tr("Delete %1 (%2)?\n\nTheir completed and cancelled appointments stay in the clinic history.")
            .arg(name, id),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;

    if (!runGuarded([&] { m_manager.deletePatient(id.toStdString()); })) return;
    emit patientsChanged();
    emit statusMessage(tr("Deleted patient %1").arg(id));
    clearForm();
    fillTable();
}

void PatientPage::onClearClicked() { clearForm(); }

// ---------------------------------------------------------------- errors
// Exception handling: expected domain errors first, then anything else as a safe fallback.
bool PatientPage::runGuarded(const std::function<void()>& action) {
    try {
        action();
        clearError();
        return true;
    } catch (const utils::ClinicException& e) {
        showError(q(e.what()));
    } catch (const std::exception& e) {
        showError(tr("Unexpected error: %1. No changes were made.").arg(q(e.what())));
    } catch (...) {
        showError(tr("An unknown error occurred. No changes were made."));
    }
    return false;
}

void PatientPage::showError(const QString& message) {
    m_errorLabel->setText(message);
    m_errorLabel->show();
    QMessageBox::warning(this, tr("Cannot complete this action"), message);
}

void PatientPage::clearError() {
    m_errorLabel->clear();
    m_errorLabel->hide();
}

} // namespace scapms::gui
