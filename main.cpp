#include <QApplication>
#include <QFileInfo>
#include <QLocale>
#include <QTranslator>
#include <stdio.h>
#include <stdlib.h>
#include "mainwindow.h"

void handleMessage(QtMsgType type, const char *msg, bool printDebugMessages)
{
    switch (type) {
    case QtDebugMsg:
        if (printDebugMessages) fprintf(stderr, "%s\n", msg);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s\n", msg);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", msg);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", msg);
        abort();
    }
}

void defaultMessageHandler(QtMsgType type, const char *msg)
{
    handleMessage(type, msg, false);
}

void debugMessageHandler(QtMsgType type, const char *msg)
{
    handleMessage(type, msg, true);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("TVkaistaGUI");

    QStringList arguments = app.arguments();
    int count = arguments.size();
    bool invalidArgs = false;

    qInstallMsgHandler(defaultMessageHandler);

    for (int i = 1; i < count; i++) {
        QString arg = arguments.at(i);

        if (arg == "-d" || arg == "--debug") {
            qInstallMsgHandler(debugMessageHandler);
        }
        else {
            invalidArgs = true;
        }
    }

    if (invalidArgs) {
        fprintf(stderr, "Usage: tvkaistagui [-d|--debug]");
        return 1;
    }

#ifdef TVKAISTAGUI_TRANSLATIONS_DIR
    QString translationsDir = TVKAISTAGUI_TRANSLATIONS_DIR;
#else
    QString translationsDir = app.applicationDirPath();
    translationsDir.append("/translations");
#endif

    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(), translationsDir);
    app.installTranslator(&qtTranslator);
    MainWindow window;
    window.show();
    return app.exec();
}
