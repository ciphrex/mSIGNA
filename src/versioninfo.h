///////////////////////////////////////////////////////////////////
//
// CoinVault
//
// versioninfo.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <QString>

int getVersionPaddingRight();
int getVersionPaddingBottom();

const QString& getVersionText();

const QString& getCommitHash();
const QString& getShortCommitHash();

uint32_t getSchemaVersion();
const QString& getSchemaVersionText();

uint32_t getOpenSSLVersionNumber();
const QString& getOpenSSLVersionText();
