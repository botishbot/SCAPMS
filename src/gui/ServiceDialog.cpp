#include "gui/ServiceDialog.h"

#include "gui/TableSupport.h"
#include "utils/ClinicException.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

namespace {
enum Column { ColId, ColName, ColDescription, ColStatus, ColCount };
QString q(const std::string& s) { return QString::fromUtf8(s.c_str()); }
} // namespace

ServiceDialog::ServiceDialog(controller::AppointmentController& controller, QWidget* parent)
    : QDialog(parent), m_controller(controller) {
    setWindowTitle(tr("Manage clinic services"));
    resize(640, 460);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 16);
    root->setSpacing(12);

    auto* intro = new QLabel(tr("Services appear in the appointment form. Deactivating a service hides it "
                                "from new bookings; existing appointments are not affected."), this);
    intro->setObjectName("Muted");
    intro->setWordWrap(true);
    root->addWidget(intro);

    m_table = new QTableWidget(0, ColCount, this);
    m_table->setObjectName("ServiceTable");
    m_table->setHorizontalHeaderLabels({tr("ID"), tr("Service"), tr("Description"), tr("Status")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setWordWrap(false);
    m_table->setSortingEnabled(false);
    m_table->verticalHeader()->setVisible(false);
    configureResizableTable(m_table, {90, 150, 280, 90}, {78, 120, 180, 80});
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &ServiceDialog::onSelectionChanged);
    root->addWidget(m_table, 1);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName("ErrorLabel");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    root->addWidget(m_errorLabel);

    auto* form = new QFormLayout();
    m_name = new QLineEdit(this);
    m_name->setObjectName("ServiceNameEdit");
    m_name->setPlaceholderText(tr("e.g. General Consultation"));
    m_description = new QLineEdit(this);
    m_description->setObjectName("ServiceDescriptionEdit");
    m_description->setPlaceholderText(tr("Optional description"));
    form->addRow(tr("New service"), m_name);
    form->addRow(tr("Description"), m_description);
    root->addLayout(form);

    auto* buttons = new QHBoxLayout();
    m_addBtn = new QPushButton(tr("Add service"), this);
    m_addBtn->setObjectName("Primary");
    m_addBtn->setToolTip(tr("Add the service typed above to the catalogue"));
    m_toggleBtn = new QPushButton(tr("Deactivate"), this);
    m_toggleBtn->setToolTip(tr("Hide or show the selected service in the appointment form"));
    auto* closeBtn = new QPushButton(tr("Close"), this);
    buttons->addWidget(m_addBtn);
    buttons->addWidget(m_toggleBtn);
    buttons->addStretch(1);
    buttons->addWidget(closeBtn);
    root->addLayout(buttons);

    connect(m_addBtn, &QPushButton::clicked, this, &ServiceDialog::onAddClicked);
    connect(m_toggleBtn, &QPushButton::clicked, this, &ServiceDialog::onToggleClicked);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    fillTable();
    onSelectionChanged();
}

void ServiceDialog::fillTable() {
    const QString keep = selectedServiceId();
    const auto& services = m_controller.services();
    m_table->blockSignals(true);
    m_table->clearContents();
    m_table->setRowCount(static_cast<int>(services.size()));
    int reselect = -1;
    for (int r = 0; r < static_cast<int>(services.size()); ++r) {
        const auto& s = services[static_cast<std::size_t>(r)];
        auto put = [&](int col, const QString& text) {
            auto* item = new QTableWidgetItem(text);
            if (!s.active) item->setForeground(QBrush(QColor("#8A97A0")));
            m_table->setItem(r, col, item);
        };
        put(ColId, q(s.id));
        put(ColName, q(s.name));
        put(ColDescription, q(s.description));
        put(ColStatus, s.active ? tr("Active") : tr("Inactive"));
        if (q(s.id) == keep) reselect = r;
    }
    if (reselect >= 0) m_table->selectRow(reselect);
    m_table->blockSignals(false);
    onSelectionChanged();
}

QString ServiceDialog::selectedServiceId() const {
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return {};
    return m_table->item(rows.first().row(), ColId)->text();
}

void ServiceDialog::onSelectionChanged() {
    const QString id = selectedServiceId();
    m_toggleBtn->setEnabled(!id.isEmpty());
    if (id.isEmpty()) {
        m_toggleBtn->setText(tr("Deactivate"));
        return;
    }
    const int row = m_table->selectionModel()->selectedRows().first().row();
    const bool active = m_table->item(row, ColStatus)->text() == tr("Active");
    m_toggleBtn->setText(active ? tr("Deactivate") : tr("Activate"));
}

void ServiceDialog::onAddClicked() {
    if (!runGuarded([&] {
            m_controller.addService(m_name->text().toStdString(), m_description->text().toStdString());
        })) return;
    m_changed = true;
    m_name->clear();
    m_description->clear();
    fillTable();
}

void ServiceDialog::onToggleClicked() {
    const QString id = selectedServiceId();
    if (id.isEmpty()) return;
    const int row = m_table->selectionModel()->selectedRows().first().row();
    const bool nowActive = m_table->item(row, ColStatus)->text() == tr("Active");
    if (!runGuarded([&] { m_controller.setServiceActive(id.toStdString(), !nowActive); })) return;
    m_changed = true;
    fillTable();
}

bool ServiceDialog::runGuarded(const std::function<void()>& action) {
    QString message;
    try {
        action();
        m_errorLabel->hide();
        return true;
    } catch (const utils::ClinicException& e) {
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
