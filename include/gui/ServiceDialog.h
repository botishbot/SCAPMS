#pragma once
#include "controller/AppointmentController.h"

#include <QDialog>

class QTableWidget;
class QLineEdit;
class QPushButton;
class QLabel;

namespace scapms::gui {

// Small dialog for the clinic service catalogue: add a service, activate/deactivate one.
class ServiceDialog : public QDialog {
    Q_OBJECT
public:
    explicit ServiceDialog(controller::AppointmentController& controller, QWidget* parent = nullptr);

    bool changed() const { return m_changed; }  // true if anything was added or toggled

private slots:
    void onAddClicked();
    void onToggleClicked();
    void onSelectionChanged();

private:
    void fillTable();
    QString selectedServiceId() const;
    bool runGuarded(const std::function<void()>& action);

    controller::AppointmentController& m_controller;
    QTableWidget* m_table = nullptr;
    QLineEdit* m_name = nullptr;
    QLineEdit* m_description = nullptr;
    QPushButton* m_addBtn = nullptr;
    QPushButton* m_toggleBtn = nullptr;
    QLabel* m_errorLabel = nullptr;
    bool m_changed = false;
};

} // namespace scapms::gui
