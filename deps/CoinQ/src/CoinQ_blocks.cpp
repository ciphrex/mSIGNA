///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_blocks.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "CoinQ_blocks.h"

#include <logger/logger.h>

using namespace CoinQ;

bool CoinQBlockTreeMem::setBestChain(ChainHeader& header)
{
    if (header.inBestChain) return false;

    // Retrace back to earliest best block
    std::stack<ChainHeader*> newBestChain;
    ChainHeader* pParent = &header;
    while (!pParent->inBestChain)
    {
        newBestChain.push(pParent);
        pParent = &mHeaderHashMap.at(pParent->prevBlockHash());
    }

    for (auto it = pParent->childHashes.begin(); it != pParent->childHashes.end(); ++it)
    {
        ChainHeader& child = mHeaderHashMap.at(*it);
        if (child.inBestChain)
        {
            unsetBestChain(child);
            break;
        }
    }

    // First set these so we have the most current info.
    mBestHeight = header.height;
    mTotalWork = header.chainWork;

    // Pop back up stack and make this the best chain
    int count = 0;
    while (!newBestChain.empty())
    {
        ChainHeader* pChild = newBestChain.top();
        pChild->inBestChain = true;
        mHeaderHeightMap[pChild->height] = pChild;
        if (count == 0) notifyReorg(*pChild);
        notifyAddBestChain(*pChild);
        newBestChain.pop();
        count++;
    }

    pHead = &header;

    return true;
}

bool CoinQBlockTreeMem::unsetBestChain(ChainHeader& header)
{
    if (!header.inBestChain) return false;

    if (header.height == 0) throw std::runtime_error("Cannot remove genesis block from best chain.");

    ChainHeader* pParent = &mHeaderHashMap.at(header.prevBlockHash());
    if (pParent->inBestChain)
    {
        mBestHeight = pParent->height;
        mTotalWork = pParent->chainWork;
    }
    header.inBestChain = false;
    notifyRemoveBestChain(header);

    pParent = &header;
    while (pParent->childHashes.size() != 0)
    {
        for (auto it = pParent->childHashes.begin(); it != pParent->childHashes.end(); ++it)
        {
            ChainHeader* pChild = &mHeaderHashMap.at(*it);
            if (pChild->inBestChain)
            {
                pParent = pChild;
                mHeaderHeightMap.erase(pChild->height);
                pChild->inBestChain = false;
                notifyRemoveBestChain(*pChild);
                break;
            }
        }
    }
    return true;
}

void CoinQBlockTreeMem::setGenesisBlock(const Coin::CoinBlockHeader& header)
{
    LOGGER(trace) << "setGenesisBlock - hash: " << header.getPOWHashLittleEndian().getHex() << std::endl;
    if (mHeaderHashMap.size() != 0) throw std::runtime_error("Tree is not empty.");

    bFlushed = false;
    uchar_vector hash = header.hash();
    ChainHeader& genesisHeader = mHeaderHashMap[hash] = header;
    mHeaderHeightMap[0] = &genesisHeader;
    genesisHeader.height = 0;
    genesisHeader.inBestChain = true;
    genesisHeader.chainWork = genesisHeader.getWork();
    mBestHeight = 0;
    mTotalWork = genesisHeader.chainWork;
    pHead = &genesisHeader;
    notifyInsert(header);
    notifyAddBestChain(header);
}

bool CoinQBlockTreeMem::insertHeader(const Coin::CoinBlockHeader& header, bool bCheckProofOfWork, bool bReplaceTip)
{
    if (mHeaderHashMap.size() == 0) throw std::runtime_error("No genesis block.");

    uchar_vector headerHash = header.hash();
    if (hasHeader(headerHash)) return false;

    header_hash_map_t::iterator it = mHeaderHashMap.find(header.prevBlockHash());
    if (it == mHeaderHashMap.end()) throw std::runtime_error("Parent not found.");

    ChainHeader& parent = it->second;

    // TODO: Check version, compute work required.

    // Check timestamp
/*    if (bCheckTimestamp && header.timestamp > time(NULL) + 2 * 60 * 60) {
        throw std::runtime_error("Timestamp too far in the future.");
    }*/

    // Check proof of work
    if (bCheckProofOfWork && BigInt(header.getPOWHashLittleEndian()) > header.getTarget()) throw std::runtime_error("Header hash is too big.");

    ChainHeader& chainHeader = mHeaderHashMap[headerHash] = header;
    chainHeader.height = parent.height + 1;
    chainHeader.chainWork = parent.chainWork + chainHeader.getWork();
    parent.childHashes.insert(headerHash);
    notifyInsert(chainHeader);

    if ((bReplaceTip && chainHeader.chainWork >= mTotalWork) || chainHeader.chainWork > mTotalWork)
    {
        setBestChain(chainHeader);
    }

    bFlushed = false;
    return true;
}

bool CoinQBlockTreeMem::deleteHeader(const uchar_vector& hash)
{
    header_hash_map_t::iterator it = mHeaderHashMap.find(hash);
    if (it == mHeaderHashMap.end()) return false;

    ChainHeader& header = it->second;
    unsetBestChain(header);
    header_hash_map_t::iterator itParent = mHeaderHashMap.find(header.hash());
    if (itParent == mHeaderHashMap.end()) throw std::runtime_error("Critical error: parent for block not found.");

    // Recurse through children
    for (auto itChild = header.childHashes.begin(); itChild != header.childHashes.end(); ++itChild) { deleteHeader(*itChild); }

    // TODO: Find new best chain if this header was in best chain.

    // Remove header
    ChainHeader& parent = itParent->second;
    unsigned int nErased = parent.childHashes.erase(hash);
    assert(nErased == 1);
    notifyDelete(header);
    mHeaderHashMap.erase(hash);
    bFlushed = false;
    return true;
}

bool CoinQBlockTreeMem::hasHeader(const uchar_vector& hash) const
{
    return (mHeaderHashMap.find(hash) != mHeaderHashMap.end());
}

const ChainHeader& CoinQBlockTreeMem::getHeader(const uchar_vector& hash) const
{
    header_hash_map_t::const_iterator it = mHeaderHashMap.find(hash);
    if (it == mHeaderHashMap.end()) throw std::runtime_error("Not found.");

    return it->second;
}

const ChainHeader& CoinQBlockTreeMem::getHeader(int height) const
{
    if (mHeaderHeightMap.size() > 0)
    {
        if (height < 0) height += mBestHeight + 1;
        if (height >= 0)
        {
            header_height_map_t::const_iterator it = mHeaderHeightMap.find((unsigned int)height);
            if (it != mHeaderHeightMap.end()) return *it->second;
        }
    }

    throw std::runtime_error("Not found.");
}

const ChainHeader& CoinQBlockTreeMem::getTip() const
{
    if (!pHead) throw std::runtime_error("Tree is empty.");

    return *pHead;
}

int CoinQBlockTreeMem::getTipHeight() const
{
    if (!pHead) throw std::runtime_error("Tree is empty.");

    return pHead->height;
}

const ChainHeader& CoinQBlockTreeMem::getHeaderBefore(uint32_t timestamp) const
{
    if (mBestHeight == -1) throw std::runtime_error("Tree is empty.");

    int i;
    for (i = 1; i <= mBestHeight; i++)
    {
        ChainHeader* header = mHeaderHeightMap.at(i);
        if (header->timestamp() > timestamp) break; 
    }

    return *mHeaderHeightMap.at(i - 1);
}

std::vector<uchar_vector> CoinQBlockTreeMem::getLocatorHashes(int maxSize = -1) const
{
    std::vector<uchar_vector> locatorHashes;

    if (mBestHeight == -1)
    {
        locatorHashes.push_back(g_zero32bytes);
        return locatorHashes;
    }

    if (maxSize < 0) maxSize = mBestHeight + 1;

    int i = mBestHeight;
    int n = 0;
    int step = 1;
    while ((i >= 0) && (n < maxSize))
    {
        locatorHashes.push_back(mHeaderHeightMap.at(i)->hash());
        i -= step;
        n++;
        if (n > 10) step *= 2;
    }
    return locatorHashes;
}

int CoinQBlockTreeMem::getConfirmations(const uchar_vector& hash) const
{
    header_hash_map_t::const_iterator it = mHeaderHashMap.find(hash);
    if (it == mHeaderHashMap.end() || !it->second.inBestChain) return 0;

    return mBestHeight - it->second.height + 1;
}

void CoinQBlockTreeMem::loadFromFile(const std::string& filename, bool bCheckProofOfWork, CoinQBlockTreeMem::callback_t callback)
{
    boost::filesystem::path p(filename);
    if (!boost::filesystem::exists(p)) throw BlockTreeFileNotFoundException();

    if (!boost::filesystem::is_regular_file(p)) throw BlockTreeInvalidFileTypeException();

    const unsigned int RECORD_SIZE = MIN_COIN_BLOCK_HEADER_SIZE + 4;
    if (boost::filesystem::file_size(p) % RECORD_SIZE != 0) throw BlockTreeInvalidFileLengthException();

#ifndef _WIN32
    std::ifstream fs(p.native(), std::ios::binary);
#else
    std::ifstream fs(filename, std::ios::binary);
#endif
    if (!fs.good()) throw BlockTreeFailedToOpenFileForReadException();

    clear();
    uchar_vector headerBytes;
    uchar_vector hash;
    Coin::CoinBlockHeader header;

    unsigned int count = 0;

    char buf[RECORD_SIZE * 64];
    while (fs)
    {
        fs.read(buf, RECORD_SIZE * 64);
        if (fs.bad()) throw BlockTreeFileReadFailureException();

        unsigned int nbytesread = fs.gcount();
        unsigned int pos = 0;
        for (; pos <= nbytesread - RECORD_SIZE; pos += RECORD_SIZE)
        {
            headerBytes.assign((unsigned char*)&buf[pos], (unsigned char*)&buf[pos + MIN_COIN_BLOCK_HEADER_SIZE]);
            header.setSerialized(headerBytes);
            hash = header.hash();
            if (memcmp(&buf[pos + MIN_COIN_BLOCK_HEADER_SIZE], &hash[0], 4)) throw BlockTreeChecksumErrorException();

            try
            {
                if (mBestHeight >= 0)
                {
                    insertHeader(header, bCheckProofOfWork);
                    if (count % 10000 == 0)
                    {
                        if (callback && !callback(*this)) throw BlockTreeLoadInterruptedException();
                        LOGGER(debug) << "CoinQBlockTreeMem::loadFromFile() - header hash: " << header.hash().getHex() << " height: " << count << std::endl;
                    }
                    count++;
                }
                else
                { 
                    setGenesisBlock(header);
                    if (callback && !callback(*this)) throw BlockTreeLoadInterruptedException();
                    LOGGER(debug) << "CoinQBlockTreeMem::loadFromFile() - genesis hash: " << header.hash().getHex() << std::endl;
                    count++;
                }
            }
            catch (const BlockTreeException& e)
            {
                throw e;
            }
            catch (const std::exception& e)
            {
                throw std::runtime_error(std::string("Block ") + hash.getHex() + ": " + e.what());
            }
        }

        if (pos != nbytesread) throw BlockTreeUnexpectedEndOfFileException();
    }

    if (callback) callback(*this); // No need to interrupt since we're done.
}

void CoinQBlockTreeMem::flushToFile(const std::string& filename)
{
    if (mBestHeight == -1) throw std::runtime_error("Tree is empty.");

    boost::filesystem::path swapfile(filename + ".swp");
    //if (boost::filesystem::exists(swapfile)) throw BlockTreeSwapfileAlreadyExistsException();

    {
#ifndef _WIN32
        std::ofstream fs(swapfile.native(), std::ios::binary | std::ios::trunc);
#else
        std::ofstream fs(filename + ".swp", std::ios::binary | std::ios::trunc);
#endif

        uchar_vector headerBytes, hash;

        for (int i = 0; i <= mBestHeight; i++)
        {
            ChainHeader* pHeader = mHeaderHeightMap.at(i);

            headerBytes = pHeader->getSerialized();
            hash = pHeader->hash();

            fs.write((const char*)&headerBytes[0], MIN_COIN_BLOCK_HEADER_SIZE);
            if (fs.bad()) throw BlockTreeFileWriteFailureException();

            fs.write((const char*)&hash[0], 4);
            if (fs.bad()) throw BlockTreeFileWriteFailureException();
        }
    }

    boost::system::error_code ec;
    boost::filesystem::path p(filename);
    boost::filesystem::rename(swapfile, p, ec);
    if (!!ec) throw std::runtime_error(ec.message());

    bFlushed = true;
}

