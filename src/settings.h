///////////////////////////////////////////////////////////////////
//
// CoinVault
//
// settings.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_SETTINGS_H
#define COINVAULT_SETTINGS_H

#include <QDir>
#include <QString>

const QString APPNAME("Vault");
const QString APPDATADIR(QDir::homePath() + "/vault");

const unsigned char BASE58_VERSIONS[] = { 0x00, 0x05 };

#endif //  COINVAULT_SETTINGS_H
