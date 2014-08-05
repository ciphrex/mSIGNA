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

#include <CoinDB/Schema.h>

// Definitions
const int VERSIONPADDINGRIGHT = 20;
const int VERSIONPADDINGBOTTOM = 30;
 
const QString VERSIONTEXT("Version 0.6.0 beta");

const QString commitHash(COMMIT_HASH);
const QString shortCommitHash(QString(COMMIT_HASH).left(6));

const uint32_t schemaVersion = SCHEMA_VERSION;

// Accessors
int getVersionPaddingRight() { return VERSIONPADDINGRIGHT; }
int getVersionPaddingBottom() { return VERSIONPADDINGBOTTOM; }

const QString& getVersionText() { return VERSIONTEXT; }

const QString& getCommitHash() { return commitHash; }
const QString& getShortCommitHash() { return shortCommitHash; }

uint32_t getSchemaVersion() { return schemaVersion; }
