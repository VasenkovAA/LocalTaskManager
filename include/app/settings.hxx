#pragma once

#include <QString>

struct AppSettings
{
  QString iniPath;
  QString dbPath;
};

QString defaultIniPath();

AppSettings loadOrCreateSettings(const QString& iniPath, const QString& cliDbPath);