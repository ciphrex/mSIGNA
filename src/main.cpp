///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// main.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "settings.h"

#include <QApplication>
#include <QDateTime>

#include "splashscreen.h"
#include "mainwindow.h"
#include "commandserver.h"

#include "severitylogger.h"

int main(int argc, char* argv[])
{
    INIT_LOGGER((APPDATADIR + "/debug.log").toStdString().c_str());
    LOGGER(debug) << std::endl << std::endl << std::endl << std::endl << QDateTime::currentDateTime().toString().toStdString() << std::endl;

    Q_INIT_RESOURCE(coinvault);

    QApplication app(argc, argv);
    app.setOrganizationName("Ciphrex");
    app.setApplicationName(APPNAME);

    CommandServer commandServer(&app);
    if (commandServer.processArgs(argc, argv)) exit(0);

    LOGGER(debug) << "Vault started." << std::endl;

    SplashScreen splash;

    splash.show();
    splash.setAutoFillBackground(true);

    app.processEvents();
    splash.showMessage("\n  Loading settings...");
    MainWindow mainWin; // constructor loads settings
    QObject::connect(&mainWin, &MainWindow::status, [&](const QString& message) { splash.showMessage(message); });

    splash.showMessage("\n  Starting command server...");
    if (!commandServer.start()) {
        LOGGER(debug) << "Could not start command server." << std::endl;
    }
    else {
        app.connect(&commandServer, SIGNAL(gotUrl(const QUrl&)), &mainWin, SLOT(processUrl(const QUrl&)));
        app.connect(&commandServer, SIGNAL(gotFile(const QString&)), &mainWin, SLOT(processFile(const QString&)));
    }

    app.processEvents();
    splash.showMessage("\n  Loading block headers...");
    app.processEvents();
    mainWin.loadBlockTree();
    mainWin.tryConnect();

    mainWin.show();
    splash.finish(&mainWin);
    commandServer.uiReady();

    int rval = app.exec();
    LOGGER(debug) << "Program stopped with code " << rval << std::endl;
    return rval;
}
