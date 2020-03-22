#include "bitcoin-miner.h"


NS_LOG_COMPONENT_DEFINE ("BNSBitcoinMiner");

namespace bns {

double BitcoinMiner::blockSizeFactor = 1.0;
double BitcoinMiner::blockIntervalFactor = 1.0;

BitcoinMiner::BitcoinMiner(BitcoinNode * const nodeCtx, double hashRate) : m_nodeCtx(nodeCtx), m_hashRate(hashRate), m_difficulty(btcDifficulty), m_mining(false)
{
    NS_LOG_FUNCTION(this);
}

BitcoinMiner::~BitcoinMiner() 
{
    NS_LOG_FUNCTION(this);
    
}

void 
BitcoinMiner::StartMining()
{
    NS_LOG_FUNCTION(this);
    if(m_mining) {
        NS_LOG_INFO("Restarting mining process.");
        ns3::Simulator::Cancel(m_curMiningEvent); // start over
    }
    m_mining = true;
    uint64_t prevID = m_nodeCtx->GetBlockchain()->GetTopBlockID();

    ns3::Time nextBlockTime = GetNextBlockTime();
    NS_LOG_INFO("Starting mining on new block. Prev ID:" << prevID << ", Prev Height: " << m_nodeCtx->GetBlockchain()->GetTopBlockHeight() << ", will be found in " << nextBlockTime.GetSeconds() << " seconds.");
    m_curMiningEvent = ns3::Simulator::Schedule(nextBlockTime, &BitcoinMiner::MineBlock, this, prevID);
}

void 
BitcoinMiner::StopMining()
{
    NS_LOG_FUNCTION(this);
    m_mining = false;
    ns3::Simulator::Cancel(m_curMiningEvent); // stop scheduled mining event
}

void
BitcoinMiner::MineBlock(uint64_t prevID)
{
    NS_LOG_FUNCTION(this);
    if(!m_mining) return; // do nothing if we should not be mining
	ns3::Ptr<ns3::UniformRandomVariable> rand = ns3::CreateObject<ns3::UniformRandomVariable> ();	
    double min = std::numeric_limits<uint64_t>::min();
    double max = std::numeric_limits<uint64_t>::max();
	rand->SetAttribute ("Min", ns3::DoubleValue (min));
	rand->SetAttribute ("Max", ns3::DoubleValue (max));

    uint64_t newBlockID = (uint64_t) rand->GetValue();
    uint64_t newPrevID = prevID;
    uint32_t newBlockSize = GetNextBlockSize();
    Block newBlock = Blockchain::GetNewBlock(newBlockID, newPrevID, newBlockSize);

    m_mining = false;
    m_nodeCtx->NotifyNewBlock(newBlock, true);
}

ns3::Time 
BitcoinMiner::GetNextBlockTime() {
    NS_LOG_FUNCTION(this);
    // avg. time = difficulty * 2**32 / hashRate
    // hashRate is provided in TH/s. 
    // 1 TH/s = 1,000,000,000,000 H/s => 2**32 / 1,000,000,000,000 = 0.004294967296
    uint64_t average_interval_seconds = (m_difficulty * 0.004294967296) / m_hashRate;
    double blockInterval = average_interval_seconds * BitcoinMiner::blockIntervalFactor;
	ns3::Ptr<ns3::UniformRandomVariable> rand = ns3::CreateObject<ns3::UniformRandomVariable> ();	
	rand->SetAttribute ("Min", ns3::DoubleValue (0));
	rand->SetAttribute ("Max", ns3::DoubleValue (1ULL << 48));
    uint64_t next = (int64_t)(log1p(rand->GetValue() * -0.0000000000000035527136788 /* -1/2^48 */) * blockInterval * -1000000.0 + 0.5);
    return ns3::MicroSeconds(next);
}

uint64_t 
BitcoinMiner::GetNextBlockSize() {
    NS_LOG_FUNCTION(this);
    ns3::Ptr<ns3::UniformRandomVariable> x = ns3::CreateObject<ns3::UniformRandomVariable> ();
    x->SetAttribute ("Min", ns3::DoubleValue (0));
    x->SetAttribute ("Max", ns3::DoubleValue (btcBlockSizes.size() - 1));
    auto steps = x->GetInteger ();
    uint64_t blockSize = (uint64_t) (btcBlockSizes[steps] * 1024 * 1024 * BitcoinMiner::blockSizeFactor * BitcoinMiner::blockIntervalFactor);
    return blockSize;
}
}
