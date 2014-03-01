///////////////////////////////////////////////////////////////////
//
// CoinVault
//
// versioninfo.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef VAULT_VERSIONINFO_H
#define VAULT_VERSIONINFO_H

#include <QString>

const int VERSIONPADDINGRIGHT = 20;
const int VERSIONPADDINGBOTTOM = 30;

const QString VERSIONTEXT("Version 0.0.30 alpha");

#ifndef COMMIT_HASH
#define COMMIT_HASH "N/A"
#endif

const QString SHORT_COMMIT_HASH(QString(COMMIT_HASH).left(6));

#endif //  VAULT_VERSIONINFO_H
