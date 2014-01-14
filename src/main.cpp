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

// For splash screen timer
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

const int MINIMUM_SPLASH_SECS = 5;

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

    // Require splash screen to always remain open for at least a couple seconds
    bool waiting = true;
    boost::asio::io_service timer_io;
    boost::asio::deadline_timer timer(timer_io, boost::posix_time::seconds(MINIMUM_SPLASH_SECS));
    timer.async_wait([&](const boost::system::error_code& /*ec*/) { waiting = false; });
    timer_io.run();

    app.processEvents();
    splash.showMessage("\n  Initializing...");
    app.processEvents();
    while (waiting) { usleep(200); }
    timer_io.stop();

    mainWin.tryConnect();

    mainWin.show();
    splash.finish(&mainWin);
    commandServer.uiReady();

    int rval = app.exec();
    LOGGER(debug) << "Program stopped with code " << rval << std::endl;
    return rval;
}
