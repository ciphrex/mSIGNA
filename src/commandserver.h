///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// commandserver.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <QObject>
#include <vector>

class QLocalServer;

class CommandServer : public QObject
{
    Q_OBJECT

public:
    CommandServer(QObject* parent);
    ~CommandServer();

    bool start();
    bool stop();

    bool processArgs(int argc, char* argv[]);
    void uiReady();

signals:
    void gotUrl(const QUrl& url);
    void gotFile(const QString& fileName);
    void gotCommand(const QString& command);

private slots:
    void handleConnection();

private:
    void processArg(const QString& arg);

    QLocalServer* server;

    std::vector<QString> args;
};

