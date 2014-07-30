///////////////////////////////////////////////////////////////////////////////
//
// peer example program 
//
// main.cpp
//
// Copyright (c) 2012-2014 Eric Lombrozo
//
// All Rights Reserved.

#include <CoinCore/CoinNodeData.h>
#include <CoinCore/MerkleTree.h>
#include <CoinCore/typedefs.h>

#include <CoinQ/CoinQ_peer_io.h>
#include <CoinQ/CoinQ_coinparams.h>

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

void onClose(Peer& peer, int code, const std::string& message)
{
    cout << "Peer " << peer.name() << " closed with code " << code << ": " << message << "." << endl;

    g_bShutdown = true;
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
    string host("localhost");
    if (argc > 1) { host = argv[1]; }

    signal(SIGINT, &finish);
    signal(SIGTERM, &finish);

    CoinParams coinParams = getBitcoinParams();

    CoinQ::io_service_t io_service;
    CoinQ::io_service_t::work work(io_service);
    boost::thread thread(boost::bind(&CoinQ::io_service_t::run, &io_service));


    cout << "Connecting to peer" << endl
         << "-------------------------------------------" << endl
         << "  host:             " << host << endl
         << "  port:             " << coinParams.default_port() << endl
         << "  magic bytes:      " << hex << coinParams.magic_bytes() << endl
         << "  protocol version: " << dec << coinParams.protocol_version() << endl
         << endl;

    Peer peer(io_service);
    peer.set(host, coinParams.default_port(), coinParams.magic_bytes(), coinParams.protocol_version(), "peer v1.0", 0, true);

    peer.subscribeStart([&](Peer& peer) { cout << "Peer " << peer.name() << " started." << endl; });
    peer.subscribeStop([&](Peer& peer) { cout << "Peer " << peer.name() << " stopped." << endl; });
    peer.subscribeOpen(&onOpen);
    peer.subscribeClose(&onClose);

    peer.subscribeTimeout([&](Peer& peer) { cout << "Peer " << peer.name() << " timed out." << endl; });

    peer.subscribeInv(&onInv);
    peer.subscribeHeaders(&onHeaders);
    peer.subscribeTx(&onTx);
    peer.subscribeBlock(&onBlock);
    peer.subscribeMerkleBlock(&onMerkleBlock);

    peer.start();
    while (!g_bShutdown) { usleep(200); }
    peer.stop();

    return 0;
}
