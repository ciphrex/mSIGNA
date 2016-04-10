///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// networksettingsdialog.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

class QLineEdit;
class QCheckBox;

#include <QDialog>

class NetworkSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    NetworkSettingsDialog(const QString& host, int port, bool autoConnect, QWidget* parent = NULL);

    QString getHost() const;
    int getPort() const;
    bool getAutoConnect() const;

private:
    QLineEdit* hostEdit;
    QLineEdit* portEdit;
    QCheckBox* autoConnectCheckBox;
};

