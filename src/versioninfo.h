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

#include "../BuildInfo.h"

#include <QString>

const int VERSIONPADDINGRIGHT = 20;
const int VERSIONPADDINGBOTTOM = 30;

const QString VERSIONTEXT("Version 0.0.31 alpha");

const QString SHORT_COMMIT_HASH(QString(COMMIT_HASH).left(6));

#endif //  VAULT_VERSIONINFO_H
