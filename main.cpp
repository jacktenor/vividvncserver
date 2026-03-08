#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("VividVNCServer");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("VividVNC Server");
    parser.addHelpOption();
    parser.addVersionOption();

    // Command line options
    QCommandLineOption optAutostart("autostart", "Start server immediately without showing UI");
    QCommandLineOption optPort("port", "VNC port", "port", "5903");
    QCommandLineOption optFps("fps", "Capture FPS", "fps", "60");
    QCommandLineOption optPassword("password", "Password", "password");
    QCommandLineOption optRequirePassword("require-password", "Require password");
    QCommandLineOption optListenAll("listen-all", "Listen on all interfaces");

    parser.addOption(optAutostart);
    parser.addOption(optPort);
    parser.addOption(optFps);
    parser.addOption(optPassword);
    parser.addOption(optRequirePassword);
    parser.addOption(optListenAll);

    parser.process(app);

    MainWindow w;

    if (parser.isSet(optAutostart)) {

        // Apply CLI settings
        int port = parser.value(optPort).toInt();
        int fps  = parser.value(optFps).toInt();

        bool requirePassword = parser.isSet(optRequirePassword);
        bool localhostOnly   = !parser.isSet(optListenAll);

        QString password = parser.value(optPassword);

        w.setListenLocalhostOnly(localhostOnly);
        w.setRequirePassword(requirePassword);
        w.setPassword(password);

        if (!w.startServer(port, fps)) {
            qCritical("Failed to start VNC server.");
            return 2;
        }

        // Do NOT show UI
        return app.exec();
    }

    // Normal GUI mode
    w.show();

    return app.exec();
}
