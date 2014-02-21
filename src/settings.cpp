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
#include "filesystem.h"

static const unsigned char BASE58_VERSIONS[] = { 0x00, 0x05 };

void DefaultSettings::load()
{
    appName = "Vault";
    dataDir = QString::fromStdString(getDefaultDataDir());
    documentDir = QDir::homePath() + "/Vaults";
    base58Versions = BASE58_VERSIONS;
    loaded = true;
}

static DefaultSettings defaultSettings;

const DefaultSettings& getDefaultSettings()
{
    if (!defaultSettings.isLoaded()) defaultSettings.load();
    return defaultSettings;
}
