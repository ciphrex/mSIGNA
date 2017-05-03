///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// commandserver.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "commandserver.h"

#include <QLocalSocket>
#include <QLocalServer>
#include <QDataStream>

#include <QUrl>
#include <QFile>

#include "settings.h"
#include "coinparams.h"

#include "severitylogger.h"

const int COMMAND_SERVER_TIMEOUT = 500;
const QString COMMAND_SERVER_NAME(QString("mSIGNA_") + getDefaultNetwork() + "_CommandServer");
const QString COMMAND_SERVER_URL_PREFIX(getDefaultNetwork() + ":");

CommandServer::CommandServer(QObject* parent)
    : QObject(parent), server(NULL)
{
}

CommandServer::~CommandServer()
{
    stop();
}

bool CommandServer::start()
{
    if (server) stop();

    server = new QLocalServer(this);
    if (!server) return false;

    server->removeServer(COMMAND_SERVER_NAME);

    if (!server->listen(COMMAND_SERVER_NAME)) {
        server->close();
        delete server;
        server = NULL;
        return false;
    }

    connect(server, SIGNAL(newConnection()), this, SLOT(handleConnection()));
    return true;
}

bool CommandServer::stop()
{
    if (!server) return false;

    server->close();
    delete server;
    server = NULL;
    return true;
}

bool CommandServer::processArgs(int argc, char* argv[])
{
    QLocalSocket socket;
    socket.connectToServer(COMMAND_SERVER_NAME, QIODevice::WriteOnly);
    if (!socket.waitForConnected(COMMAND_SERVER_TIMEOUT)) {
         // This is the only instance running
        if (argc > 1) { command = QString(argv[1]); }
        for (int i = 2; i < argc; i++) { args.push_back(QString(argv[i])); }
        return false;
    }

    if (argc > 1) {
        // Send command and args to main instance
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);

        // Send each arg
        for (int i = 1; i < argc; i++) {
            QString arg(argv[i]);
            if (arg.isEmpty()) continue; // Not sure this is necessary, but it can't hurt.
            out << arg;
        }
        socket.write(block);
        if (!socket.waitForBytesWritten(COMMAND_SERVER_TIMEOUT)) {
            LOGGER(error) << "CommandServer::processArgs - " + socket.errorString().toStdString() << std::endl;
        }
    }

    socket.disconnectFromServer();
    return true;
}

void CommandServer::uiReady()
{
    emitCommand(command, args);
/*
    for (auto& arg: args) {
        processArg(arg);
    }
*/
}

void CommandServer::handleConnection()
{
    QLocalSocket* client = server->nextPendingConnection();
    QDataStream in(client);

    bool bReadArgs = false;
    QString command;
    std::vector<QString> args;
    while (client->waitForReadyRead(COMMAND_SERVER_TIMEOUT))
    {
        if (bReadArgs)
        {
            QString arg;
            in >> arg;
            args.push_back(arg);
        }
        else
        {
            in >> command;
            bReadArgs = true;
        }
    }

    emitCommand(command, args);
}

void CommandServer::emitCommand(const QString& command, const std::vector<QString>& args)
{
    if (command.startsWith(COMMAND_SERVER_URL_PREFIX, Qt::CaseInsensitive)) {
        emit gotUrl(QUrl(command));
    }
    else if (QFile::exists(command)) {
        emit gotFile(command);
    }
    else {
        emit gotCommand(command, args);
    }
}
