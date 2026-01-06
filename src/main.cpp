#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QMessageBox>
#include <iostream>

#include "app/settings.hxx"
#include "db/database.hxx"
#include "gui/main_window.hxx"

int main(int argc, char *argv[]) {
  try {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("LocalTaskManager");
    QCoreApplication::setApplicationName("LocalTaskManager");

    QCommandLineParser parser;
    parser.setApplicationDescription("LocalTaskManager");
    parser.addHelpOption();

    QCommandLineOption configOpt(
        QStringList{"c", "config"},
        "Path to ini config file (default: рядом с программой).", "path");
    QCommandLineOption dbOpt(
        QStringList{"d", "db"},
        "Path to sqlite database file. If set, it will be saved to ini.",
        "path");

    parser.addOption(configOpt);
    parser.addOption(dbOpt);
    parser.process(app);

    QString iniPath = parser.value(configOpt).trimmed();
    if (iniPath.isEmpty())
      iniPath = defaultIniPath();

    const QString cliDbPath = parser.value(dbOpt).trimmed();

    const AppSettings st = loadOrCreateSettings(iniPath, cliDbPath);

    auto opened = db::openDatabaseReadOrCreate(st.dbPath);

    if (opened.createdNew) {
      db::ensureSchemaBestEffort(*opened.db);
      db::seedIfEmpty(*opened.db);
    }

    MainWindow w(opened.db);
    w.show();

    return app.exec();
  } catch (const std::exception &e) {
    try {
      QMessageBox::critical(nullptr, "Fatal error", e.what());
    } catch (...) {
      std::cerr << "Fatal error: " << e.what() << std::endl;
    }
    return 1;
  }
}