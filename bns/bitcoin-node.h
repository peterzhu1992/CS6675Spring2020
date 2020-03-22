#ifndef BITCOIN_NODE_H
#define BITCOIN_NODE_H

#include <unordered_map>
#include <cassert>

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"

#include "bitcoin-miner.h"
#include "blockchain.h"

namespace bns
{

class Blockchain;
class BitcoinMiner;

class BitcoinNode : public ns3::Application
{
public:
    BitcoinNode(ns3::Ipv4Address address, bool isMiner, double hashRate);
    ~BitcoinNode();

    /**
         * \brief Set known addresses for bootstrapping
         */
    void SetKnownAddresses(const std::vector<ns3::Ipv4Address> &knownAddresses);

    /**
         * \brief Notify when a new block is found.
         */
    void NotifyNewBlock(Block &newBlock, bool mined = false);

    void NotifyNewValidBlock(Block &newBlock);
    /**
         * \brief Initialize a broadcast operation
         */
    virtual void InitBroadcast(Block &b) = 0;

    /**
         * \brief Calculate how long the block validation takes.
         */
    ns3::Time GetValidationDelay(Block &b);

    void SetTTLB(uint64_t blockID, ns3::Time ttlb);
    std::unordered_map<uint64_t, ns3::Time> GetTTLB();

    void SetTTFB(uint64_t blockID, ns3::Time ttfb);
    std::unordered_map<uint64_t, ns3::Time> GetTTFB();

    void SetMiningTime(uint64_t blockID, ns3::Time miningTime);
    std::unordered_map<uint64_t, ns3::Time> GetMiningTime();

    uint32_t GetNMinedBlocks();
    uint32_t GetTotalMinedBlocksSize();

    Blockchain *GetBlockchain();
    bool IsMiner();

    void SetByzantine(bool byzantine);
    bool IsByzantine();

protected:
    /**
         * \brief Pick a hashrate
         */
    double GetHashRate();

    std::vector<ns3::Ipv4Address> m_knownAddresses; //! A vector with known peer addresses
    ns3::Ptr<ns3::Socket> m_socket;                 //!< Listening socket
    ns3::Ipv4Address m_address;
    Blockchain *m_blockchain;
    bool m_isRunning; //<! Is the application running?!
    bool m_isMiner;
    bool m_isSelfish;
    bool m_isByzantine;
    BitcoinMiner *m_miner;
    double m_hashRate; // hashRate in TH/s

    bool m_receivedFirstPartBlock;
    bool m_receivedFirstFullBlock;

private:
    std::unordered_map<uint64_t, ns3::Time> m_ttfb;
    std::unordered_map<uint64_t, ns3::Time> m_ttlb;
    std::unordered_map<uint64_t, ns3::Time> m_miningTime;
    uint32_t m_nMinedBlocks;
    uint32_t m_totalMinedBlocksSize;
};
} // namespace bns
#endif
