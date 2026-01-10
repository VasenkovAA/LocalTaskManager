#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QString>

#include "app/settings.hxx"
#include "db/database.hxx"
#include "gui/main_window.hxx"

int main(int argc, char *argv[]) {
  try {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("LocalTaskManager");
    QApplication::setApplicationName("LocalTaskManager");

    QCommandLineParser parser;
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
    } else {

      db::seedIfEmpty(*opened.db);
    }

    MainWindow w(opened.db);
    w.show();

    return app.exec();
  } catch (const std::exception &e) {
    fprintf(stderr, "Fatal error: %s\n", e.what());
    return 1;
  }
}