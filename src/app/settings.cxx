#include "app/settings.hxx"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <stdexcept>

static QString appDir() { return QCoreApplication::applicationDirPath(); }

static QString ensureDirOrThrow(const QString& p)
{
  QDir d(p);
  if (!d.exists() && !d.mkpath("."))
    throw std::runtime_error(("Failed to create directory: " + p).toStdString());
  return d.absolutePath();
}

QString defaultIniPath()
{
  const QString cfgDir =
      ensureDirOrThrow(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
  return QDir(cfgDir).filePath("LocalTaskManager.ini");
}

static QString defaultDbPathForIniDir(const QString& /*iniPath*/)
{
  const QString dataDir =
      ensureDirOrThrow(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
  return QDir(dataDir).filePath("task_database.db");
}

static QString resolveMaybeRelativePath(const QString &iniPath,
                                        const QString &path) {
  QFileInfo fi(path);
  if (fi.isAbsolute())
    return QDir::cleanPath(path);

  const QDir iniDir(QFileInfo(iniPath).absolutePath());
  return QDir::cleanPath(iniDir.filePath(path));
}

AppSettings loadOrCreateSettings(const QString &iniPath,
                                 const QString &cliDbPath) {
  if (iniPath.trimmed().isEmpty())
    throw std::runtime_error("Empty ini path");

  QFileInfo iniFi(iniPath);
  QDir iniDir(iniFi.absolutePath());
  if (!iniDir.exists() && !iniDir.mkpath("."))
    throw std::runtime_error(
        ("Failed to create ini directory: " + iniDir.absolutePath())
            .toStdString());

  const bool iniExists = QFile::exists(iniPath);

  QSettings s(iniPath, QSettings::IniFormat);

  if (!iniExists) {
    s.beginGroup("Database");
    s.setValue("Path", "data/task_database.db");
    s.endGroup();
    s.sync();
  }

  QString dbPath;
  if (!cliDbPath.trimmed().isEmpty()) {
    dbPath = resolveMaybeRelativePath(iniPath, cliDbPath.trimmed());

    s.beginGroup("Database");
    s.setValue("Path", dbPath);
    s.endGroup();
    s.sync();
  } else {
    s.beginGroup("Database");
    const QString raw =
        s.value("Path", "data/task_database.db").toString().trimmed();
    s.endGroup();

    if (raw.isEmpty()) {
      dbPath = defaultDbPathForIniDir(iniPath);

      s.beginGroup("Database");
      s.setValue("Path", "data/task_database.db");
      s.endGroup();
      s.sync();
    } else {
      dbPath = resolveMaybeRelativePath(iniPath, raw);
    }
  }

  AppSettings out;
  out.iniPath = QDir::cleanPath(iniPath);
  out.dbPath = QDir::cleanPath(dbPath);
  return out;
}