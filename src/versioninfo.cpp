///////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// versioninfo.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "versioninfo.h"
#include "../BuildInfo.h"

#include <CoinDB/Schema.h>

#include <openssl/opensslv.h>

// Definitions
const QString VERSIONTEXT("0.11.1");

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

