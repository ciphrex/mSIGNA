///////////////////////////////////////////////////////////////////
//
// CoinVault
//
// settings.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_SETTINGS_H
#define COINVAULT_SETTINGS_H

#include <QDir>
#include <QString>

class DefaultSettings
{
public:
    DefaultSettings() : loaded(false) { }

    void load();
    bool isLoaded() const { return loaded; }

    const QString& getAppName() const { return appName; }
    const QString& getDataDir() const { return dataDir; }
    const unsigned char* getBase58Versions() const { return base58Versions; }

private:
    bool loaded;

    QString appName;
    QString dataDir;
    const unsigned char* base58Versions;
};

// Singleton
extern const DefaultSettings& getDefaultSettings();

#endif //  COINVAULT_SETTINGS_H
