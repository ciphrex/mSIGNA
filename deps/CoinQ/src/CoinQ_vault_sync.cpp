///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_vault_sync.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "CoinQ_vault_sync.h"

#include "CoinQ_vault.h"
#include "CoinQ_netsync.h"

using namespace CoinQ;

VaultSync::VaultSync(Vault* vault, NetworkSync* networksync)
    : vault_(vault), networksync_(networksync)
{
    networksync->subscribeTx([&](const Coin::Transaction& tx) {
        std::shared_ptr<Tx> vault_tx(new Tx());
        try {
            vault_tx->set(tx);
            vault_->addTx(tx);
        }
        catch (const std::exception& e) {
            // TODO
        }
    });

    networksync->subscribeMerkleBlock([&](const ChainMerkleBlock& block) {
        try {
            vault_->insertMerkleBlock(block);
        }
        catch (const std::exception& e) {
            // TODO
        }
    });
}

void VaultSync::start(const std::string& host, const std::string& port)
{
    networksync_->start(host, port);
}

void VaultSync::start(const std::string& host, int port)
{
    networksync_->start(host, port);
}

void VaultSync::stop();
{
    networksync_->stop();
}

void VaultSync::resync();
{
    networksync_->resync(vault_->getLocatorHashes(), vault_->getFirstAccountTimeCreated());
}


