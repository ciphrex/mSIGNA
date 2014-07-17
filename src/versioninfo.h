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

