#include "gui/DataCoordinator.h"

#include "controller/DemoData.h"
#include "utils/ClinicException.h"
#include "utils/FileHandler.h"

#include <QDate>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>

namespace scapms::gui {

using utils::ClinicException;
using utils::FileHandler;

DataCoordinator::DataCoordinator(model::ClinicDatabase& database, utils::IDGenerator& ids,
                                 controller::UndoManager& undo, controller::QueueManager& queue,
                                 controller::PatientCache& cache, QWidget* dialogParent, QObject* parent)
    : QObject(parent), m_database(database), m_ids(ids), m_undo(undo), m_queue(queue), m_cache(cache),
      m_parent(dialogParent), m_path(defaultPath()) {}

QString DataCoordinator::defaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/scapms_data.json";
}

QString DataCoordinator::choose(bool forSave, const QString& caption, const QString& suggested, const QString& filter) {
    if (m_chooser) return m_chooser(forSave, caption, suggested, filter);
    return forSave ? QFileDialog::getSaveFileName(m_parent, caption, suggested, filter)
                   : QFileDialog::getOpenFileName(m_parent, caption, suggested, filter);
}

void DataCoordinator::showError(const QString& title, const QString& message) {
    QMessageBox::warning(m_parent, title, message);
}

bool DataCoordinator::databaseHasData() const {
    return m_database.patientCount() > 0 || !m_database.services().empty() || !m_database.history().empty();
}

void DataCoordinator::resetSessionState() {
    m_undo.clear();   // undo entries refer to the old data
    m_queue.clear();  // the waiting queue is not saved data
    m_cache.clear();
}

// ---------------------------------------------------------------- unsaved changes
bool DataCoordinator::confirmSaveOrDiscard() {
    if (!m_isDirty || !m_isDirty()) return true;
    const auto answer = QMessageBox::question(
        m_parent, tr("Unsaved changes"), tr("You have unsaved changes.\n\nDo you want to save them first?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Save) return save();
    return answer == QMessageBox::Discard;
}

// ---------------------------------------------------------------- startup
void DataCoordinator::loadStartupFile() {
    if (!QFileInfo::exists(m_path)) return;  // first run: nothing saved yet
    try {
        FileHandler::load(m_path, m_database, m_ids);
    } catch (const ClinicException& e) {
        showError(tr("Could not open saved data"),
                  tr("The saved data file could not be loaded, so SCAPMS started empty.\n\n%1\n\n"
                     "The file was not changed. Use File > Save As to keep your new work elsewhere.")
                      .arg(QString::fromUtf8(e.what())));
        m_path.clear();  // do not overwrite a file that could not be read
        emit pathChanged();
        return;
    } catch (...) {
        showError(tr("Could not open saved data"), tr("An unexpected error occurred while loading the saved data."));
        m_path.clear();
        emit pathChanged();
        return;
    }
    resetSessionState();
    emit dataReplaced(false);
    emit statusMessage(tr("Loaded %1").arg(QFileInfo(m_path).fileName()));
}

// ---------------------------------------------------------------- new
void DataCoordinator::newData() {
    if (!confirmSaveOrDiscard()) return;
    m_database.replaceContentsWith(model::ClinicDatabase());
    m_ids.reset();
    resetSessionState();
    m_path.clear();  // Save will ask where to save, so the old file is not overwritten by accident
    emit pathChanged();
    emit dataReplaced(false);
    emit statusMessage(tr("Started a new, empty data set"));
}

// ---------------------------------------------------------------- save
bool DataCoordinator::save() {
    if (m_path.isEmpty()) return saveAs();
    try {
        FileHandler::save(m_database, m_path);
    } catch (const ClinicException& e) {
        showError(tr("Could not save"), QString::fromUtf8(e.what()));
        return false;
    } catch (...) {
        showError(tr("Could not save"), tr("An unexpected error occurred. Nothing was changed."));
        return false;
    }
    emit dataSaved();
    emit statusMessage(tr("Saved to %1").arg(QFileInfo(m_path).fileName()));
    return true;
}

bool DataCoordinator::saveAs() {
    QString path = choose(true, tr("Save clinic data"), m_path.isEmpty() ? defaultPath() : m_path,
                          tr("SCAPMS data (*.json)"));
    if (path.isEmpty()) return false;
    if (QFileInfo(path).suffix().isEmpty()) path += ".json";
    try {
        FileHandler::save(m_database, path);
    } catch (const ClinicException& e) {
        showError(tr("Could not save"), QString::fromUtf8(e.what()));
        return false;
    } catch (...) {
        showError(tr("Could not save"), tr("An unexpected error occurred. Nothing was changed."));
        return false;
    }
    m_path = path;
    emit pathChanged();
    emit dataSaved();
    emit statusMessage(tr("Saved to %1").arg(QFileInfo(m_path).fileName()));
    return true;
}

// ---------------------------------------------------------------- load
void DataCoordinator::load() {
    const QString path = choose(false, tr("Load clinic data"), m_path.isEmpty() ? defaultPath() : m_path,
                                tr("SCAPMS data (*.json);;All files (*)"));
    if (path.isEmpty()) return;
    if (!confirmSaveOrDiscard()) return;
    try {
        FileHandler::load(path, m_database, m_ids);  // all-or-nothing: on any problem the current data stays as it is
    } catch (const ClinicException& e) {
        showError(tr("Could not load the file"),
                  tr("%1\n\nYour current data has not been changed.").arg(QString::fromUtf8(e.what())));
        return;
    } catch (...) {
        showError(tr("Could not load the file"), tr("An unexpected error occurred. Your current data has not been changed."));
        return;
    }
    resetSessionState();
    m_path = path;
    emit pathChanged();
    emit dataReplaced(false);
    emit statusMessage(tr("Loaded %1").arg(QFileInfo(path).fileName()));
}

// ---------------------------------------------------------------- export
void DataCoordinator::exportPatientsCsv() {
    QString path = choose(true, tr("Export patients"),
                          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/scapms_patients.csv",
                          tr("CSV files (*.csv)"));
    if (path.isEmpty()) return;
    if (QFileInfo(path).suffix().isEmpty()) path += ".csv";
    try {
        FileHandler::exportPatientsCsv(m_database, path);
    } catch (const ClinicException& e) {
        showError(tr("Could not export"), QString::fromUtf8(e.what()));
        return;
    } catch (...) {
        showError(tr("Could not export"), tr("An unexpected error occurred."));
        return;
    }
    emit statusMessage(tr("Exported %1 patient(s) to %2").arg(m_database.patientCount()).arg(QFileInfo(path).fileName()));
}

// ---------------------------------------------------------------- demo data
void DataCoordinator::loadDemo() {
    if (databaseHasData() || (m_isDirty && m_isDirty())) {
        QMessageBox box(m_parent);
        box.setIcon(QMessageBox::Question);
        box.setWindowTitle(tr("Load demo data"));
        box.setText(tr("Loading the demo data replaces ALL current data. It is never merged with what is here."));
        QPushButton* saveFirst = box.addButton(tr("Save first, then replace"), QMessageBox::AcceptRole);
        QPushButton* replace = box.addButton(tr("Replace without saving"), QMessageBox::DestructiveRole);
        box.addButton(QMessageBox::Cancel);
        box.setDefaultButton(saveFirst);
        box.exec();
        if (box.clickedButton() == saveFirst) {
            if (!save()) return;
        } else if (box.clickedButton() != replace) {
            return;  // cancelled
        }
    }
    model::ClinicDatabase staging;
    utils::IDGenerator stagingIds;
    try {
        controller::DemoData::populate(staging, stagingIds, QDate::currentDate().toString("yyyy-MM-dd").toStdString());
    } catch (const ClinicException& e) {
        showError(tr("Could not create demo data"), QString::fromUtf8(e.what()));
        return;
    }
    m_database.replaceContentsWith(std::move(staging));
    m_ids = stagingIds;
    resetSessionState();
    m_path.clear();  // demo data is not a file yet: saving asks where to put it
    emit pathChanged();
    emit dataReplaced(true);
    emit statusMessage(tr("Loaded demo data (fictional patients)"));
}

} // namespace scapms::gui
