///////////////////////////////////////////////////////////////////
//
// CoinVault
//
// settings.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#include "settings.h"

static const unsigned char BASE58_VERSIONS[] = { 0x00, 0x05 };

void DefaultSettings::load()
{
    appName = "Vault";
    dataDir = QDir::homePath() + "/vault";
    base58Versions = BASE58_VERSIONS;
    loaded = true;
}

static DefaultSettings defaultSettings;

const DefaultSettings& getDefaultSettings()
{
    if (!defaultSettings.isLoaded()) defaultSettings.load();
    return defaultSettings;
}
