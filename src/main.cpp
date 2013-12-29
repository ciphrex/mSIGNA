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

#include "splashscreen.h"
#include "mainwindow.h"
#include "commandserver.h"

#ifdef USE_LOGGING
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#endif

#include <iostream>

#ifdef USE_LOGGING
void init_logging()
{
    namespace logging = boost::log;
    namespace attrs = boost::log::attributes;
    namespace src = boost::log::sources;
    namespace sinks = boost::log::sinks;
    namespace expr = boost::log::expressions;
    namespace keywords = boost::log::keywords;

    boost::shared_ptr< logging::core > core = logging::core::get();

    boost::shared_ptr< sinks::text_file_backend > backend =
        boost::make_shared< sinks::text_file_backend >(
            keywords::file_name = "debug_%5N.log"//,
            //keywords::rotation_size = 5 * 1024 * 1024,
            //keywords::time_based_rotation = sinks::file::rotation_at_time_point(12, 0, 0)
        );

    core->set_filter
    (
        logging::trivial::severity >= logging::trivial::debug
    );

    backend->auto_flush(true);

    // Wrap it into the frontend and register in the core.
    // The backend requires synchronization in the frontend.
    typedef sinks::synchronous_sink< sinks::text_file_backend > sink_t;
    boost::shared_ptr< sink_t > sink(new sink_t(backend));

    core->add_sink(sink);
}
#endif

int main(int argc, char* argv[])
{
    Q_INIT_RESOURCE(coinvault);

    QApplication app(argc, argv);
    app.setOrganizationName("Ciphrex");
    app.setApplicationName(APPNAME);

    CommandServer commandServer(&app);
    if (commandServer.processArgs(argc, argv)) exit(0);

#ifdef USE_LOGGING
    init_logging();
    BOOST_LOG_TRIVIAL(debug) << "Vault started.";
#endif

    SplashScreen splash;

    splash.show();
    splash.setAutoFillBackground(true);
    app.processEvents();

    splash.showMessage("\n  Loading settings...");
    app.processEvents();
    MainWindow mainWin; // constructor loads settings
    QObject::connect(&mainWin, &MainWindow::status, [&](const QString& message) { splash.showMessage(message); });

    splash.showMessage("\n  Starting command server...");
    app.processEvents();
    if (!commandServer.start()) {
#ifdef USE_LOGGING
        BOOST_LOG_TRIVIAL(debug) << "Could not start command server.";
#endif
    }
    else {
        app.connect(&commandServer, SIGNAL(gotUrl(const QUrl&)), &mainWin, SLOT(processUrl(const QUrl&)));
        app.connect(&commandServer, SIGNAL(gotFile(const QString&)), &mainWin, SLOT(processFile(const QString&)));
    }

    splash.showMessage("\n  Loading block headers...");
    app.processEvents();
    mainWin.loadBlockTree();

    app.processEvents();
    mainWin.tryConnect();

    mainWin.show();
    splash.finish(&mainWin);
    commandServer.uiReady();

    return app.exec();
}
