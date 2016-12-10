///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// splashscreen.h
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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
