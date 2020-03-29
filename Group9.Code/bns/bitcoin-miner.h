#ifndef BITCOIN_MINER_H
#define BITCOIN_MINER_H

#include "ns3/simulator.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"

#include "bitcoin-node.h"
#include "bitcoin-data.h"

namespace bns {

struct Block;
class BitcoinNode;

class BitcoinMiner {
    
    public:
        BitcoinMiner(BitcoinNode * const nodeCtx, double hashRate);
        ~BitcoinMiner();

        void StartMining();
        void StopMining();
        ns3::Time GetNextBlockTime();

        static double blockSizeFactor;
        static double blockIntervalFactor;
    private:
        /**
         * \brief MineBlock gets called when a new block is found.
         */
        void MineBlock(uint64_t prevID);

        uint64_t GetNextBlockSize(); 

        BitcoinNode * const m_nodeCtx;
        double m_hashRate; // hashrate in TH
        double m_difficulty; // current difficulty
        
        bool m_mining;
        ns3::EventId m_curMiningEvent;
};
}
#endif
