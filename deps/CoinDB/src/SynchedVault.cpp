///////////////////////////////////////////////////////////////////////////////
//
// SynchedVault.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#include "SynchedVault.h"

using namespace CoinDB;
using namespace CoinQ;

SynchedVault::SynchedVault() : m_vault(nullptr)
{
}

SynchedVault::~SynchedVault()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_vault) delete m_vault;
}

void SynchedVault::openVault(const std::string& filename, bool bCreate)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_vault) delete m_vault;
    m_vault = new Vault(filename, bCreate);
}

void SynchedVault::closeVault()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_vault)
    {
        delete m_vault;
        m_vault = nullptr;
    }
}
