#include "gui/SearchPage.h"

#include "gui/TableSupport.h"
#include "utils/ClinicException.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

using controller::PatientList;
using controller::SearchAlgorithm;
using controller::SearchEngine;
using controller::SearchField;
using controller::SortAlgorithm;
using controller::SortEngine;
using controller::SortField;
using model::PatientCategory;
using model::PatientRecord;
using utils::ClinicException;

namespace {

enum Column { ColId, ColName, ColAge, ColCategory, ColPriority, ColPhone, ColEmail, ColAppointments, ColCount };

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

SearchPage::SearchPage(controller::ClinicManager& patients, QWidget* parent) : QWidget(parent), m_patients(patients) {
    buildUi();
    refresh();
}

void SearchPage::buildUi() {
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // ---------------- left: table ----------------
    auto* left = new QVBoxLayout();
    left->setSpacing(12);
    auto* title = new QLabel(tr("Search & Sort"), this);
    title->setObjectName("PageTitle");
    left->addWidget(title);

    m_table = new QTableWidget(0, ColCount, this);
    m_table->setObjectName("SearchResultsTable");
    m_table->setHorizontalHeaderLabels({tr("ID"), tr("Name"), tr("Age"), tr("Category"), tr("Priority"),
                                        tr("Phone"), tr("Email"), tr("Appts")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setWordWrap(false);
    m_table->setSortingEnabled(false);  // ordering comes only from our own Selection / Insertion Sort
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(40);
    // Match the Patients table so search results remain familiar and equally readable.
    configureResizableTable(m_table, {104, 190, 66, 112, 100, 145, 190, 74},
                            {88, 140, 58, 96, 90, 115, 130, 64});
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &SearchPage::onSelectionChanged);
    left->addWidget(m_table, 1);

    // details of the selected patient with the actions that can be done on them
    auto* detail = new QFrame(this);
    detail->setObjectName("Card");
    auto* detailLay = new QVBoxLayout(detail);
    detailLay->setContentsMargins(16, 12, 16, 12);
    detailLay->setSpacing(8);
    m_selectedInfo = new QLabel(detail);
    m_selectedInfo->setObjectName("SelectedPatientInfo");
    m_selectedInfo->setTextFormat(Qt::RichText);
    m_selectedInfo->setWordWrap(true);
    auto* actions = new QHBoxLayout();
    m_openBtn = new QPushButton(tr("Open patient record"), detail);
    m_openBtn->setToolTip(tr("Show this patient on the Patients page to edit or delete them (also: double-click a row)"));
    m_appointmentsBtn = new QPushButton(tr("Appointments..."), detail);
    m_appointmentsBtn->setToolTip(tr("Show this patient's appointments to schedule, cancel, complete or remove them"));
    m_queueBtn = new QPushButton(tr("Add to queue"), detail);
    m_queueBtn->setToolTip(tr("Put this patient in the waiting queue for their next pending appointment"));
    actions->addWidget(m_openBtn);
    actions->addWidget(m_appointmentsBtn);
    actions->addWidget(m_queueBtn);
    actions->addStretch(1);
    detailLay->addWidget(m_selectedInfo);
    detailLay->addLayout(actions);
    left->addWidget(detail);
    connect(m_openBtn, &QPushButton::clicked, this, [this] { if (!m_selectedId.isEmpty()) emit openPatientRequested(m_selectedId); });
    connect(m_appointmentsBtn, &QPushButton::clicked, this, [this] { if (!m_selectedId.isEmpty()) emit openAppointmentsRequested(m_selectedId); });
    connect(m_queueBtn, &QPushButton::clicked, this, [this] { if (!m_selectedId.isEmpty()) emit addToQueueRequested(m_selectedId); });
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int, int) { if (!m_selectedId.isEmpty()) emit openPatientRequested(m_selectedId); });

    m_statusLine = new QLabel(this);
    m_statusLine->setObjectName("Muted");
    m_statusLine->setWordWrap(true);
    left->addWidget(m_statusLine);
    root->addLayout(left, 1);

    // ---------------- right: search + sort controls (scrollable on small windows) ----------------
    auto* card = new QFrame(this);
    card->setObjectName("Card");
    auto* cardLay = new QVBoxLayout(card);
    cardLay->setContentsMargins(20, 18, 20, 18);
    cardLay->setSpacing(10);

    auto* searchTitle = new QLabel(tr("Search"), card);
    searchTitle->setObjectName("SectionTitle");
    cardLay->addWidget(searchTitle);

    m_searchForm = new QFormLayout();
    m_searchForm->setVerticalSpacing(8);
    m_searchField = new QComboBox(card);
    m_searchField->setObjectName("SearchFieldCombo");
    m_searchField->addItem(tr("Patient ID"), static_cast<int>(SearchField::Id));
    m_searchField->addItem(tr("Patient name"), static_cast<int>(SearchField::Name));
    m_searchField->addItem(tr("Category"), static_cast<int>(SearchField::Category));
    m_searchField->setCurrentIndex(1);
    m_searchField->setToolTip(tr("What to search by"));
    m_keyword = new QLineEdit(card);
    m_keyword->setObjectName("SearchKeywordEdit");
    m_keyword->setPlaceholderText(tr("Type to search..."));
    m_keyword->setClearButtonEnabled(true);
    m_keyword->setToolTip(tr("A name matches anywhere in the name (\"eze\" finds \"Ngozi Eze\"). An ID can be typed as P-0003, p3 or 3. Case does not matter."));
    m_category = new QComboBox(card);
    m_category->setObjectName("SearchCategoryCombo");
    m_category->addItems({tr("General"), tr("Emergency"), tr("Follow-Up")});
    m_searchAlgorithm = new QComboBox(card);
    m_searchAlgorithm->setObjectName("SearchAlgorithmCombo");
    m_searchAlgorithm->setToolTip(tr("Only the searches that work on the current list order are offered"));
    m_searchForm->addRow(tr("Search by"), m_searchField);
    m_searchForm->addRow(tr("Keyword"), m_keyword);
    m_searchForm->addRow(tr("Category"), m_category);
    m_searchForm->addRow(tr("Algorithm"), m_searchAlgorithm);
    cardLay->addLayout(m_searchForm);

    m_algorithmHint = new QLabel(card);
    m_algorithmHint->setObjectName("SmallNote");
    m_algorithmHint->setWordWrap(true);
    cardLay->addWidget(m_algorithmHint);

    auto* searchButtons = new QHBoxLayout();
    m_searchBtn = new QPushButton(tr("Search"), card);
    m_searchBtn->setObjectName("Primary");
    m_searchBtn->setToolTip(tr("Run the search (Enter)"));
    m_clearBtn = new QPushButton(tr("Clear"), card);
    m_clearBtn->setToolTip(tr("Clear the search and show all patients"));
    searchButtons->addWidget(m_searchBtn);
    searchButtons->addWidget(m_clearBtn);
    cardLay->addLayout(searchButtons);

    m_searchInfo = new QLabel(card);
    m_searchInfo->setObjectName("InfoLabel");
    m_searchInfo->setWordWrap(true);
    m_searchInfo->hide();
    cardLay->addWidget(m_searchInfo);

    auto* line = new QFrame(card);
    line->setFrameShape(QFrame::HLine);
    line->setObjectName("Divider");
    cardLay->addWidget(line);

    auto* sortTitle = new QLabel(tr("Sort"), card);
    sortTitle->setObjectName("SectionTitle");
    cardLay->addWidget(sortTitle);

    auto* sortForm = new QFormLayout();
    sortForm->setVerticalSpacing(8);
    m_sortAlgorithm = new QComboBox(card);
    m_sortAlgorithm->setObjectName("SortAlgorithmCombo");
    m_sortAlgorithm->addItem(tr("Selection Sort"), static_cast<int>(SortAlgorithm::Selection));
    m_sortAlgorithm->addItem(tr("Insertion Sort"), static_cast<int>(SortAlgorithm::Insertion));
    m_sortAlgorithm->setCurrentIndex(1);
    m_sortAlgorithm->setToolTip(tr("The sorting algorithm to use"));
    m_sortField = new QComboBox(card);
    m_sortField->setObjectName("SortFieldCombo");
    m_sortField->addItem(tr("Patient name"), static_cast<int>(SortField::Name));
    m_sortField->addItem(tr("Patient ID"), static_cast<int>(SortField::Id));
    m_sortField->addItem(tr("Age"), static_cast<int>(SortField::Age));
    m_sortField->setToolTip(tr("The field to sort by (ascending)"));
    sortForm->addRow(tr("Sort algorithm"), m_sortAlgorithm);
    sortForm->addRow(tr("Sort by"), m_sortField);
    cardLay->addLayout(sortForm);

    m_sortWhy = new QLabel(card);
    m_sortWhy->setObjectName("SmallNote");
    m_sortWhy->setWordWrap(true);
    m_sortWhy->setToolTip(tr("Both algorithms take time proportional to the square of the list size, which is fine "
                             "for a clinic's records (hundreds of patients). A very large database would call for "
                             "Merge Sort or Quick Sort."));
    cardLay->addWidget(m_sortWhy);

    auto* sortButtons = new QHBoxLayout();
    m_sortBtn = new QPushButton(tr("SORT"), card);
    m_sortBtn->setObjectName("Primary");
    m_sortBtn->setToolTip(tr("Sort the patient list with the chosen algorithm"));
    m_resetBtn = new QPushButton(tr("Original order"), card);
    m_resetBtn->setToolTip(tr("Go back to registration order"));
    sortButtons->addWidget(m_sortBtn);
    sortButtons->addWidget(m_resetBtn);
    cardLay->addLayout(sortButtons);

    m_sortInfo = new QLabel(card);
    m_sortInfo->setObjectName("InfoLabel");
    m_sortInfo->setWordWrap(true);
    m_sortInfo->hide();
    cardLay->addWidget(m_sortInfo);
    cardLay->addStretch(1);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(card);
    scroll->setMinimumWidth(350);
    scroll->setMaximumWidth(370);
    root->addWidget(scroll, 0);

    connect(m_searchField, &QComboBox::currentIndexChanged, this, &SearchPage::onFieldChanged);
    connect(m_keyword, &QLineEdit::textChanged, this, &SearchPage::onKeywordChanged);     // live search as you type
    connect(m_keyword, &QLineEdit::returnPressed, this, &SearchPage::onSearchClicked);
    connect(m_category, &QComboBox::currentIndexChanged, this, &SearchPage::onCategoryChanged);
    connect(m_searchAlgorithm, &QComboBox::currentIndexChanged, this, [this] {
        if (m_filtered) performSearch(false);
    });
    connect(m_searchBtn, &QPushButton::clicked, this, &SearchPage::onSearchClicked);
    connect(m_clearBtn, &QPushButton::clicked, this, &SearchPage::onClearClicked);
    connect(m_sortBtn, &QPushButton::clicked, this, &SearchPage::onSortClicked);
    connect(m_resetBtn, &QPushButton::clicked, this, &SearchPage::onResetOrderClicked);
    connect(m_sortAlgorithm, &QComboBox::currentIndexChanged, this, &SearchPage::onSortChoiceChanged);
    connect(m_sortField, &QComboBox::currentIndexChanged, this, &SearchPage::onSortChoiceChanged);

    onFieldChanged();
}

// ---------------------------------------------------------------- list state
void SearchPage::rebuildView() {
    m_view.clear();
    for (const auto& p : m_patients.patients()) m_view.push_back(p.get());
    m_shown = m_view;
    m_sortedBy.reset();
    m_filtered = false;
    m_sortSummary.clear();
    m_version = m_patients.patientsVersion();
}

void SearchPage::refresh() {
    if (m_patients.patientsVersion() != m_version) {
        const bool hadOrder = m_sortedBy.has_value();
        rebuildView();  // patient objects changed: old pointers must not be used any more
        if (hadOrder) {
            m_sortInfo->setText(tr("The patient list changed, so the sort order was reset. Sort again to restore it."));
            m_sortInfo->show();
        }
        updateAlgorithmChoices();
        updateSortExplanation();
        if (!m_keyword->text().trimmed().isEmpty() || m_searchField->currentData().toInt() == static_cast<int>(SearchField::Category))
            if (!m_keyword->text().trimmed().isEmpty()) performSearch(false);
    }
    fillTable();
}

void SearchPage::showAll() {
    m_shown = m_view;
    m_filtered = false;
    m_searchInfo->hide();
    fillTable();
}

SearchField SearchPage::currentField() const { return static_cast<SearchField>(m_searchField->currentData().toInt()); }

std::string SearchPage::currentKeyword() const {
    return currentField() == SearchField::Category ? m_category->currentText().toStdString()
                                                   : m_keyword->text().toStdString();
}

// ---------------------------------------------------------------- table
void SearchPage::fillTable() {
    m_table->blockSignals(true);
    m_table->clearContents();
    m_table->setRowCount(static_cast<int>(m_shown.size()));
    for (int r = 0; r < static_cast<int>(m_shown.size()); ++r) {
        const PatientRecord& p = *m_shown[static_cast<std::size_t>(r)];  // base-class reference
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
    int keep = -1;
    for (int r = 0; r < m_table->rowCount(); ++r)
        if (m_table->item(r, ColId)->data(Qt::UserRole).toString() == m_selectedId) keep = r;
    if (keep >= 0) {
        m_table->blockSignals(true);
        m_table->selectRow(keep);
        m_table->blockSignals(false);
    } else {
        m_selectedId.clear();
    }
    updateSelectedPatient();
    updateStatusLine();
}

void SearchPage::updateStatusLine() {
    QString text;
    if (m_view.empty()) {
        text = tr("No patients yet. Register patients on the Patients page.");
    } else if (m_filtered) {
        text = tr("Showing %1 of %2 patients").arg(m_shown.size()).arg(m_view.size());
        if (m_shown.empty()) text = tr("No patient matches this search. Showing 0 of %1 patients.").arg(m_view.size());
    } else {
        text = tr("Showing all %1 patients").arg(m_view.size());
    }
    text += m_sortedBy ? QString(" | ") + m_sortSummary : tr(" | Order: registration order (not sorted)");
    m_statusLine->setText(text);
}

void SearchPage::onSelectionChanged() {
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        m_selectedId.clear();
    } else {
        m_selectedId = m_table->item(rows.first().row(), ColId)->data(Qt::UserRole).toString();
        emit patientViewed(m_selectedId);
    }
    updateSelectedPatient();
}

void SearchPage::updateSelectedPatient() {
    const bool has = !m_selectedId.isEmpty();
    m_openBtn->setEnabled(has);
    m_appointmentsBtn->setEnabled(has);
    m_queueBtn->setEnabled(has);
    if (!has) {
        m_selectedInfo->setText(tr("Select a patient in the table to open their record, see or cancel their appointments, or add them to the queue."));
        return;
    }
    try {
        const PatientRecord& p = m_patients.getPatient(m_selectedId.toStdString());  // base-class reference
        int pending = 0, completed = 0, cancelled = 0;
        std::string next;
        for (const auto& a : p.appointments()) {
            switch (a.status()) {
                case model::AppointmentStatus::Pending:
                    ++pending;
                    if (next.empty() || a.dateTime() < next) next = a.dateTime();
                    break;
                case model::AppointmentStatus::Completed: ++completed; break;
                case model::AppointmentStatus::Cancelled: ++cancelled; break;
            }
        }
        QString appointments = tr("%1 appointment(s): %2 pending, %3 completed, %4 cancelled")
                                   .arg(p.appointments().size()).arg(pending).arg(completed).arg(cancelled);
        if (pending > 0) appointments += tr(". Next: %1").arg(q(next));
        m_selectedInfo->setText(QString("<b>%1</b> (%2) - %3, %4<br>Age %5 | %6 | %7<br>%8")
                                    .arg(q(p.name()).toHtmlEscaped(), q(p.id()), q(p.categoryName()), q(p.priorityLabel()))
                                    .arg(p.age())
                                    .arg(q(p.contactSummary()).toHtmlEscaped(), q(p.summary()).toHtmlEscaped(), appointments));
    } catch (const ClinicException&) {
        m_selectedId.clear();
        m_selectedInfo->setText(tr("Select a patient in the table."));
        m_openBtn->setEnabled(false);
        m_appointmentsBtn->setEnabled(false);
        m_queueBtn->setEnabled(false);
    }
}

// ---------------------------------------------------------------- search
void SearchPage::onFieldChanged() {
    const bool isCategory = currentField() == SearchField::Category;
    m_searchForm->setRowVisible(m_keyword, !isCategory);   // dynamic components: keyword box or category list
    m_searchForm->setRowVisible(m_category, isCategory);
    updateAlgorithmChoices();
    if (isCategory) performSearch(false);
    else if (!m_keyword->text().trimmed().isEmpty()) performSearch(false);
    else showAll();
}

void SearchPage::updateAlgorithmChoices() {
    const SearchField field = currentField();
    m_searchAlgorithm->blockSignals(true);
    m_searchAlgorithm->clear();
    for (SearchAlgorithm a : {SearchAlgorithm::Linear, SearchAlgorithm::Binary, SearchAlgorithm::Recursive})
        if (SearchEngine::isAlgorithmValid(a, field, m_sortedBy))
            m_searchAlgorithm->addItem(q(SearchEngine::name(a)), static_cast<int>(a));
    // pre-select the technique that fits the current state of the data
    const int best = m_searchAlgorithm->findData(static_cast<int>(SearchEngine::suggest(field, m_sortedBy)));
    m_searchAlgorithm->setCurrentIndex(best >= 0 ? best : 0);
    m_searchAlgorithm->blockSignals(false);
    m_algorithmHint->setText(q(SearchEngine::whySuggested(field, m_sortedBy)));
}

void SearchPage::onKeywordChanged(const QString& text) {
    if (text.trimmed().isEmpty()) {
        if (currentField() != SearchField::Category) showAll();
        return;
    }
    performSearch(false);
}

void SearchPage::onCategoryChanged() {
    if (currentField() == SearchField::Category) performSearch(false);
}

void SearchPage::onSearchClicked() { performSearch(true); }

void SearchPage::onClearClicked() {
    m_keyword->clear();
    showAll();
}

void SearchPage::performSearch(bool interactive) {
    controller::SearchResult result;
    const auto algorithm = static_cast<SearchAlgorithm>(m_searchAlgorithm->currentData().toInt());
    if (!runGuarded(interactive, [&] {
            result = SearchEngine::search(m_view, currentField(), currentKeyword(), algorithm, m_sortedBy);
        })) return;

    m_shown = result.matches;
    m_filtered = true;
    m_searchInfo->setText(tr("%1 | %2 comparison(s) | %3 result(s)")
                              .arg(q(SearchEngine::name(result.algorithm)))
                              .arg(result.comparisons)
                              .arg(result.matches.size()));
    m_searchInfo->show();
    fillTable();
    emit statusMessage(tr("%1 found %2 patient(s)").arg(q(SearchEngine::name(result.algorithm))).arg(result.matches.size()));
}

void SearchPage::quickSearch(const QString& text) {
    const QString t = text.trimmed();
    if (t.isEmpty()) { onClearClicked(); return; }
    // a number such as "3", "p3" or "P-0003" searches by ID, anything else by name
    const bool looksLikeId = QRegularExpression("^([pP]-?)?\\d+$").match(t).hasMatch();
    m_searchField->setCurrentIndex(m_searchField->findData(static_cast<int>(looksLikeId ? SearchField::Id : SearchField::Name)));
    m_keyword->setText(t);
}

// ---------------------------------------------------------------- sort
void SearchPage::onSortChoiceChanged() { updateSortExplanation(); }

void SearchPage::updateSortExplanation() {
    const auto algorithm = static_cast<SortAlgorithm>(m_sortAlgorithm->currentData().toInt());
    const auto field = static_cast<SortField>(m_sortField->currentData().toInt());
    const bool sorted = SortEngine::isSortedBy(m_view, field);
    m_sortWhy->setText(q(SortEngine::explain(algorithm, m_view.size(), sorted && !m_view.empty())));
}

void SearchPage::onSortClicked() {
    const auto algorithm = static_cast<SortAlgorithm>(m_sortAlgorithm->currentData().toInt());
    const auto field = static_cast<SortField>(m_sortField->currentData().toInt());
    controller::SortStats stats;
    if (!runGuarded(true, [&] { stats = SortEngine::sort(m_view, algorithm, field); })) return;

    m_sortedBy = field;
    m_sortSummary = tr("Sorted by %1 (%2)").arg(q(SortEngine::fieldName(field)), q(SortEngine::name(algorithm)));
    m_sortInfo->setText(tr("%1: %2 comparison(s), %3 %4")
                            .arg(q(SortEngine::name(algorithm)))
                            .arg(stats.comparisons)
                            .arg(stats.moves)
                            .arg(algorithm == SortAlgorithm::Selection ? tr("swap(s)") : tr("shift(s)")));
    m_sortInfo->show();
    m_shown = m_view;
    m_filtered = false;
    updateAlgorithmChoices();       // Binary Search is now possible for the sorted field
    updateSortExplanation();
    if (currentField() == SearchField::Category || !m_keyword->text().trimmed().isEmpty()) performSearch(false);
    else fillTable();
    emit statusMessage(m_sortSummary);
}

void SearchPage::onResetOrderClicked() {
    rebuildView();
    m_sortInfo->hide();
    updateAlgorithmChoices();
    updateSortExplanation();
    if (currentField() == SearchField::Category || !m_keyword->text().trimmed().isEmpty()) performSearch(false);
    else fillTable();
}

bool SearchPage::runGuarded(bool interactive, const std::function<void()>& action) {
    QString message;
    try {
        action();
        return true;
    } catch (const ClinicException& e) {
        message = q(e.what());
    } catch (const std::exception& e) {
        message = tr("Unexpected error: %1. Nothing was changed.").arg(q(e.what()));
    } catch (...) {
        message = tr("An unknown error occurred. Nothing was changed.");
    }
    if (interactive) {
        QMessageBox::warning(this, tr("Cannot complete this action"), message);
    } else {
        m_searchInfo->setText(message);  // while typing, show the problem quietly instead of a dialog
        m_searchInfo->show();
    }
    return false;
}

} // namespace scapms::gui
