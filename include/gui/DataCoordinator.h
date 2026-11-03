#pragma once
#include "controller/PatientCache.h"
#include "controller/QueueManager.h"
#include "controller/UndoManager.h"
#include "model/ClinicDatabase.h"
#include "utils/IDGenerator.h"

#include <QObject>
#include <QString>
#include <functional>

class QWidget;

namespace scapms::gui {

// Runs the "whole data set" workflows: New, Save, Save As, Load, demo data, CSV export, and the
// "unsaved changes" question. It talks to the user through dialogs and to the data through FileHandler.
// Load, New and demo data replace the current data (they never merge) and also clear the undo stack, queue and cache.
class DataCoordinator : public QObject {
    Q_OBJECT
public:
    // Opens a file dialog. forSave = true for Save/Export. Returns "" if the user cancels.
    using FileChooser = std::function<QString(bool forSave, const QString& caption, const QString& suggestedPath,
                                              const QString& filter)>;

    DataCoordinator(model::ClinicDatabase& database, utils::IDGenerator& ids, controller::UndoManager& undo,
                    controller::QueueManager& queue, controller::PatientCache& cache, QWidget* dialogParent,
                    QObject* parent = nullptr);

    void setDirtyProvider(std::function<bool()> provider) { m_isDirty = std::move(provider); }
    void setFileChooser(FileChooser chooser) { m_chooser = std::move(chooser); }  // lets tests skip real dialogs

    QString currentPath() const { return m_path; }
    static QString defaultPath();

    void loadStartupFile();       // loads the default data file if there is one
    bool confirmSaveOrDiscard();  // "unsaved changes" question. false = the user cancelled

public slots:
    void newData();
    bool save();
    bool saveAs();
    void load();
    void exportPatientsCsv();
    void loadDemo();

signals:
    void dataReplaced(bool markDirty);  // load / new / demo finished: the whole screen must refresh
    void dataSaved();
    void pathChanged();
    void statusMessage(const QString& text);

private:
    QString choose(bool forSave, const QString& caption, const QString& suggested, const QString& filter);
    void resetSessionState();           // undo stack, waiting queue and cache belong to the old data
    void showError(const QString& title, const QString& message);
    bool databaseHasData() const;

    model::ClinicDatabase& m_database;
    utils::IDGenerator& m_ids;
    controller::UndoManager& m_undo;
    controller::QueueManager& m_queue;
    controller::PatientCache& m_cache;
    QWidget* m_parent;
    std::function<bool()> m_isDirty;
    FileChooser m_chooser;
    QString m_path;
};

} // namespace scapms::gui
