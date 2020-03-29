#include "blockchain.h"
NS_LOG_COMPONENT_DEFINE ("BNSBlockchain");

namespace bns {
Blockchain::Blockchain(BitcoinNode * const nodeCtx) : m_nodeCtx(nodeCtx), m_topBlockID(0)
{
    NS_LOG_FUNCTION(this);

    Block genesisBlock = GetNewBlock(0,0,0);

    AddBlock(genesisBlock);
}

Block 
Blockchain::GetNewBlock (uint64_t blockID, uint64_t prevID, uint32_t blockSize)
{
    Block b;
    b.blockID = blockID;
    b.prevID = prevID;
    b.blockSize = blockSize;
    b.blockHeight = 0;
    return b;
}

bool
Blockchain::AddBlock(Block b)
{
    if(!HasBlock(b.blockID)) {
        m_blockMap[b.blockID] = b;
    }
    bool updatedBest = false;
    uint32_t topBlockHeight = GetBlockHeight(m_topBlockID);

    // check if other are waiting for this block
    if(m_waitMap.count(b.blockID) != 0) {
        std::set<uint64_t> waitingBlocks = m_waitMap[b.blockID];
        for (auto wBlock : waitingBlocks) {
            GetBlockHeight(wBlock);
        }
        m_waitMap.erase(b.blockID);
    }

    uint32_t height = GetBlockHeight(b.blockID);

    if (height > topBlockHeight) {
        //best block was set
        updatedBest = true;
    }
    return updatedBest;
}

uint64_t 
Blockchain::GetTopBlockID()
{
    NS_LOG_FUNCTION(this);
    return m_topBlockID;
}

uint32_t 
Blockchain::GetTopBlockHeight()
{
    return GetBlockHeight(m_topBlockID);
}

Block 
Blockchain::GetBlockById(uint64_t blockID)
{
    NS_LOG_FUNCTION(this);
    assert(m_blockMap.count(blockID) == 1);
    return m_blockMap.at(blockID);
}

bool 
Blockchain::HasBlock (uint64_t blockID)
{
    return (m_blockMap.count(blockID) == 1);
}

uint32_t 
Blockchain::GetBlockHeight(uint64_t blockID)
{
    uint32_t height = 0;
    if (!HasBlock(blockID)) {
        //NS_LOG_INFO("Block not in chain!: " << blockID);
        return height;
    }
    if (blockID == 0) {
        //NS_LOG_INFO("Requested genesisBlock height");
        return height;
    }

    Block& b = m_blockMap[blockID];
    if (b.blockHeight != 0) {
        // if we have a calculated height already, return this
        height = b.blockHeight;
        //NS_LOG_INFO("Returned height (" << height << ") for " << blockID);
        return height;
    }

    // if we land here, we don't have a set hight yet and are not the genesis
    uint32_t prevHeight = GetBlockHeight(b.prevID);
    if (b.prevID != 0 && prevHeight == 0) {
        // we asked for some block other than the genesis and got height 0 
        // => we do not have a connected chain
        // => postpone, keep height at 0
        NS_LOG_INFO("Prev not in chain -- postponed setting height for " << blockID);

        std::set<uint64_t> waitingBlocks;
        if (m_waitMap.count(b.prevID) == 1) {
            std::set<uint64_t> waitingBlocks = m_waitMap[b.blockID];
        }
        waitingBlocks.insert(blockID);
        m_waitMap[b.prevID] = waitingBlocks;
        
        height = 0;
        return height;
    }

    height = prevHeight + 1;

    b.blockHeight = height;

    uint32_t topBlockHeight = GetBlockHeight(m_topBlockID);
    if (height > topBlockHeight) {
        // set new best block
        m_topBlockID = b.blockID;
    }

    NS_LOG_INFO("Set new height (" << height << ") for " << blockID);
    m_nodeCtx->NotifyNewValidBlock(b);

    return height;
}
}
