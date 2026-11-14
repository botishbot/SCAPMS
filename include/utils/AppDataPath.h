#pragma once

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QString>

namespace scapms::utils {

// The default database belongs to the current user's application-data directory,
// never beside the executable or source tree.
inline QString applicationDataFilePath() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath("scapms_data.json");
}

// Older builds could store the automatically selected database in ../data beside
// the executable. Ignore only that legacy automatic path; preserve user-selected paths.
inline bool isLegacyProjectDataPath(const QString& candidate, const QString& applicationDir) {
    const QString legacy = QDir(applicationDir).absoluteFilePath("../data/scapms_data.json");
    return QDir::cleanPath(QFileInfo(candidate).absoluteFilePath()) ==
           QDir::cleanPath(QFileInfo(legacy).absoluteFilePath());
}

} // namespace scapms::utils
