#pragma once
#include "controller/ClinicManager.h"

#include <QString>
#include <QWidget>
#include <functional>

class QTableWidget;
class QLineEdit;
class QComboBox;
class QDateEdit;
class QFormLayout;
class QLabel;
class QPushButton;

namespace scapms::gui {

// Patients page: master-detail layout. Table on the left (QVBoxLayout),
// details form on the right (QFormLayout). All rules live in ClinicManager;
// this class only collects input, shows results and reports errors.
class PatientPage : public QWidget {
    Q_OBJECT
public:
    explicit PatientPage(controller::ClinicManager& manager, QWidget* parent = nullptr);

    void refresh();

signals:
    void patientsChanged();            // data was modified (registered / edited / deleted)
    void patientViewed(const QString& patientId);  // a patient's record was opened (feeds the cache)
    void statusMessage(const QString&);

private slots:
    void onSelectionChanged();
    void onCategoryChanged();
    void onRegisterClicked();
    void onSaveChangesClicked();
    void onDeleteClicked();
    void onClearClicked();

private:
    void buildUi();
    void fillTable();
    void loadIntoForm(const model::PatientRecord& patient);
    void clearForm();
    void updateDynamicFields();
    void updateButtons();
    controller::PatientInput readForm() const;  // throws ClinicException on bad input
    bool runGuarded(const std::function<void()>& action);
    void showError(const QString& message);
    void clearError();

    controller::ClinicManager& m_manager;
    QString m_editingId;  // empty = registering a new patient

    QTableWidget* m_table = nullptr;
    QLabel* m_formTitle = nullptr;
    QLabel* m_errorLabel = nullptr;
    QFormLayout* m_form = nullptr;
    QComboBox* m_category = nullptr;
    QLineEdit* m_name = nullptr;
    QLineEdit* m_age = nullptr;
    QLineEdit* m_phone = nullptr;
    QLineEdit* m_email = nullptr;
    QComboBox* m_triage = nullptr;      // Emergency only
    QDateEdit* m_prevVisit = nullptr;   // Follow-Up only
    QLineEdit* m_reason = nullptr;      // Follow-Up only
    QPushButton* m_registerBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;
    QPushButton* m_clearBtn = nullptr;
};

} // namespace scapms::gui
