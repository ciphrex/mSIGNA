///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// splashscreen.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef VAULT_SPLASHSCREEN_H
#define VAULT_SPLASHSCREEN_H

#include <QSplashScreen>

class SplashScreen : public QSplashScreen
{
    Q_OBJECT

public:
    explicit SplashScreen();

public slots:
    void showProgressMessage(const QString& message);
};

#endif // VAULT_SPLASHSCREEN_H
