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
#include "coinparams.h"
#include "filesystem.h"

//static const unsigned char BASE58_VERSIONS[] = { 0x00, 0x05 };

void DefaultSettings::load()
{
    m_appName = getCoinParams().network_name();
    m_appName += "Vault";
    m_settingsRoot = "CoinVault";
    m_networkSettingsPath = m_settingsRoot + "/" + getCoinParams().network_name();
    m_dataDir = QString::fromStdString(getDefaultDataDir(m_appName.toStdString()));
    m_documentDir = QDir::homePath();
    m_base58Versions[0] = getCoinParams().pay_to_pubkey_hash_version();
    m_base58Versions[1] = getCoinParams().pay_to_script_hash_version();
    m_loaded = true;
}

static DefaultSettings defaultSettings;

const DefaultSettings& getDefaultSettings()
{
    if (!defaultSettings.isLoaded()) defaultSettings.load();
    return defaultSettings;
}
