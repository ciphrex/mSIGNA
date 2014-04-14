///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_vault_sync.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINQ_VAULT_SYNC_H
#define COINQ_VAULT_SYNC_H

namespace CoinQ {

class Vault;
class NetworkSync;

class VaultSync
{
public:
    VaultSync(Vault* vault, NetworkSync* networksync);

    void start(const std::string& host, const std::string& port);
    void start(const std::string& host, int port);
    void stop();
    void resync();

private:
    Vault* vault_;
    NetworkSync* networksync_;    
};

}

#endif //  COINQ_VAULT_SYNC_H
