///////////////////////////////////////////////////////////////////////////////
//
// peer example program 
//
// main.cpp
//
// Copyright (c) 2012-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include <CoinCore/CoinNodeData.h>
#include <CoinCore/MerkleTree.h>
#include <CoinCore/typedefs.h>

#include <CoinQ/CoinQ_peer_io.h>
#include <CoinQ/CoinQ_coinparams.h>

#include <stdutils/stringutils.h>

#include <logger/logger.h>

#include <signal.h>

bool g_bShutdown = false;

void finish(int sig)
{
    std::cout << "Stopping..." << std::endl;
    g_bShutdown = true;
}

using namespace CoinQ;
using namespace std;

void onOpen(Peer& peer)
{
    cout << "Peer " << peer.name() << " open." << endl;
}

void onClose(Peer& peer)
{
    cout << "Peer " << peer.name() << " closed." << endl;

    g_bShutdown = true;
}

void onProtocolError(Peer& peer, const std::string& message, int /*codd*/)
{
    cout << "Peer " << peer.name() << " protocol error - " << message << "." << endl;
}

void onConnectionError(Peer& peer, const std::string& message, int /*code*/)
{
    cout << "Peer " << peer.name() << " connection error - " << message << "." << endl;
}

void onInv(Peer& peer, const Coin::Inventory& inv)
{
    cout << endl << "Received inventory from " << peer.name() << endl << inv.toIndentedString() << endl;

    Coin::GetDataMessage getData;

    for (auto& item: inv.getItems()) {
        if (item.getType() == MSG_BLOCK || item.getType() == MSG_FILTERED_BLOCK) {
            getData.addItem(MSG_BLOCK, item.getItemHash());
        }
        else if (item.getType() == MSG_TX) {
            getData.addItem(MSG_TX, item.getItemHash());
        }
    }

    if (getData.getCount() > 0) peer.send(getData);
}

void onHeaders(Peer& peer, const Coin::HeadersMessage& headers)
{
    cout << endl << "Received headers message from peer " << peer.name() << endl << headers.toIndentedString() << endl;
}

void onTx(Peer& peer, const Coin::Transaction& tx)
{
    cout << endl << "Received transaction " << tx.getHashLittleEndian().getHex() << " from peer " << peer.name() << endl;// << tx.toIndentedString() << endl;
}

void onBlock(Peer& peer, const Coin::CoinBlock& block)
{
    cout << endl << "Received block from peer " << peer.name() << endl << block.toRedactedIndentedString() << endl; 
}

void onMerkleBlock(Peer& peer, const Coin::MerkleBlock& merkleBlock)
{
    cout << endl << "Received merkle block from peer " << peer.name() << endl << merkleBlock.toIndentedString() << endl;

    Coin::MerkleTree tree;
    for (auto& hash: merkleBlock.hashes) { tree.addHash(hash); }
    cout << "Merkle root: " << tree.getRootLittleEndian().getHex() << endl;
}

int main(int argc, char* argv[])
{
    try
    {
        NetworkSelector networkSelector;

        if (argc < 3)
        {
            cerr << "# Usage: " << argv[0] << " <network> <host> [port]" << endl
                 << "# Supported networks: " << stdutils::delimited_list(networkSelector.getNetworkNames(), ", ") << endl;
            return -1;
        }

        CoinParams coinParams = networkSelector.getCoinParams(argv[1]);
        string host = argv[2];
        string port = (argc > 3) ? argv[3] : coinParams.default_port();

        CoinQ::io_service_t io_service;
        CoinQ::io_service_t::work work(io_service);
        boost::thread thread(boost::bind(&CoinQ::io_service_t::run, &io_service));


        cout << endl << "Connecting to " << coinParams.network_name() << " peer" << endl
             << "-------------------------------------------" << endl
             << "  host:             " << host << endl
             << "  port:             " << port << endl
             << "  magic bytes:      " << hex << coinParams.magic_bytes() << endl
             << "  protocol version: " << dec << coinParams.protocol_version() << endl
             << endl;

        Peer peer(io_service);
        peer.set(host, port, coinParams.magic_bytes(), coinParams.protocol_version(), "peer v1.0", 0, true);

        peer.subscribeStart([&](Peer& peer) { cout << "Peer " << peer.name() << " started." << endl; });
        peer.subscribeStop([&](Peer& peer) { cout << "Peer " << peer.name() << " stopped." << endl; });
        peer.subscribeOpen(&onOpen);
        peer.subscribeClose(&onClose);
        peer.subscribeConnectionError(&onConnectionError);

        peer.subscribeTimeout([&](Peer& peer) { cout << "Peer " << peer.name() << " timed out." << endl; });

        peer.subscribeInv(&onInv);
        peer.subscribeHeaders(&onHeaders);
        peer.subscribeTx(&onTx);
        peer.subscribeBlock(&onBlock);
        peer.subscribeMerkleBlock(&onMerkleBlock);
        peer.subscribeProtocolError(&onProtocolError);

        INIT_LOGGER("peer.log");
    
        signal(SIGINT, &finish);
        signal(SIGTERM, &finish);

        peer.start();
        while (!g_bShutdown) { usleep(200); }
        peer.stop();
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -2;
    }

    return 0;
}
