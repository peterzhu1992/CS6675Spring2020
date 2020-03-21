#include "bitcoin-node.h"

NS_LOG_COMPONENT_DEFINE ("BNSBitcoinNode");

namespace bns {

BitcoinNode::BitcoinNode (ns3::Ipv4Address address, bool isMiner, double hashRate) : m_socket(0), m_address (address), m_isRunning(false), m_isMiner(isMiner), m_isSelfish(false), m_isByzantine(false), m_miner(nullptr), m_hashRate(hashRate), m_receivedFirstPartBlock(false), m_receivedFirstFullBlock(false), m_ttfb(0), m_ttlb(0), m_miningTime(0), m_nMinedBlocks(0), m_totalMinedBlocksSize(0)
{ 
    NS_LOG_FUNCTION(this);
    m_blockchain = new Blockchain(this);

    if (isMiner) {
        m_miner = new BitcoinMiner(this, m_hashRate);
    }

}

BitcoinNode::~BitcoinNode()
{
    delete m_blockchain;
    delete m_miner;
}


void BitcoinNode::SetKnownAddresses (const std::vector<ns3::Ipv4Address> &knownAddresses)
{
    NS_LOG_FUNCTION(this);
    m_knownAddresses = knownAddresses;
}

void 
BitcoinNode::NotifyNewValidBlock(Block& newBlock)
{
    if (m_isMiner) {
        // Restart mining immediately, validation delay is added before
        // NotifyNewValidBlock is called
        //m_miner->StartMining();

        // ###### Mincast ######
        // Only 1 broadcast, no more restart
        m_miner->StopMining();
        // ###### Mincast ######
    }

    if (!m_isSelfish && !m_isByzantine) {
        InitBroadcast(newBlock);
    }
}

void 
BitcoinNode::NotifyNewBlock(Block& newBlock, bool mined)
{
    NS_LOG_FUNCTION(this);

    if(mined) {
        m_nMinedBlocks++;
        m_totalMinedBlocksSize += newBlock.blockSize;
        if (!m_receivedFirstPartBlock || !m_receivedFirstFullBlock) {
            m_receivedFirstPartBlock = true;
            m_receivedFirstFullBlock = true;
            if (newBlock.prevID == 0) {
                SetTTFB(ns3::Simulator::Now());
                SetTTLB(ns3::Simulator::Now());
                SetMiningTime(ns3::Simulator::Now());
            }
        }
    }

    uint32_t oldHeight = m_blockchain->GetTopBlockHeight();

    if (m_blockchain->HasBlock(newBlock.blockID)) return; // we already have this block

    // Else save all blocks we get
    
    bool updatedTop = m_blockchain->AddBlock(newBlock);

    if (!updatedTop) {
        uint32_t h = m_blockchain->GetBlockHeight(newBlock.blockID);
        NS_LOG_INFO ("Didn't update top block.  (block: " << newBlock.blockID << " height: " << h << " cur: " << oldHeight << ").");
        return; // stop if old
    }

    uint32_t newHeight = m_blockchain->GetTopBlockHeight();
    assert (newHeight > oldHeight); 

    if (mined) {
        NS_LOG_INFO("Mined new BLOCK " << newBlock.blockID << " size: " << newBlock.blockSize);
    } else {
        NS_LOG_INFO("Got new BLOCK: " << newBlock.blockID);
    }

    // if (m_isSelfish) {
    // insert strategy here
    // InitBroadcast(...)
    // }
}

double 
BitcoinNode::GetHashRate()
{
    NS_LOG_FUNCTION(this);
    return m_hashRate;
}

ns3::Time
BitcoinNode::GetValidationDelay(Block& b)
{
    // Adopted from Gervais et al. simulator:
    const int averageBlockSizeBytes = 458263;
    const double averageValidationTimeSeconds = 0.174;
    double validationTime = averageValidationTimeSeconds * b.blockSize / averageBlockSizeBytes;		
    return ns3::Seconds(validationTime);
}

ns3::Time 
BitcoinNode::GetTTFB()
{
    return m_ttfb;
}

void
BitcoinNode::SetTTFB(ns3::Time ttfb)
{
    if (m_ttfb == 0) {
        m_ttfb = ttfb;
    }

}

ns3::Time 
BitcoinNode::GetTTLB()
{
    return m_ttlb;
}

void
BitcoinNode::SetTTLB(ns3::Time ttlb)
{
    if (m_ttlb == 0) {
        m_ttlb = ttlb;
    }
}

ns3::Time 
BitcoinNode::GetMiningTime()
{
    return m_miningTime;
}

void
BitcoinNode::SetMiningTime(ns3::Time miningTime)
{
    if (m_miningTime == 0) {
        m_miningTime = miningTime;
    }

}

uint32_t 
BitcoinNode::GetNMinedBlocks()
{
    return m_nMinedBlocks;
}

uint32_t 
BitcoinNode::GetTotalMinedBlocksSize ()
{
    return m_totalMinedBlocksSize;
}

Blockchain*
BitcoinNode::GetBlockchain ()
{
    return m_blockchain;
}

bool
BitcoinNode::IsMiner()
{
    return m_isMiner;
}

bool
BitcoinNode::IsByzantine()
{
    return m_isByzantine;
}

void
BitcoinNode::SetByzantine(bool byzantine)
{
    m_isByzantine = byzantine;
}
}
