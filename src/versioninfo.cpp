///////////////////////////////////////////////////////////////////
//
// CoinVault
//
// versioninfo.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "versioninfo.h"
#include "../BuildInfo.h"

// Definitions
const int VERSIONPADDINGRIGHT = 20;
const int VERSIONPADDINGBOTTOM = 30;
 
const QString VERSIONTEXT("Version 0.4.10 beta");

const QString commitHash(COMMIT_HASH);
const QString shortCommitHash(QString(COMMIT_HASH).left(6));

// Accessors
int getVersionPaddingRight() { return VERSIONPADDINGRIGHT; }
int getVersionPaddingBottom() { return VERSIONPADDINGBOTTOM; }

const QString& getVersionText() { return VERSIONTEXT; }

const QString& getCommitHash() { return commitHash; }
const QString& getShortCommitHash() { return shortCommitHash; }
