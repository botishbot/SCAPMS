#include "utils/FileHandler.h"

#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/ClinicException.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <cmath>

namespace scapms::utils {

using model::Appointment;
using model::AppointmentStatus;
using model::ClinicDatabase;
using model::ClinicService;
using model::PatientCategory;
using model::PatientRecord;

namespace {

QString q(const std::string& s) { return QString::fromUtf8(s.c_str()); }
std::string s8(const QString& s) { return s.toStdString(); }

[[noreturn]] void fail(ErrorCode code, const QString& message) { throw ClinicException(code, s8(message)); }

// ---------------------------------------------------------------- reading helpers
QString reqString(const QJsonObject& o, const char* key, const QString& ctx, bool allowEmpty = false) {
    const QJsonValue v = o.value(key);
    if (!v.isString()) fail(ErrorCode::FileError, ctx + ": the field \"" + key + "\" is missing or is not text.");
    const QString text = v.toString();
    if (!allowEmpty && text.trimmed().isEmpty())
        fail(ErrorCode::FileError, ctx + ": the field \"" + key + "\" is empty.");
    return text;
}

QString optString(const QJsonObject& o, const char* key, const QString& ctx) {
    const QJsonValue v = o.value(key);
    if (v.isUndefined() || v.isNull()) return QString();
    if (!v.isString()) fail(ErrorCode::FileError, ctx + ": the field \"" + key + "\" must be text.");
    return v.toString();
}

int reqInt(const QJsonObject& o, const char* key, const QString& ctx, ErrorCode code = ErrorCode::FileError) {
    const QJsonValue v = o.value(key);
    if (!v.isDouble() || v.toDouble() != std::floor(v.toDouble()) || std::fabs(v.toDouble()) > 1e9)
        fail(code, ctx + ": the field \"" + key + "\" must be a whole number.");
    return v.toInt();
}

QJsonObject objectAt(const QJsonArray& array, int index, const QString& ctx) {
    if (!array.at(index).isObject()) fail(ErrorCode::FileError, ctx + " is not an object.");
    return array.at(index).toObject();
}

QJsonArray reqArray(const QJsonObject& o, const char* key, const QString& ctx) {
    const QJsonValue v = o.value(key);
    if (!v.isArray()) fail(ErrorCode::FileError, ctx + "the list \"" + key + "\" is missing.");
    return v.toArray();
}

const char* categoryKey(PatientCategory c) {
    switch (c) {
        case PatientCategory::General: return "General";
        case PatientCategory::Emergency: return "Emergency";
        case PatientCategory::FollowUp: return "FollowUp";
    }
    return "General";
}

AppointmentStatus parseStatus(const QString& text, const QString& ctx) {
    if (text == "Pending") return AppointmentStatus::Pending;
    if (text == "Completed") return AppointmentStatus::Completed;
    if (text == "Cancelled") return AppointmentStatus::Cancelled;
    fail(ErrorCode::FileError, ctx + ": unknown appointment status \"" + text + "\".");
}

Appointment parseAppointment(const QJsonObject& o, const QString& ctx, const ClinicDatabase& staging,
                             QSet<QString>& seenIds) {
    const QString id = reqString(o, "id", ctx);
    const QString where = ctx + " (" + id + ")";
    if (seenIds.contains(id)) fail(ErrorCode::DuplicateId, "Appointment ID already exists: " + id);
    seenIds.insert(id);

    const QString serviceId = reqString(o, "serviceId", where);
    if (!staging.findService(s8(serviceId)))
        fail(ErrorCode::FileError, where + ": it refers to an unknown service (" + serviceId + ").");
    const AppointmentStatus status = parseStatus(reqString(o, "status", where), where);
    try {
        Appointment a(s8(id), s8(serviceId), s8(reqString(o, "serviceName", where)),
                      s8(reqString(o, "dateTime", where)), s8(optString(o, "reason", where)));
        a.setStatus(status);
        a.setOutcomeNote(s8(optString(o, "outcomeNote", where)));
        return a;
    } catch (const ClinicException& e) {
        fail(e.code(), where + ": " + q(e.what()));
    }
}

std::unique_ptr<PatientRecord> parsePatient(const QJsonObject& o, const QString& ctx) {
    const QString id = reqString(o, "id", ctx);
    const QString where = ctx + " (" + id + ")";
    const QString category = reqString(o, "category", where);
    const std::string name = s8(reqString(o, "name", where));
    const int age = reqInt(o, "age", where, ErrorCode::InvalidAge);
    const std::string phone = s8(optString(o, "phone", where));
    const std::string email = s8(optString(o, "email", where));
    try {
        if (category == "General")
            return std::make_unique<model::GeneralPatient>(s8(id), name, age, phone, email);
        if (category == "Emergency")
            return std::make_unique<model::EmergencyPatient>(s8(id), name, age, phone, email,
                                                             reqInt(o, "triageLevel", where));
        if (category == "FollowUp")
            return std::make_unique<model::FollowUpPatient>(s8(id), name, age, phone, email,
                                                            s8(reqString(o, "previousVisitDate", where)),
                                                            s8(optString(o, "followUpReason", where)));
    } catch (const ClinicException& e) {
        fail(e.code(), where + ": " + q(e.what()));
    }
    fail(ErrorCode::FileError, where + ": unknown patient category \"" + category + "\".");
}

// ---------------------------------------------------------------- writing helpers
QJsonObject appointmentToJson(const Appointment& a) {
    QJsonObject o;
    o["id"] = q(a.id());
    o["serviceId"] = q(a.serviceId());
    o["serviceName"] = q(a.serviceNameSnapshot());
    o["dateTime"] = q(a.dateTime());
    o["reason"] = q(a.reason());
    o["status"] = model::statusName(a.status());
    o["outcomeNote"] = q(a.outcomeNote());
    return o;
}

QString csvField(const QString& text) {
    if (!text.contains(',') && !text.contains('"') && !text.contains('\n')) return text;
    QString escaped = text;
    escaped.replace('"', "\"\"");
    return "\"" + escaped + "\"";
}

} // namespace

// ---------------------------------------------------------------- JSON out
QByteArray FileHandler::toJson(const ClinicDatabase& db) {
    QJsonObject root;
    root["format"] = "SCAPMS";
    root["schemaVersion"] = kSchemaVersion;

    QJsonArray services;
    for (const ClinicService& s : db.services()) {
        QJsonObject o;
        o["id"] = q(s.id);
        o["name"] = q(s.name);
        o["description"] = q(s.description);
        o["active"] = s.active;
        services.append(o);
    }
    root["services"] = services;

    QJsonArray patients;
    for (const auto& p : db.patients()) {
        QJsonObject o;
        o["id"] = q(p->id());
        o["category"] = categoryKey(p->category());
        o["name"] = q(p->name());
        o["age"] = p->age();
        o["phone"] = q(p->phone());
        o["email"] = q(p->email());
        if (const auto* e = dynamic_cast<const model::EmergencyPatient*>(p.get())) o["triageLevel"] = e->triageLevel();
        if (const auto* f = dynamic_cast<const model::FollowUpPatient*>(p.get())) {
            o["previousVisitDate"] = q(f->previousVisitDate());
            o["followUpReason"] = q(f->followUpReason());
        }
        QJsonArray appts;
        for (const Appointment& a : p->appointments()) appts.append(appointmentToJson(a));
        o["appointments"] = appts;
        patients.append(o);
    }
    root["patients"] = patients;

    QJsonArray history;
    for (const Appointment& a : db.history()) {
        QJsonObject o = appointmentToJson(a);
        o["patientId"] = q(a.patientId());
        o["patientName"] = q(a.patientNameSnapshot());
        history.append(o);
    }
    root["history"] = history;

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

// ---------------------------------------------------------------- JSON in (validate everything first)
void FileHandler::fromJson(const QByteArray& json, ClinicDatabase& target, IDGenerator& ids) {
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (doc.isNull() || !doc.isObject())
        fail(ErrorCode::FileError, "This is not a valid SCAPMS data file (" +
                                       (parseError.error != QJsonParseError::NoError ? parseError.errorString()
                                                                                    : QString("it is not a JSON object")) + ").");
    const QJsonObject root = doc.object();
    if (root.value("format").toString() != "SCAPMS")
        fail(ErrorCode::FileError, "This file was not created by SCAPMS (missing format marker).");
    const QJsonValue version = root.value("schemaVersion");
    if (!version.isDouble()) fail(ErrorCode::FileError, "The file has no schema version.");
    if (version.toInt() > kSchemaVersion)
        fail(ErrorCode::FileError, "This file was created by a newer version of SCAPMS (format " +
                                       QString::number(version.toInt()) + ") and cannot be read.");
    if (version.toInt() < 1) fail(ErrorCode::FileError, "The file has an invalid schema version.");

    // Everything is built in a separate staging database. The live data is untouched until the very end.
    ClinicDatabase staging;
    IDGenerator stagingIds;
    QSet<QString> appointmentIds;

    const QJsonArray services = reqArray(root, "services", "");
    for (int i = 0; i < services.size(); ++i) {
        const QString ctx = QString("services[%1]").arg(i);
        const QJsonObject o = objectAt(services, i, ctx);
        ClinicService s;
        s.id = s8(reqString(o, "id", ctx));
        s.name = s8(reqString(o, "name", ctx));
        s.description = s8(optString(o, "description", ctx));
        if (!o.value("active").isBool()) fail(ErrorCode::FileError, ctx + ": the field \"active\" must be true or false.");
        s.active = o.value("active").toBool();
        try {
            staging.addService(s);
        } catch (const ClinicException& e) {
            fail(e.code(), ctx + ": " + q(e.what()));
        }
        stagingIds.observe(s.id);
    }

    const QJsonArray patients = reqArray(root, "patients", "");
    for (int i = 0; i < patients.size(); ++i) {
        const QString ctx = QString("patients[%1]").arg(i);
        const QJsonObject o = objectAt(patients, i, ctx);
        auto patient = parsePatient(o, ctx);
        const QString where = ctx + " (" + q(patient->id()) + ")";

        const QJsonArray appts = reqArray(o, "appointments", where + ": ");
        for (int k = 0; k < appts.size(); ++k) {
            const QString actx = QString("%1.appointments[%2]").arg(where).arg(k);
            Appointment a = parseAppointment(objectAt(appts, k, actx), actx, staging, appointmentIds);
            stagingIds.observe(a.id());
            try {
                patient->addLoadedAppointment(a);
            } catch (const ClinicException& e) {
                fail(e.code(), actx + ": " + q(e.what()));
            }
        }
        stagingIds.observe(patient->id());
        try {
            staging.addPatient(std::move(patient));  // throws DuplicateId: "Patient ID already exists"
        } catch (const ClinicException& e) {
            fail(e.code(), where + ": " + q(e.what()));
        }
    }

    const QJsonArray history = reqArray(root, "history", "");
    std::vector<Appointment> historyList;
    for (int i = 0; i < history.size(); ++i) {
        const QString ctx = QString("history[%1]").arg(i);
        const QJsonObject o = objectAt(history, i, ctx);
        Appointment a = parseAppointment(o, ctx, staging, appointmentIds);
        a.setPatient(s8(reqString(o, "patientId", ctx)), s8(reqString(o, "patientName", ctx, true)));
        stagingIds.observe(a.id());
        historyList.push_back(a);
    }
    staging.archiveAppointments(historyList);

    // The whole file is valid: replace the live data.
    target.replaceContentsWith(std::move(staging));
    ids = stagingIds;
}

// ---------------------------------------------------------------- files
void FileHandler::save(const ClinicDatabase& db, const QString& path) {
    const QByteArray json = toJson(db);
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);  // writes to a temporary file and renames on commit, so a crash cannot corrupt the old file
    if (!file.open(QIODevice::WriteOnly))
        fail(ErrorCode::FileError, "Could not save to \"" + path + "\": " + file.errorString());
    if (file.write(json) != json.size() || !file.commit())
        fail(ErrorCode::FileError, "Could not save to \"" + path + "\": " + file.errorString());
}

void FileHandler::load(const QString& path, ClinicDatabase& target, IDGenerator& ids) {
    QFile file(path);
    if (!file.exists()) fail(ErrorCode::FileError, "The file \"" + path + "\" does not exist.");
    if (!file.open(QIODevice::ReadOnly))
        fail(ErrorCode::FileError, "Could not open \"" + path + "\": " + file.errorString());
    fromJson(file.readAll(), target, ids);
}

// ---------------------------------------------------------------- export
QByteArray FileHandler::patientsCsv(const ClinicDatabase& db) {
    QString csv = "Patient ID,Name,Age,Category,Priority,Phone,Email,Details,Appointments,Completed,Pending\n";
    for (const auto& p : db.patients()) {
        const QStringList fields = {q(p->id()),
                                    csvField(q(p->name())),
                                    QString::number(p->age()),
                                    q(p->categoryName()),
                                    q(p->priorityLabel()),
                                    csvField(q(p->phone())),
                                    csvField(q(p->email())),
                                    csvField(q(p->summary())),
                                    QString::number(p->appointments().size()),
                                    QString::number(p->completedCount()),
                                    QString::number(p->pendingCount())};
        csv += fields.join(',') + "\n";
    }
    return csv.toUtf8();
}

void FileHandler::exportPatientsCsv(const ClinicDatabase& db, const QString& path) {
    writeTextFile(path, patientsCsv(db).toStdString());
}

void FileHandler::writeTextFile(const QString& path, const std::string& text) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        fail(ErrorCode::FileError, "Could not write \"" + path + "\": " + file.errorString());
    const QByteArray data = QByteArray::fromStdString(text);
    if (file.write(data) != data.size() || !file.commit())
        fail(ErrorCode::FileError, "Could not write \"" + path + "\": " + file.errorString());
}

} // namespace scapms::utils
