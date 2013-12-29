////////////////////////////////////////////////////////////////////////////////
//
// spvtxs.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

//#define SQLITE3CPP_LOG

#include "CoinQ_peer.h"
#include "CoinQ_filter.h"
#include "CoinQ_blocks.h"
#include "CoinQ_txs.h"
#include "CoinQ_keys.h"
#include "CoinQ_sqlite3.h"

#include <iostream>
#include <string>

using namespace Coin;

void onDoneSync(CoinQPeer& peer, ICoinQBlockTree& blockTree, ICoinQTxStore& txStore, int resyncHeight)
{
    blockTree.subscribeAddBestChain([&](const ChainHeader& header) {
        uchar_vector hash = header.getHashLittleEndian();
        cout << "Added to best chain:     " << hash.getHex()
             << " Height: " << header.height
             << " ChainWork: " << header.chainWork.getDec() << endl;
    });

    if (resyncHeight > -1) {
        int maxHeight = blockTree.getBestHeight();
        if (maxHeight >= resyncHeight) {
            cout << "Resynching blocks " << resyncHeight << " - " << maxHeight << endl;
            const ChainHeader& header = blockTree.getHeader(resyncHeight);
            std::string blockHashStr = header.getHashLittleEndian().getHex();
            cout << "Asking for block " << blockHashStr << endl;
            peer.askForBlock(blockHashStr);
        }
    }
}

int main(int argc, char** argv)
{
    if (argc < 6 || argc > 7) {
        std::cerr << "Usage: " << argv[0] << " [host] [port] [blocktree file] [tx file] [address file] <resync height>" << std::endl;
        return -1;
    }

    std::string blockTreeFile = argv[3];
    std::string txFile        = argv[4];
    std::string addressesFile = argv[5];

    const CoinBlockHeader kGenesisBlock(1, 1231006505, 486604799, 2083236893, g_zero32bytes, uchar_vector("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));
//    const CoinBlockHeader kGenesisBlock(2, 1375162707, 436242792, 3601811532, uchar_vector("000000000000001617794c92f14979ee194f070bb550f4a1beb179c37c6d107d"), uchar_vector("3ea9fc33d82a0ab796930f7bcdbe8aeb656d1f18d1fc35278eb5d705c0074204"));

    CoinQBlockTreeSqlite3 blockTree;
    try {
        std::cout << "Loading block tree..." << std::flush;
        blockTree.open(blockTreeFile, kGenesisBlock);
        std::cout << "Done. "
                  << " mBestHeight: " << blockTree.getBestHeight()
                  << " mTotalWork: " << blockTree.getTotalWork().getDec() << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << std::endl;
        std::cerr << "Error loading block tree: " << e.what() << std::endl;
        return 1;
    }

    int resyncHeight          = (argc > 6) ? strtoul(argv[6], NULL, 0) : (blockTree.getBestHeight() - 10);

    CoinQTxStoreSqlite3 txStore;
    if (!txFile.empty()) {
        try {
            std::cout << "Opening transaction file..." << std::flush;
            txStore.open(txFile);
            std::cout << "Done." << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << std::endl;
            std::cout << "Exception opening " << txFile << ": " << e.what() << std::endl;
        }
    }
    txStore.subscribeInsert([](const ChainTransaction& tx) {
        std::cout << "Transaction inserted: " << tx.getHashLittleEndian().getHex() << std::endl;
    });
    txStore.subscribeDelete([](const ChainTransaction& tx) {
        std::cout << "Transaction deleted: " << tx.getHashLittleEndian().getHex() << std::endl;
    });
    txStore.subscribeConfirm([](const ChainTransaction& tx) {
        std::cout << "Transaction confirmed: " << tx.getHashLittleEndian().getHex() << std::endl;
    });
    txStore.subscribeUnconfirm([](const ChainTransaction& tx) {
        std::cout << "Transaction unconfirmed: " << tx.getHashLittleEndian().getHex() << std::endl;
    });

    CoinQ::Keys::AddressSet filterAddresses;
    if (!addressesFile.empty()) {
        std::cout << "Loading filter addresses..." << std::flush;
        try {
            filterAddresses.loadFromFile(addressesFile);
            std::cout << "Done." << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    }

    CoinQTxAddressFilter txFilter(&filterAddresses, CoinQTxFilter::Mode::BOTH);
    txFilter.connect([&](const ChainTransaction& tx) {
        if (txStore.isOpen()) {
            std::cout << "Inserting transaction into store: " << tx.getHashLittleEndian().getHex() << std::endl;
            txStore.insert(tx);
        }
    });


    std::string host = argv[1];
    uint16_t port = strtoul(argv[2], NULL, 0);

    SetAddressVersion(listener_network::ADDRESS_VERSION);
    SetMultiSigAddressVersion(listener_network::MULTISIG_ADDRESS_VERSION);

    CoinQPeer peer(host, port);

    peer.subscribeTx([&](const Transaction& tx) { txFilter.push(tx); });

    peer.subscribeHeaders([&](const HeadersMessage& headers) {
        cout << "Received headers message..." << endl;
        try {
            if (headers.headers.size() > 0) {
                for (auto& item: headers.headers) {
                    try {
                        blockTree.insertHeader(item);
                    }
                    catch (const std::exception& e) {
                        cout << "block tree insertion error for block " << item.getHashLittleEndian().getHex() << ": "  << e.what() << endl;
                        throw e;
                    }
                }

                cout << "Processed " << headers.headers.size() << " headers."
                     << " mBestHeight: " << blockTree.getBestHeight()
                     << " mTotalWork: " << blockTree.getTotalWork().getDec()
                     << " Attempting to fetch more headers..." << endl;
                peer.getHeaders(blockTree.getLocatorHashes(1));
            }
            else {
                cout << "No more headers." << endl;
                onDoneSync(peer, blockTree, txStore, resyncHeight);
            }
        }
        catch (const std::exception& e) {
            std::cout << std::endl << "block tree exception: " << e.what() << std::endl;
        }
    });

    CoinQBestChainBlockFilter blockFilter(&blockTree);
    blockFilter.connect([&](const ChainBlock& block) {
        uchar_vector blockHash = block.blockHeader.getHashLittleEndian();
        cout << "Got block: " << blockHash.getHex() << " height: " << block.height << " chainWork: " << block.chainWork.getDec() << endl;
        for (unsigned int i = 0; i < block.txs.size(); i++) {
            txFilter.push(ChainTransaction(block.txs[i], block.getHeader(), i));
        }
        if ((resyncHeight > -1) && (block.height >= resyncHeight) && (blockTree.getBestHeight() > block.height)) {
            const ChainHeader& nextHeader = blockTree.getHeader(block.height + 1);
            std::string blockHashStr = nextHeader.getHashLittleEndian().getHex();
            cout << "Asking for block " << blockHashStr << endl;
            peer.askForBlock(blockHashStr);
        }
    });

    blockTree.subscribeRemoveBestChain([&](const ChainHeader& header) {
        uchar_vector hash = header.getHashLittleEndian();
        cout << "Removed from best chain: " << hash.getHex()
             << " Height: " << header.height
             << " ChainWork: " << header.chainWork.getDec() << endl;
        if (txStore.isOpen()) {
            txStore.unconfirm(hash);
        }
    });

    peer.subscribeBlock([&](const CoinBlock& block) {
        uchar_vector hash = block.blockHeader.getHashLittleEndian();
        if (!blockTree.hasHeader(hash)) {
            try {
                blockTree.insertHeader(block.blockHeader);
            }
            catch (const std::exception& e) {
                std::cout << std::endl << "block tree exception: " << e.what() << std::endl;
            }
        }
        blockFilter.push(block);
    });

    try
    {
        cout << "Connecting to peer..." << flush;
        peer.start();
        cout << "Done." << endl;
        cout << "Fetching block headers..." << endl;
        peer.getHeaders(blockTree.getLocatorHashes(-1));
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl << endl;
        return -1;
    }

    while (peer.isRunning()) { sleep(1); }
    return 0;

}
