////////////////////////////////////////////////////////////////////////////////
//
// CoinQ_test1.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "CoinQ_peer.h"
#include "CoinQ_filter.h"
#include "CoinQ_blocks.h"
#include "CoinQ_keys.h"

#include <iostream>
#include <string>

using namespace Coin;

bool initBlockTree(CoinQBlockTreeMem& blockTree, const CoinBlockHeader& genesisBlock, const std::string& filename = "")
{
    if (!filename.empty()) {
        try {
            std::cout << "Loading block tree from file..." << std::flush;
            blockTree.loadFromFile(filename);
            std::cout << "Done. "
                      << " mBestHeight: " << blockTree.getBestHeight()
                      << " mTotalWork: " << blockTree.getTotalWork().getDec() << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cout << std::endl;
            std::cout << "Block tree file loading error: " << e.what() << std::endl;
        }
    }

    std::cout << "Constructing new block tree." << std::endl;
    blockTree.clear();
    blockTree.setGenesisBlock(genesisBlock);
    return false;
}

void onDoneSync(CoinQPeer& peer, ICoinQBlockTree& blockTree)
{
    blockTree.subscribeAddBestChain([&](const ChainHeader& header) {
        cout << "Added to best chain:     " << header.getHashLittleEndian().getHex()
             << " Height: " << header.height
             << " ChainWork: " << header.chainWork.getDec() << endl;
    });

    blockTree.subscribeRemoveBestChain([&](const ChainHeader& header) {
        cout << "Removed from best chain: " << header.getHashLittleEndian().getHex()
             << " Height: " << header.height
             << " ChainWork: " << header.chainWork.getDec() << endl;
    });

    int maxHeight = blockTree.getBestHeight();
    for (int i = 249530; i <= maxHeight; i++) {
        const ChainHeader& header = blockTree.getHeader(i);
        peer.askForBlock(header.getHashLittleEndian().getHex());
    }
}

int main(int argc, char** argv)
{
    if (argc < 3 || argc > 5) {
        std::cerr << "Usage: " << argv[0] << " [host] [port] <blocktree file> <address file>" << std::endl;
        return -1;
    }

    std::string blockTreeFile = (argc > 3) ? argv[3] : "";
    std::string addressesFile = (argc > 4) ? argv[4] : "";

    const CoinBlockHeader kGenesisBlock(1, 1231006505, 486604799, 2083236893, g_zero32bytes, uchar_vector("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));
//    const CoinBlockHeader kGenesisBlock(2, 1375162707, 436242792, 3601811532, uchar_vector("000000000000001617794c92f14979ee194f070bb550f4a1beb179c37c6d107d"), uchar_vector("3ea9fc33d82a0ab796930f7bcdbe8aeb656d1f18d1fc35278eb5d705c0074204"));

    CoinQBlockTreeMem blockTree;
    bool bFlushed = initBlockTree(blockTree, kGenesisBlock, blockTreeFile);

    CoinQ::Keys::AddressSet filterAddresses;

    if (argc > 4) {
        std::cout << "Loading filter addresses..." << std::flush;
        try {
            filterAddresses.loadFromFile(argv[4]);
            std::cout << "Done." << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    }

    CoinQTxAddressFilter txFilter(&filterAddresses, CoinQTxFilter::Mode::RECEIVE);
    txFilter.connect([](const ChainTransaction& tx) {
        cout     << "--------------------------------------------------------------------------" << endl
                 << "--New Tx: " << tx.getHashLittleEndian().getHex() << endl;
        if (tx.index > -1) {
            cout << "--  Block:       " << tx.blockHeader.getHashLittleEndian().getHex() << endl
                 << "--  BlockHeight: " << tx.blockHeader.height << endl
                 << "--  MerkleIndex: " << tx.index << endl;
        }
        else {
            cout << "--  Unconfirmed." << endl;
        }
        cout     << "--------------------------------------------------------------------------" << endl
                 << tx.toIndentedString() << endl << endl;
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
                        if (blockTree.insertHeader(item)) {
                            bFlushed = false;
                        }
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
                if (!blockTreeFile.empty() && !bFlushed) {
                    cout << "Flushing to file..." << flush;
                    blockTree.flushToFile(blockTreeFile);
                    bFlushed = true;
                    cout << "Done." << endl;
                }
                onDoneSync(peer, blockTree);
            }
        }
        catch (const std::exception& e) {
            std::cout << std::endl << "block tree exception: " << e.what() << std::endl;
        }
    });

    CoinQBestChainBlockFilter blockFilter(&blockTree);
    blockFilter.connect([&](const ChainBlock& block) {
        for (unsigned int i = 0; i < block.txs.size(); i++) {
            txFilter.push(ChainTransaction(block.txs[i], block.getHeader(), i));
        }
        cout << "Got block: " << block.getHashLittleEndian().getHex() << " height: " << block.height << " chainWork: " << block.chainWork.getDec() << endl;
    });

    peer.subscribeBlock([&](const CoinBlock& block) {
        uchar_vector hash = block.blockHeader.getHashLittleEndian();
        if (!blockTree.hasHeader(hash)) {
            try {
                blockTree.insertHeader(block.blockHeader);
                if (!blockTreeFile.empty()) {
                    cout << "Flushing to file..." << flush;
                    blockTree.flushToFile(blockTreeFile);
                    cout << "Done." << endl;
                }
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
