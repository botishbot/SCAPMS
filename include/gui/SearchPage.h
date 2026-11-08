#pragma once
#include "controller/ClinicManager.h"
#include "controller/SearchEngine.h"
#include "controller/SortEngine.h"

#include <QString>
#include <QWidget>
#include <functional>
#include <optional>

class QTableWidget;
class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;
class QFormLayout;

namespace scapms::gui {

// Search & Sort page. Works on a private list of patients (in registration order until the user sorts).
// Sorting and searching are done by SortEngine / SearchEngine; this class only collects input and shows
// results. The list is rebuilt whenever patients are added, deleted or replaced.
class SearchPage : public QWidget {
    Q_OBJECT
public:
    explicit SearchPage(controller::ClinicManager& patients, QWidget* parent = nullptr);

    void refresh();
    void quickSearch(const QString& text);  // used by the search box in the top bar

signals:
    void patientViewed(const QString& patientId);  // a result row was selected (feeds the cache)
    void statusMessage(const QString&);
    void openPatientRequested(const QString& patientId);        // show the patient's record (edit / delete)
    void openAppointmentsRequested(const QString& patientId);   // show the patient's appointments (schedule / cancel ...)
    void addToQueueRequested(const QString& patientId);         // put the patient in the waiting queue

private slots:
    void onFieldChanged();
    void onKeywordChanged(const QString& text);
    void onCategoryChanged();
    void onSearchClicked();
    void onClearClicked();
    void onSortClicked();
    void onResetOrderClicked();
    void onSortChoiceChanged();
    void onSelectionChanged();

private:
    void buildUi();
    void rebuildView();
    void fillTable();
    void updateAlgorithmChoices();
    void updateSortExplanation();
    void updateStatusLine();
    void showAll();
    void updateSelectedPatient();
    void performSearch(bool interactive);
    controller::SearchField currentField() const;
    std::string currentKeyword() const;
    bool runGuarded(bool interactive, const std::function<void()>& action);

    controller::ClinicManager& m_patients;
    controller::PatientList m_view;                       // all patients, in current order
    controller::PatientList m_shown;                      // what the table shows (all, or search results)
    std::optional<controller::SortField> m_sortedBy;      // empty = registration order
    unsigned long m_version = static_cast<unsigned long>(-1);
    bool m_filtered = false;
    QString m_sortSummary;                                // e.g. "Sorted by Patient name (Insertion Sort)"

    QTableWidget* m_table = nullptr;
    QLabel* m_statusLine = nullptr;
    QFormLayout* m_searchForm = nullptr;
    QComboBox* m_searchField = nullptr;
    QLineEdit* m_keyword = nullptr;
    QComboBox* m_category = nullptr;
    QComboBox* m_searchAlgorithm = nullptr;
    QLabel* m_algorithmHint = nullptr;
    QLabel* m_searchInfo = nullptr;
    QPushButton* m_searchBtn = nullptr;
    QPushButton* m_clearBtn = nullptr;
    QComboBox* m_sortAlgorithm = nullptr;
    QComboBox* m_sortField = nullptr;
    QLabel* m_sortWhy = nullptr;
    QLabel* m_sortInfo = nullptr;
    QPushButton* m_sortBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QLabel* m_selectedInfo = nullptr;       // details of the selected result
    QPushButton* m_openBtn = nullptr;
    QPushButton* m_appointmentsBtn = nullptr;
    QPushButton* m_queueBtn = nullptr;
    QString m_selectedId;
};

} // namespace scapms::gui
