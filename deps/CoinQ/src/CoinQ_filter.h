////////////////////////////////////////////////////////////////////////////////
//
// CoinQ_filter.h
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#ifndef _COINQ_FILTER_H_
#define _COINQ_FILTER_H_

#include "CoinQ_signals.h"
#include "CoinQ_slots.h"
#include "CoinQ_blocks.h"
#include "CoinQ_txs.h"
#include "CoinQ_keys.h"

#include <CoinCore/CoinNodeData.h>

#include <string>
#include <set>
#include <algorithm>

#include <boost/filesystem.hpp>

template <typename PushType, typename... SlotValues>
class ICoinQFilter
{
public:
    virtual bool push(PushType) const = 0;
    virtual void connect(std::function<void(SlotValues...)>) = 0;
    virtual void clear() = 0;
};


class CoinQTxFilter : public ICoinQFilter<const ChainTransaction&, const ChainTransaction&>
{
public:
    enum Mode {
        RECEIVE = 1,
        SEND = 2,
        BOTH = 3
    };

protected:
    Mode mode;

public:
    virtual void setMode(Mode mode) { this->mode = mode; }
    virtual Mode getMode() { return mode; }
};

class CoinQBestChainBlockFilter : public ICoinQFilter<const Coin::CoinBlock&, const ChainBlock&>
{
private:
    ICoinQBlockTree* pBlockTree;
    mutable CoinQSignal<const ChainBlock&> notify;

public:
    CoinQBestChainBlockFilter() : pBlockTree(NULL) { }
    CoinQBestChainBlockFilter(ICoinQBlockTree* pBlockTree) { setBlockTree(pBlockTree); }

    void setBlockTree(ICoinQBlockTree* pBlockTree) { this->pBlockTree = pBlockTree; }

    bool push(const Coin::CoinBlock& block) const;

    typedef std::function<void(const ChainBlock&)> chain_block_slot_t;
    void connect(chain_block_slot_t slot) { notify.connect(slot); }

    void clear() { notify.clear(); }
};



class CoinQTxAddressFilter : public CoinQTxFilter
{
private:
    CoinQ::Keys::AddressSet* pAddressSet;
    mutable CoinQSignal<const ChainTransaction&> notify;

public:
    CoinQTxAddressFilter(CoinQ::Keys::AddressSet* _pAddressSet, Mode _mode = Mode::BOTH) : pAddressSet(_pAddressSet) { setMode(_mode); }

    void setAddressSet(CoinQ::Keys::AddressSet* _pAddressSet) { pAddressSet = _pAddressSet; }

    bool push(const ChainTransaction& tx) const;
    void connect(chain_tx_slot_t slot) { notify.connect(slot); }
    void clear() { notify.clear(); }
/*
    void loadFromFile(const std::string& filename);
    void flushToFile(const std::string& filename);*/
};

/*
void CoinQTxFilterMem::addAddresses(const address_set_t& addresses)
{
//    std::set_union(this->addresses.begin(), this->addresses.end(), addresses.begin(), addresses.end(), this->addresses.begin());
}

void CoinQTxFilterMem::addAddress(const std::string& address)
{
    addresses.insert(address);
}

void CoinQTxFilterMem::loadFromFile(const std::string& filename)
{
    boost::filesystem::path p(filename);
    if (!boost::filesystem::exists(p)) {
        throw std::runtime_error("File not found.");
    }

    if (!boost::filesystem::is_regular_file(p)) {
        throw std::runtime_error("Invalid file type.");
    }

    std::string address;
    std::ifstream fs(p.native());
    getline(fs, address);
    while (fs.good()) {
        addAddress(address);
        getline(fs, address);
    }
    if (fs.bad()) {
        throw std::runtime_error("Error reading from file.");
    }
}

void CoinQTxFilterMem::flushToFile(const std::string& filename)
{
    boost::filesystem::path swapfile(filename + ".swp");
    if (boost::filesystem::exists(swapfile)) {
        throw std::runtime_error("Swapfile already exists.");
    }

    {
        std::ofstream fs(swapfile.native());
        for (auto&  address: addresses) {
            if (fs.bad()) {
                throw std::runtime_error("Error writing to file.");
            }
            fs << address << std::endl;
        }
    }

    boost::system::error_code ec;
    boost::filesystem::path p(filename);
    boost::filesystem::rename(swapfile, p, ec);
    if (!!ec) {
        throw std::runtime_error(ec.message());
    }
}
*/
#endif // _COINQ_FILTER_H_
