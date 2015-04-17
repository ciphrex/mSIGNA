///////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// versioninfo.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "versioninfo.h"
#include "../BuildInfo.h"

#include <CoinDB/Schema.h>

#include <openssl/opensslv.h>

// Definitions
const QString VERSIONTEXT("0.9.1 beta");

const QString commitHash(COMMIT_HASH);
const QString shortCommitHash(QString(COMMIT_HASH).left(7));

const uint32_t schemaVersion(SCHEMA_VERSION);
const QString schemaVersionText(QString::number(SCHEMA_VERSION));

const uint32_t openSSLVersionNumber(OPENSSL_VERSION_NUMBER);
const QString openSSLVersionText(OPENSSL_VERSION_TEXT);

// Accessors
const QString& getVersionText() { return VERSIONTEXT; }

const QString& getCommitHash() { return commitHash; }
const QString& getShortCommitHash() { return shortCommitHash; }

uint32_t getSchemaVersion() { return schemaVersion; }
const QString& getSchemaVersionText() { return schemaVersionText; } 

uint32_t getOpenSSLVersionNumber() { return openSSLVersionNumber; }
const QString& getOpenSSLVersionText() { return openSSLVersionText; }

