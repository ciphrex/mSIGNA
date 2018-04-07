///////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// versioninfo.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <QString>

const QString& getVersionText();

const QString& getCommitHash();
const QString& getShortCommitHash();

uint32_t getSchemaVersion();
const QString& getSchemaVersionText();

uint32_t getOpenSSLVersionNumber();
const QString& getOpenSSLVersionText();
