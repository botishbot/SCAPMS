#pragma once
#include "model/ClinicDatabase.h"
#include "utils/IDGenerator.h"

#include <QByteArray>
#include <QString>

namespace scapms::utils {

// Saves and loads all clinic data as JSON (patients with their appointments, services, history).
// Loading is all-or-nothing: the whole file is read into a separate staging database and checked;
// only if everything is valid is the live data replaced. A bad file never damages current data.
// All problems are reported as ClinicException with a message the user can understand.
class FileHandler {
public:
    static constexpr int kSchemaVersion = 1;

    // ---- JSON text ----
    static QByteArray toJson(const model::ClinicDatabase& database);
    static void fromJson(const QByteArray& json, model::ClinicDatabase& target, IDGenerator& ids);

    // ---- files (path may be a Qt resource such as ":/..." when loading) ----
    static void save(const model::ClinicDatabase& database, const QString& path);  // atomic: all or nothing
    static void load(const QString& path, model::ClinicDatabase& target, IDGenerator& ids);

    // ---- export ----
    static QByteArray patientsCsv(const model::ClinicDatabase& database);
    static void exportPatientsCsv(const model::ClinicDatabase& database, const QString& path);
    static void writeTextFile(const QString& path, const std::string& text);
};

} // namespace scapms::utils
