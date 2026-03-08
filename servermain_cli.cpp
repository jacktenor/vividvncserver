#include <QGuiApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QTimer>

#include "vncserver.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName("VividVNCServer");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser p;
    p.setApplicationDescription("VividVNC Server (headless CLI)");
    p.addHelpOption();
    p.addVersionOption();

    QCommandLineOption optPort({"p","port"}, "VNC port", "port", "5903");
    QCommandLineOption optFps({"f","fps"}, "Capture FPS", "fps", "60");
    QCommandLineOption optPassword({"P","password"}, "Password (optional)", "password", "");
    QCommandLineOption optRequirePass("require-password", "Require password");
    QCommandLineOption optListenAll("listen-all", "Listen on all interfaces (NOT just localhost)");

    p.addOption(optPort);
    p.addOption(optFps);
    p.addOption(optPassword);
    p.addOption(optRequirePass);
    p.addOption(optListenAll);

    p.process(app);

    const int port = p.value(optPort).toInt();
    const int fps  = p.value(optFps).toInt();

    VncServer server;
    if (!p.value(optPassword).isEmpty())
        server.setPassword(p.value(optPassword));
    server.setRequirePassword(p.isSet(optRequirePass));
    server.setListenLocalhostOnly(!p.isSet(optListenAll));

    if (!server.start(port, fps)) {
        qCritical("Failed to start VNC server.");
        return 2;
    }

    return app.exec();
}
