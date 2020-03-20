#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <unordered_map>

#include "ns3/application.h"

#include "bitcoin-node.h"

namespace bns {

class BitcoinNode;

/**
 * \brief Represents a block. 
 * Block height isn't fixed and transmitted over the wire anymore, but
 * inferred by every node independently.
 * That is blockHeight == 0 can mean:
 * a) Block is the genesis block (test against blockID == 0)
 * b) Block is freshly generated, GetBlockHeight wasn't run yet.
 * c) Height inference has been postponed since we do not know the previous block yet.
 */
struct Block
{
    uint64_t blockID;
    uint64_t prevID;
    uint32_t blockHeight;
    uint32_t blockSize;
};

class Blockchain {
    public:
    Blockchain(BitcoinNode * const nodeCtx);

    /**
     * \brief Return a newly initialized Block structure, height set to 0
     * \return new block
     */
    static Block GetNewBlock (uint64_t blockID, uint64_t prevID, uint32_t blockSize);

    /**
     * \brief Add a block to the blockchain, calculate its height and update
     * the topBlock, if the height is increased
     * \return true if topBlock was update, else false
     */
    bool AddBlock(Block b);


    /**
     * \brief Get the identifier of the highest known block
     */
    uint64_t GetTopBlockID();

    /**
     * \brief Get block by identifier
     */
    Block GetBlockById(uint64_t blockID);

    /**
     * \brief Return the height of the current top block.
     */
    uint32_t GetTopBlockHeight();

    /**
     * \brief Returns if the node has a block
     */
    bool HasBlock (uint64_t blockID);

    /**
     * \brief Calculate block height
     */
    uint32_t GetBlockHeight(uint64_t blockID);
    private: 


    std::unordered_map<uint64_t, Block> m_blockMap;

    // in this map, we store all blocks that were postponed because their
    // prevBlock wasn't present. mapping prevBlockID -> set(waiting blocks)
    std::unordered_map<uint64_t, std::set<uint64_t>> m_waitMap;

    BitcoinNode * const m_nodeCtx;

    // topBlock should always be the last block of the longest chain we seen first
    uint64_t m_topBlockID;

};

}
#endif
