///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// commandserver.h
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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
    void gotCommand(const QString& command, const std::vector<QString>& args);

private slots:
    void handleConnection();

private:
    void emitCommand(const QString& command, const std::vector<QString>& args);

    QLocalServer* server;

    QString command;
    std::vector<QString> args;
};

