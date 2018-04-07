///////////////////////////////////////////////////////////////////////////////
//
// BlockchainDownload.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "BlockchainDownload.h"

#include <logger/logger.h>

#include <thread>
#include <chrono>

using namespace CoinQ::Network;
using namespace std;

BlockchainDownload::BlockchainDownload(const CoinQ::CoinParams& coinParams, bool bCheckProofOfWork) :
    m_coinParams(coinParams),
    m_bCheckProofOfWork(bCheckProofOfWork),
    m_bStarted(false),
    m_bIOServiceStarted(false),
    m_work(m_ioService),
    m_bConnected(false),
    m_peer(m_ioService)
{
    // Select hash functions
    Coin::CoinBlockHeader::setHashFunc(m_coinParams.block_header_hash_function());
    Coin::CoinBlockHeader::setPOWHashFunc(m_coinParams.block_header_pow_hash_function());

    // Subscribe peer handlers
    m_peer.subscribeOpen([&](CoinQ::Peer& /*peer*/)
    {
        LOGGER(trace) << "BlockchainDownload - Peer connection opened." << endl;
        m_bConnected = true;
        notifyOpen();
        try
        {
            if (m_blockTree.isEmpty())
            {
  //              m_blockTree.setGenesisBlock(m_coinParams.genesis_block());
                m_peer.getBlocks(m_locatorHashes, m_hashStop);
            }
            else
            {
                m_peer.getBlocks(m_blockTree.getLocatorHashes(-1));
            }
        }
        catch (const std::exception& e)
        {
            LOGGER(error) << "BlockchainDownload - Peer::getBlocks error - " << e.what();
            // TODO: propagate code
            notifyBlockTreeError(e.what(), -1);
        }
    });

    m_peer.subscribeClose([this](CoinQ::Peer& /*peer*/)
    {
        if (m_bConnected) { stop(); }
        notifyClose();
    });

    m_peer.subscribeTimeout([&](CoinQ::Peer& /*peer*/)
    {
        notifyTimeout();
    });

    m_peer.subscribeConnectionError([&](CoinQ::Peer& /*peer*/, const std::string& error, int code)
    {
        notifyConnectionError(error, code);
    });

    m_peer.subscribeProtocolError([&](CoinQ::Peer& /*peer*/, const std::string& error, int code)
    {
        notifyProtocolError(error, code);
    });

    m_peer.subscribeInv([&](CoinQ::Peer& peer, const Coin::Inventory& inv)
    {
        if (!m_bConnected) return;
        LOGGER(trace) << "Received inventory message:" << std::endl << inv.toIndentedString(2) << std::endl;

        using namespace Coin;
        GetDataMessage getData;
        for (auto& item: inv.items)
        {
            switch (item.itemType)
            {
            case MSG_BLOCK:
                getData.items.push_back(InventoryItem(MSG_BLOCK, item.hash));
                break;
            default:
                break;
            } 
        }

        if (!getData.items.empty())
        {
            try
            {
                const unsigned char* hash = getData.items[getData.items.size() - 1].hash;
                m_lastRequestedBlockHash = bytes_t(hash, hash + 32);
                m_peer.send(getData);
            }
            catch (const exception& e)
            {
                LOGGER(error) << "BlockchainDownload - Peer::send() failed: " << e.what() << endl;
                m_lastRequestedBlockHash.clear();
                notifyConnectionError(e.what(), -1);
            }
        }
    });

    m_peer.subscribeBlock([&](CoinQ::Peer& /*peer*/, const Coin::CoinBlock& block)
    {
        m_lastReceivedBlockHash = block.hash();
        LOGGER(trace) << "BlockchainDownload - Received block: " << m_lastReceivedBlockHash.getHex() << endl;

        try
        {
            if (m_blockTree.isEmpty()) { m_blockTree.setGenesisBlock(block.blockHeader); }
            m_blockTree.insertHeader(block.blockHeader);
        }
        catch (const exception& e)
        {
            LOGGER(error) << "BlockchainDownload - Failed to insert header: " << e.what() << endl;
            notifyBlockTreeError(e.what(), -1);
        }

        notifyBlock(block);

        try
        {
            m_peer.getBlocks(m_blockTree.getLocatorHashes(-1));
        }
        catch (const exception& e)
        {
            LOGGER(error) << "BlockchainDownload - Failed to request blocks: " << e.what() << endl;
            notifyConnectionError(e.what(), -1);
        }
    });
}

BlockchainDownload::~BlockchainDownload()
{
    stop();
}

void BlockchainDownload::start(const string& host, const string& port, const vector<uchar_vector>& locatorHashes, const uchar_vector& hashStop)
{
    {
        if (m_bStarted) throw runtime_error("BlockchainDownload - already started.");
        boost::lock_guard<boost::mutex> lock(m_startMutex);
        if (m_bStarted) throw runtime_error("BlockchainDownload - already started.");
    
        LOGGER(trace) << "BlockchainDownload::start(" << host << ", " << port << ")" << std::endl;

        startIOServiceThread();
        m_bStarted = true;

        string port_ = port.empty() ? m_coinParams.default_port() : port;
        m_peer.set(host, port_, m_coinParams.magic_bytes(), m_coinParams.protocol_version(), "Wallet v0.1", 0, false);

        if (locatorHashes.empty())
        {
            m_locatorHashes.clear();
            m_locatorHashes.push_back(uchar_vector(32, 0));
        }
        else
        {
            m_locatorHashes = locatorHashes;
        }

        m_hashStop = hashStop;

        LOGGER(trace) << "Starting peer " << host << ":" << port_ << "..." << endl;
        m_peer.start();
        LOGGER(trace) << "Peer started." << endl;
    }

    notifyStarted();
}

void BlockchainDownload::start(const string& host, int port, const vector<uchar_vector>& locatorHashes, const uchar_vector& hashStop)
{
    std::stringstream ssport;
    ssport << port;
    start(host, ssport.str(), locatorHashes, hashStop);
}

void BlockchainDownload::stop()
{
    {
        if (!m_bStarted) return;
        boost::lock_guard<boost::mutex> lock(m_startMutex);
        if (!m_bStarted) return;

        m_bConnected = false;
        m_peer.stop();
        stopIOServiceThread();

        m_bStarted = false;
    }

    notifyStopped();
}

void BlockchainDownload::startIOServiceThread()
{
    if (m_bIOServiceStarted) throw std::runtime_error("BlockchainDownload - io service already started.");
    boost::lock_guard<boost::mutex> lock(m_ioServiceMutex);
    if (m_bIOServiceStarted) throw std::runtime_error("BlockchainDownload - io service already started.");

    LOGGER(trace) << "Starting IO service thread..." << endl;
    m_bIOServiceStarted = true;
    m_ioServiceThread = boost::thread(boost::bind(&CoinQ::io_service_t::run, &m_ioService));
    LOGGER(trace) << "IO service thread started." << endl;
}

void BlockchainDownload::stopIOServiceThread()
{
    if (!m_bIOServiceStarted) return;
    boost::lock_guard<boost::mutex> lock(m_ioServiceMutex);
    if (!m_bIOServiceStarted) return;

    LOGGER(trace) << "Stopping IO service thread..." << endl;
    m_ioService.stop();
    m_ioServiceThread.join();
    m_ioService.reset();
    m_bIOServiceStarted = false;
    LOGGER(trace) << "IO service thread stopped." << endl; 
}

