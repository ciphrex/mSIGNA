///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_spv_client.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_SPV_CLIENT_H
#define COINVAULT_SPV_CLIENT_H

/*
#ifdef __WIN32__
#include <winsock2.h>
#endif
*/

#include <CoinQ_peer_io.h>

#include <map>

namespace CoinQ {

class SpvClient
{
public:
    SpvClient() { }
    ~SpvClient() { }

    void createPeer(
        const std::string& host,
        const std::string& port,
        uint32_t magic_bytes,
        uint32_t protocol_version,
        const std::string& user_agent = std::string(),
        uint32_t start_height = 0
    );

    void deletePeer(const std::string& peername);

    void start();
    void stop();
    bool isRunning() const;

private:
    io_service_t io_service_;

    typedef std::map<std::string, std::vector<std::shared_ptr<Peer>>> peermap_t;
    peermap_t peermap_;
};

}

#endif // COINVAULT_SPV_CLIENT_H
