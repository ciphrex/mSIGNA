///////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// settings.h
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <QDir>
#include <QString>

class DefaultSettings
{
public:
    DefaultSettings() : m_loaded(false) { }

    void load();
    bool isLoaded() const { return m_loaded; }

    const QString& getAppName() const { return m_appName; }
    const QString& getSettingsRoot() const { return m_settingsRoot; }
    const QString& getNetworkSettingsPath() const { return m_networkSettingsPath; }
    const QString& getDataDir() const { return m_dataDir; }
    const QString& getDocumentDir() const { return m_documentDir; }
    const unsigned char* getBase58Versions() const { return m_base58Versions; }

private:
    bool m_loaded;

    QString m_appName;
    QString m_settingsRoot;         // persisted settings root
    QString m_networkSettingsPath;  // persisted network-specific settings
    QString m_dataDir;
    QString m_documentDir;
    unsigned char m_base58Versions[2];
};

// Singleton
extern const DefaultSettings& getDefaultSettings();

