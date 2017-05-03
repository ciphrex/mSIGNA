///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// main.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "settings.h"
#include "versioninfo.h"
#include "coinparams.h"
#include "networkselectiondialog.h"

#include "entropysource.h"

#include <QApplication>
#include <QSettings>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>

#include "splashscreen.h"
#include "acceptlicensedialog.h"
#include "mainwindow.h"
#include "commandserver.h"

#include "severitylogger.h"

// For splash screen timer
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <CoinCore/random.h>

const int MINIMUM_SPLASH_SECS = 5;

void selectNetwork(const std::string& networkName)
{
    std::string selected;
    CoinQ::NetworkSelector& selector = getNetworkSelector();
    if (networkName.empty())
    {
        NetworkSelectionDialog dlg(selector);
        if (!dlg.exec()) exit(0);
        selected = dlg.getNetworkName().toStdString();
    }
    else
    {
        selected = networkName;
    }

    selector.select(selected);
}

void setCurrencyUnit()
{
    QSettings settings("Ciphrex", getDefaultSettings().getNetworkSettingsPath());
    QString prefix = settings.value("currencyunitprefix", "").toString();
    setCurrencyUnitPrefix(prefix);
}

int main(int argc, char* argv[])
{
    secure_random_bytes(32);

#ifdef DEFAULT_NETWORK_LITECOIN
    Q_INIT_RESOURCE(mSIGNA_ltc);
#else
    Q_INIT_RESOURCE(mSIGNA);
#endif

    QApplication app(argc, argv);
    app.setOrganizationName("Ciphrex");
    app.setOrganizationDomain("ciphrex.com");

    // Check whether another instance is already running. If so, send it commands and exit.    
    CommandServer commandServer(&app);
    if (commandServer.processArgs(argc, argv)) exit(0);

    try {
        // Allow selecting a different network than bitcoin at startup.
        // If passed argument is an existing file, don't treat it as a network name.
        if (argc > 1)
        {
            if (std::string(argv[1]) == "select")
            {
                selectNetwork("");
            }
            else if (std::string(argv[1]) == "network")
            {
                if (argc < 3) throw std::runtime_error("No network name specified.");
                selectNetwork(argv[2]);
            }
        }
        getDefaultSettings();
        setCurrencyUnit();
    }
    catch (const std::exception& e) {
        QMessageBox msgBox;
        msgBox.setText(QMessageBox::tr("Error: ") + QString::fromStdString(e.what()));
        msgBox.exec();
        return -1;
    }

    app.setApplicationName(getDefaultSettings().getAppName());
    app.setApplicationVersion(getVersionText());

    QDir datadir(getDefaultSettings().getDataDir());
    if (!datadir.exists() && !datadir.mkpath(getDefaultSettings().getDataDir())) {
        QMessageBox msgBox;
        msgBox.setText(QMessageBox::tr("Warning: Failed to create app data directory."));
        msgBox.exec();
    }

    INIT_LOGGER((getDefaultSettings().getDataDir() + "/debug.log").toStdString().c_str());
    LOGGER(debug) << std::endl << std::endl << std::endl << std::endl << QDateTime::currentDateTime().toString().toStdString() << std::endl;
    LOGGER(debug) << "Vault started." << std::endl;

    //seedEntropySource();

    SplashScreen splash;

    splash.show();
    splash.setAutoFillBackground(true);

    app.processEvents();
    splash.showProgressMessage("Loading settings...");
    MainWindow mainWin; // constructor loads settings
    QObject::connect(&mainWin, &MainWindow::status, [&](const QString& message) { splash.showProgressMessage(message); });
    QObject::connect(&mainWin, &MainWindow::headersLoadProgress, [&](const QString& message)
    {
        splash.showProgressMessage(QString("Loading block headers... ") + message);
        app.processEvents();
    });

    splash.showProgressMessage("Starting command server...");
    app.processEvents();
    if (!commandServer.start()) {
        LOGGER(debug) << "Could not start command server." << std::endl;
    }
    else {
        app.connect(&commandServer, SIGNAL(gotUrl(const QUrl&)), &mainWin, SLOT(processUrl(const QUrl&)));
        app.connect(&commandServer, SIGNAL(gotFile(const QString&)), &mainWin, SLOT(processFile(const QString&)));
        app.connect(&commandServer, &CommandServer::gotCommand, [&](const QString& command, const std::vector<QString>& args)
        {
            mainWin.processCommand(command, args);
        });
    }

    splash.showProgressMessage("Loading block headers...");
    app.processEvents();
    mainWin.loadHeaders();

    // Require splash screen to always remain open for at least a couple seconds
    bool waiting = true;
    boost::asio::io_service timer_io;
    boost::asio::deadline_timer timer(timer_io, boost::posix_time::seconds(MINIMUM_SPLASH_SECS));
    timer.async_wait([&](const boost::system::error_code& /*ec*/) { waiting = false; });
    timer_io.run();

    splash.showProgressMessage("Initializing...");
    app.processEvents();
    while (waiting) { usleep(200); }
    timer_io.stop();

    mainWin.tryConnect();
    mainWin.show();
    splash.finish(&mainWin);

    if (!mainWin.isLicenseAccepted()) {
        //Display license agreement
        AcceptLicenseDialog acceptLicenseDialog;
        if (!acceptLicenseDialog.exec()) {
            LOGGER(error) << "License agreement not accepted." << std::endl;
            return -1;
        }
        LOGGER(debug) << "License agreement accepted." << std::endl;
        mainWin.setLicenseAccepted(true);
        mainWin.saveSettings();
    }

    commandServer.uiReady();

    int rval = app.exec();
    LOGGER(debug) << "Program stopped with code " << rval << std::endl;
    return rval;
}
