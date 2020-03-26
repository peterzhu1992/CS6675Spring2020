#include <cmath>
#include <ios>
#include <algorithm>
#include <numeric>

#include "ns3/core-module.h"
#include "ns3/data-rate.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/mpi-interface.h"

#include "bitcoin-node.h"
#include "kadcast-node.h"
#include "vanilla-node.h"
#include "bitcoin-data.h"
#include "bitcoin-topology-helper.h"
#include "mincast-node.h"

NS_LOG_COMPONENT_DEFINE("BNS");

static double totalTraffic = 0;
static void ReceivedPacket(ns3::Ptr<const ns3::Packet> packet);
void SetReceivedCallback(bns::BitcoinTopologyHelper &topology);

ns3::ApplicationContainer buildStarTopology(struct bnsParams &params);
ns3::ApplicationContainer buildGeoTopology(struct bnsParams &params);

void evaluate(struct bnsParams &params, ns3::ApplicationContainer apps);
void collectPropagationData(struct bnsParams &params, struct bnsResults &res, ns3::ApplicationContainer apps);
void collectTrafficData(struct bnsParams &params, struct bnsResults &res, ns3::ApplicationContainer apps);
void writeResults(struct bnsParams &params, struct bnsResults &res);

double median(std::vector<double> scores);

struct bnsParams
{
    uint32_t seed = 23;
    uint16_t nMinutes = 1000;
    uint32_t nPeers = 100;
    //uint32_t nMiners = bns::btcNumPools;
    uint32_t nMiners = 1;
    uint32_t nBootstrap = nPeers;
    uint32_t nBlocks = 0;
    double blockSizeFactor = 1.0;
    double blockIntervalFactor = 1.0;
    double byzantineFactor = 0.0;
    std::string netStack = "vanilla";
    std::string topo = "geo";

    // vanilla specific
    bool unsolicited = false;

    // kadcast specific
    uint16_t kadK = 20;
    uint16_t kadAlpha = 3;
    uint16_t kadBeta = 5;
    double kadFecOverhead = 0.25;

    // mincast specific
    bool mincastUseScores = false;

    // star topo specific
    std::string starLeafDataRate = "50Mbps";
    std::string starHubDataRate = "100Gbps";
};

struct bnsResults
{
    std::vector<double> ttfbValues;
    std::vector<double> ttlbValues;
    double avgTTFB = 0.0;
    double avgTTLB = 0.0;
    double medianTTFB = 0.0;
    double medianTTLB = 0.0;
    double staleRate = 0.0;
    double coverage = 0.0;
    double overheadRatio = 0.0;
    double totalTraffic = 0;
    double necessaryTraffic = 0;
};

int main(int argc, char *argv[])
{
    ns3::LogComponentEnableAll(ns3::LOG_PREFIX_ALL);
    ns3::LogComponentEnable("BNS", ns3::LOG_LEVEL_INFO);
    ns3::LogComponentEnable("BNSBitcoinTopologyHelper", ns3::LOG_LEVEL_INFO);
    ns3::LogComponentEnable("BNSBitcoinNode", ns3::LOG_LEVEL_INFO);
    ns3::LogComponentEnable("BNSBlockchain", ns3::LOG_LEVEL_INFO);
    ns3::LogComponentEnable("BNSBitcoinMiner", ns3::LOG_LEVEL_INFO);
    ns3::LogComponentEnable("BNSKadcastNode", ns3::LOG_LEVEL_INFO);
    ns3::LogComponentEnable("BNSVanillaNode", ns3::LOG_LEVEL_INFO);
    ns3::LogComponentEnable("BNSVanillaMessages", ns3::LOG_LEVEL_INFO);
    ns3::LogComponentEnable("BNSKadcastMessages", ns3::LOG_LEVEL_INFO);
    ns3::LogComponentEnable("BNSMincastNode", ns3::LOG_LEVEL_INFO);
    ns3::LogComponentEnable("BNSMincastMessages", ns3::LOG_LEVEL_INFO);

    struct bnsParams params;
    ns3::CommandLine cmd;
    cmd.AddValue("seed", "Seed number", params.seed);
    cmd.AddValue("nMinutes", "Number of minutes (simulation time) the network should run.", params.nMinutes);
    cmd.AddValue("nPeers", "Number of peers to build", params.nPeers);
    cmd.AddValue("nBootstrap", "Number of bootstrap peers", params.nBootstrap);
    cmd.AddValue("nMiners", "Number of miners", params.nMiners);
    cmd.AddValue("nBlocks", "Number of blocks to mine, need nMiners=1, stop when reached, use 0 when infinite", params.nBlocks);
    cmd.AddValue("blockSizeFactor", "Set how big blocks are (as a factor of 1 MB)", params.blockSizeFactor);
    cmd.AddValue("blockIntervalFactor", "Set how fast blocks are produced are (as a factor of 10 minutes)", params.blockIntervalFactor);
    cmd.AddValue("byzantineFactor", "Set what part of nodes are byzantine", params.byzantineFactor);
    cmd.AddValue("net", "Set the network stack (vanilla or kadcast or mincast)", params.netStack);
    cmd.AddValue("topo", "Set the network topology (star or geo)", params.topo);

    cmd.AddValue("unsolicited", "Vanilla: Enable unsolicited block transmission.", params.unsolicited);

    cmd.AddValue("kadK", "Kadcast or Mincast: Set the bucket size k.", params.kadK);
    cmd.AddValue("kadAlpha", "Kadcast or Mincast: Set the alpha factor determining the number of parallel lookup requests.", params.kadAlpha);
    cmd.AddValue("kadBeta", "Kadcast or Mincast: Set the beta factor determining the number of parallel broadcast operations.", params.kadBeta);
    cmd.AddValue("kadFecOverhead", "Kadcast or Mincast: Set the FEC overhead factor.", params.kadFecOverhead);
    cmd.AddValue("mincastUseScores", "Mincast: Use scores to determine sending BLOCK or INFORM message, instead of percentages.", params.mincastUseScores);

    cmd.AddValue("starLeafDataRate", "Set the data rate for each link", params.starLeafDataRate);
    cmd.AddValue("starHubRate", "Set the data rate for the star network hub", params.starHubDataRate);

    cmd.Parse(argc, argv);

    if (params.nMiners != 1 && params.nMiners % bns::btcNumPools != 0)
    {
        NS_LOG_INFO("Please pick either a single miner, or a multiple of 16 (as there are 16 major bitcoin pools).");
        return -1;
    }

    bns::BitcoinMiner::blockSizeFactor = params.blockSizeFactor;
    bns::BitcoinMiner::blockIntervalFactor = params.blockIntervalFactor;

    bns::BitcoinNode::nBlocks = params.nBlocks;

    if (params.unsolicited)
    {
        NS_LOG_INFO("Enabled unsolicited block relay.");
        bns::VanillaNode::vanBroadcastType = bns::BroadcastType::UNSOLICITED;
    }

    bns::KadcastNode::kadK = params.kadK;
    bns::KadcastNode::kadAlpha = params.kadAlpha;
    bns::KadcastNode::kadBeta = params.kadBeta;
    bns::KadcastNode::kadFecOverhead = params.kadFecOverhead;

    bns::MincastNode::kadK = params.kadK;
    bns::MincastNode::kadAlpha = params.kadAlpha;
    bns::MincastNode::kadBeta = params.kadBeta;
    bns::MincastNode::kadFecOverhead = params.kadFecOverhead;
    bns::MincastNode::mincastUseScores = params.mincastUseScores;

    ns3::RngSeedManager::SetSeed(time(0));

    ns3::ApplicationContainer apps;
    if (params.topo == "star")
    {
        apps = buildStarTopology(params);
    }
    else
    {
        apps = buildGeoTopology(params);
    }

    // set byzantine nodes randomly
    uint32_t nApps = apps.GetN();
    uint32_t nByzantine = std::min(nApps, (uint32_t)(params.nPeers * params.byzantineFactor));
    std::set<uint32_t> byzApps;

    uint32_t toGo = nByzantine;

    while (toGo > 0)
    {
        ns3::Ptr<ns3::UniformRandomVariable> x = ns3::CreateObject<ns3::UniformRandomVariable>();
        x->SetAttribute("Min", ns3::DoubleValue(0));
        x->SetAttribute("Max", ns3::DoubleValue(nApps - 1));
        uint32_t randomIndex = x->GetInteger();

        if (byzApps.find(randomIndex) == std::end(byzApps))
        {
            ns3::Ptr<bns::BitcoinNode> app = apps.Get(randomIndex)->GetObject<bns::BitcoinNode>();
            app->SetByzantine(true);
            byzApps.insert(randomIndex);
            toGo--;
        }
    }

    NS_LOG_INFO("Marked " << byzApps.size() << " nodes as byzantine.");

    //pointToPoint.EnablePcapAll ("KadcastTest");
    //ns3::Ipv4GlobalRoutingHelper g;
    //ns3::Ptr<ns3::OutputStreamWrapper> routingStream = ns3::Create<ns3::OutputStreamWrapper>
    //    ("routes.txt", std::ios::out);
    //g.PrintRoutingTableAllAt (ns3::Seconds (2), routingStream);

    NS_LOG_INFO("Running Simulator!");
    ns3::Simulator::Stop(ns3::Minutes(params.nMinutes));
    ns3::Simulator::Run();

    evaluate(params, apps);

    ns3::Simulator::Destroy();
    NS_LOG_INFO("Simulation finished!");
    return 0;
}

void evaluate(struct bnsParams &params, ns3::ApplicationContainer apps)
{
    struct bnsResults res;
    collectPropagationData(params, res, apps);
    collectTrafficData(params, res, apps);
    writeResults(params, res);
}

ns3::ApplicationContainer
buildGeoTopology(struct bnsParams &params)
{
    ns3::ApplicationContainer apps;
    bns::BitcoinTopologyHelper topology(params.nPeers, params.seed);

    SetReceivedCallback(topology);

    ns3::Ptr<ns3::UniformRandomVariable> leafIndexVar = ns3::CreateObject<ns3::UniformRandomVariable>();
    leafIndexVar->SetAttribute("Min", ns3::DoubleValue(0));
    leafIndexVar->SetAttribute("Max", ns3::DoubleValue(params.nPeers - 1));

    std::vector<uint32_t> miners;
    uint32_t toAdd = params.nMiners;
    while (toAdd > 0)
    {
        uint32_t randIndex = leafIndexVar->GetInteger();
        auto it = std::find(std::begin(miners), std::end(miners), randIndex);
        if (it == std::end(miners))
        {
            miners.push_back(randIndex);
            toAdd--;
        }
    }

    for (uint32_t i = 0; i < params.nPeers; i++)
    {
        ns3::Ipv4Address nodeAddr = topology.GetTopologyAddress(i);
        //NS_LOG_INFO("Setting up node " << topology.GetTopologyLeaf(i)->GetId() << ": " << nodeAddr);
        ns3::Ptr<bns::BitcoinNode> app;
        if (params.netStack == "kadcast")
        {
            auto it = std::find(std::begin(miners), std::end(miners), i);
            if (it != std::end(miners))
            {                                                       // if the current index is a miner
                auto index = std::distance(std::begin(miners), it); // get its index in miner list
                double poolShare;
                if (params.nMiners == 1)
                {
                    poolShare = 1.0;
                }
                else
                {
                    poolShare = bns::btcHashRateDistribution[index % bns::btcNumPools] / (params.nMiners / bns::btcNumPools);
                }
                double hashRate = poolShare * bns::btcTotalHashRate;
                app = ns3::CreateObject<bns::KadcastNode>(nodeAddr, true, hashRate);
                apps.Add(app);
            }
            else
            {
                app = ns3::CreateObject<bns::KadcastNode>(nodeAddr, false, 0);
                apps.Add(app);
            }
        }
        else if (params.netStack == "mincast")
        {
            auto it = std::find(std::begin(miners), std::end(miners), i);
            if (it != std::end(miners))
            {                                                       // if the current index is a miner
                auto index = std::distance(std::begin(miners), it); // get its index in miner list
                double poolShare;
                if (params.nMiners == 1)
                {
                    poolShare = 1.0;
                }
                else
                {
                    poolShare = bns::btcHashRateDistribution[index % bns::btcNumPools] / (params.nMiners / bns::btcNumPools);
                }
                double hashRate = poolShare * bns::btcTotalHashRate;
                app = ns3::CreateObject<bns::MincastNode>(nodeAddr, true, hashRate);
                apps.Add(app);
            }
            else
            {
                app = ns3::CreateObject<bns::MincastNode>(nodeAddr, false, 0);
                apps.Add(app);
            }
        }
        else
        {
            auto it = std::find(std::begin(miners), std::end(miners), i);
            if (it != std::end(miners))
            {                                                       // if the current index is a miner
                auto index = std::distance(std::begin(miners), it); // get its index in miner list
                double poolShare;
                if (params.nMiners == 1)
                {
                    poolShare = 1.0;
                }
                else
                {
                    poolShare = bns::btcHashRateDistribution[index % bns::btcNumPools] / (params.nMiners / bns::btcNumPools);
                }
                double hashRate = poolShare * bns::btcTotalHashRate;
                app = ns3::CreateObject<bns::VanillaNode>(nodeAddr, true, hashRate);
                apps.Add(app);
            }
            else
            {
                app = ns3::CreateObject<bns::VanillaNode>(nodeAddr, false, 0);
                apps.Add(app);
            }
        }
        topology.GetTopologyLeaf(i)->AddApplication(app);
        app->SetStartTime(ns3::Seconds(2.0));
        //app->SetStopTime(ns3::Minutes (10.0));

        // Bootstrap
        std::vector<ns3::Ipv4Address> peerAddresses;
        uint32_t toBootstrap = std::min(params.nBootstrap, params.nPeers);
        while (toBootstrap > 0)
        {
            uint32_t randIndex = leafIndexVar->GetInteger();
            ns3::Ipv4Address addr = topology.GetTopologyAddress(randIndex);
            auto it = std::find(std::begin(peerAddresses), std::end(peerAddresses), addr);
            if (it == std::end(peerAddresses))
            {
                peerAddresses.push_back(addr);
                toBootstrap--;
            }
        }
        app->SetKnownAddresses(peerAddresses);
    }
    return apps;
}

ns3::ApplicationContainer buildStarTopology(struct bnsParams &params)
{
    ns3::ApplicationContainer apps;
    ns3::Ptr<ns3::UniformRandomVariable> x = ns3::CreateObject<ns3::UniformRandomVariable>();
    x->SetAttribute("Min", ns3::DoubleValue(0));
    x->SetAttribute("Max", ns3::DoubleValue(bns::btcLatencies.size() - 1));

    ns3::PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", ns3::StringValue(params.starLeafDataRate));
    pointToPoint.SetDeviceAttribute("Mtu", ns3::UintegerValue(1500));
    pointToPoint.SetChannelAttribute("Delay", ns3::StringValue("20ms"));

    ns3::QueueSize qs("100MB");
    pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", ns3::QueueSizeValue(qs));

    ns3::PointToPointStarHelper star(params.nPeers, pointToPoint);

    ns3::InternetStackHelper stack;
    star.InstallStack(stack);

    star.AssignIpv4Addresses(ns3::Ipv4AddressHelper("10.0.0.0", "255.255.255.0"));

    ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    ns3::Ptr<ns3::UniformRandomVariable> indexVar = ns3::CreateObject<ns3::UniformRandomVariable>();
    indexVar->SetAttribute("Min", ns3::DoubleValue(0));
    indexVar->SetAttribute("Max", ns3::DoubleValue(params.nPeers - 1));

    uint32_t nDevices = star.GetHub()->GetNDevices();

    for (uint32_t i = 0; i < nDevices - 1; i++)
    {
        ns3::Ptr<ns3::PointToPointNetDevice> dev = ns3::DynamicCast<ns3::PointToPointNetDevice>(star.GetHub()->GetDevice(i));
        dev->SetDataRate(ns3::DataRate(params.starHubDataRate));
        dev->TraceConnectWithoutContext("MacRx", ns3::MakeCallback(&ReceivedPacket));
    }

    for (uint32_t i = 0; i < params.nPeers; ++i)
    {
        auto steps = x->GetInteger();
        double delay = bns::btcLatencies[steps] / 4;
        std::string delayString = "" + std::to_string(delay) + "ms";
        star.GetSpokeNode(i)->GetDevice(0)->GetChannel()->SetAttribute("Delay", ns3::StringValue(delayString));

        ns3::Ipv4Address nodeAddr = star.GetSpokeIpv4Address(i);

        ns3::Ptr<bns::BitcoinNode> app;
        if (params.netStack == "kadcast")
        {
            if (i < params.nMiners)
            {
                if (params.nMiners % bns::btcNumPools == 0)
                {
                    double poolShare = bns::btcHashRateDistribution[i % bns::btcNumPools] / (params.nMiners / bns::btcNumPools);
                    double hashRate = poolShare * bns::btcTotalHashRate;
                    app = ns3::CreateObject<bns::KadcastNode>(nodeAddr, true, hashRate);
                    apps.Add(app);
                }
                else if (params.nMiners == 1)
                {
                    app = ns3::CreateObject<bns::KadcastNode>(nodeAddr, true, bns::btcTotalHashRate);
                    apps.Add(app);
                }
            }
            else
            {
                app = ns3::CreateObject<bns::KadcastNode>(nodeAddr, false, 0);
                apps.Add(app);
            }
        }
        else
        {
            if (i < params.nMiners)
            {
                if (params.nMiners % bns::btcNumPools == 0)
                {
                    double poolShare = bns::btcHashRateDistribution[i % bns::btcNumPools] / (params.nMiners / bns::btcNumPools);
                    double hashRate = poolShare * bns::btcTotalHashRate;
                    app = ns3::CreateObject<bns::VanillaNode>(nodeAddr, true, hashRate);
                    apps.Add(app);
                }
                else if (params.nMiners == 1)
                {
                    app = ns3::CreateObject<bns::VanillaNode>(nodeAddr, true, bns::btcTotalHashRate);
                    apps.Add(app);
                }
            }
            else
            {
                app = ns3::CreateObject<bns::VanillaNode>(nodeAddr, false, 0);
                apps.Add(app);
            }
        }
        star.GetSpokeNode(i)->AddApplication(app);
        app->SetStartTime(ns3::Seconds(2.0));
        //app->SetStopTime(ns3::Minutes (10.0));

        std::vector<ns3::Ipv4Address> peerAddresses;
        uint32_t toBootstrap = params.nBootstrap;
        while (toBootstrap > 0)
        {
            uint32_t randIndex = indexVar->GetInteger();
            ns3::Ipv4Address addr = star.GetSpokeIpv4Address(randIndex);
            auto it = std::find(std::begin(peerAddresses), std::end(peerAddresses), addr);
            if (it == std::end(peerAddresses))
            {
                peerAddresses.push_back(addr);
                toBootstrap--;
            }
        }
        app->SetKnownAddresses(peerAddresses);
    }
    return apps;
}

void collectPropagationData(struct bnsParams &params, struct bnsResults &res, ns3::ApplicationContainer apps)
{
    //
    // Here we evaluate time first byte, time to last byte, and network coverage.
    //
    std::unordered_map<uint64_t, std::vector<double>> ttfbs;
    std::unordered_map<uint64_t, std::vector<double>> ttlbs;
    std::unordered_map<uint64_t, double> firstMiningTime;
    std::unordered_map<uint64_t, ns3::Time>::iterator itr;
    std::unordered_map<uint64_t, std::vector<double>>::iterator itr1;
    std::unordered_map<uint64_t, double>::iterator itr2;
    std::unordered_map<uint64_t, ns3::Time> time_var;
    ns3::Ptr<bns::BitcoinNode> a;

    for (uint32_t i = 0; i < apps.GetN(); ++i)
    {
        a = apps.Get(i)->GetObject<bns::BitcoinNode>();
        time_var = a->GetMiningTime();
        for (itr = time_var.begin(); itr != time_var.end(); itr++)
        {
            double maybe = itr->second.GetMilliSeconds();
            if (a->IsMiner() && maybe != 0)
            {
                if (firstMiningTime.find(itr->first) != firstMiningTime.end())
                    firstMiningTime[itr->first] = std::min(firstMiningTime[itr->first], maybe);
                else
                    firstMiningTime[itr->first] = maybe;
            }
        }
    }

    for (uint32_t i = 0; i < apps.GetN(); ++i)
    {
        a = apps.Get(i)->GetObject<bns::BitcoinNode>();
        time_var = a->GetTTFB();
        for (itr = time_var.begin(); itr != time_var.end(); itr++)
        {
            int64_t tofb = itr->second.GetMilliSeconds();
            if (tofb != 0)
            {
                int64_t ttfb = tofb - firstMiningTime[itr->first];
                NS_LOG_INFO("BlockID: " << itr->first << " TTFB: " << ttfb);

                ttfbs[itr->first].push_back(ttfb);
            }
        }

        time_var = a->GetTTLB();
        for (itr = time_var.begin(); itr != time_var.end(); itr++)
        {
            int64_t tolb = itr->second.GetMilliSeconds();
            if (tolb != 0)
            {
                int64_t ttlb = tolb - firstMiningTime[itr->first];
                NS_LOG_INFO("BlockID: " << itr->first << " TTLB: " << ttlb);
                ttlbs[itr->first].push_back(ttlb);
            }
        }
    }
    int64_t total_ttfb = 0, total_ttlb = 0;
    double avg_ttfb = 0.0, median_ttfb = 0.0, avg_ttlb = 0.0, median_ttlb = 0.0, coverage = 0.0;
    double acc_avg_ttfb = 0.0, acc_median_ttfb = 0.0, acc_avg_ttlb = 0.0, acc_median_ttlb = 0.0, acc_coverage = 0.0;
    NS_LOG_INFO("-----------------------BLOCKWISE STATS-------------------------------");
    for (itr1 = ttfbs.begin(); itr1 != ttfbs.end(); itr1++)
    {
        total_ttfb = std::accumulate(std::begin(itr1->second), std::end(itr1->second), 0);
        avg_ttfb = (double)total_ttfb / (double)itr1->second.size();
        median_ttfb = median(itr1->second);
        acc_avg_ttfb += avg_ttfb;
        acc_median_ttfb += median_ttfb;
        NS_LOG_DEBUG("BlockID: " << itr1->first);
        NS_LOG_DEBUG("total_ttfb: " << total_ttfb);
        NS_LOG_DEBUG("TTFBs size: " << itr1->second.size());
        NS_LOG_DEBUG("Avg. TTFB: " << avg_ttfb);
        NS_LOG_DEBUG("Median TTFB: " << median_ttfb);
    }
    for (itr1 = ttlbs.begin(); itr1 != ttlbs.end(); itr1++)
    {

        total_ttlb = std::accumulate(std::begin(itr1->second), std::end(itr1->second), 0);
        avg_ttlb = (double)total_ttlb / (double)itr1->second.size();
        median_ttlb = median(itr1->second);
        coverage = (double)itr1->second.size() / (double)params.nPeers;
        acc_avg_ttlb += avg_ttlb;
        acc_median_ttlb += median_ttlb;
        acc_coverage += coverage;
        NS_LOG_DEBUG("BlockID: " << itr1->first);
        NS_LOG_DEBUG("total_ttlb: " << total_ttlb);
        NS_LOG_DEBUG("TTLBs size: " << itr1->second.size());
        NS_LOG_DEBUG("Avg. TTLB: " << avg_ttlb);
        NS_LOG_DEBUG("Median TTLB: " << median_ttlb);
        NS_LOG_DEBUG("Coverage: " << coverage);
    }
    NS_LOG_DEBUG("-----------------------OVERALL STATS-------------------------------");
    res.avgTTFB = acc_avg_ttfb / ttfbs.size();
    res.avgTTLB = acc_avg_ttlb / ttlbs.size();
    res.medianTTFB = acc_median_ttfb / ttfbs.size();
    res.medianTTLB = acc_median_ttlb / ttlbs.size();
    res.coverage = acc_coverage / ttlbs.size();
    NS_LOG_DEBUG("Avg. TTFB: " << res.avgTTFB);
    NS_LOG_DEBUG("Avg. TTLB: " << res.avgTTLB);
    NS_LOG_DEBUG("Median TTFB: " << res.medianTTFB);
    NS_LOG_DEBUG("Median TTLB: " << res.medianTTLB);
    NS_LOG_DEBUG("Coverage: " << res.coverage);
}

void collectTrafficData(struct bnsParams &params, struct bnsResults &res, ns3::ApplicationContainer apps)
{
    //
    // Here we evaluate the stale rate and the total network traffic and the overhead
    //
    uint32_t topBlockHeight = 0;
    double nMinedBlocks = 0;
    double totalMinedBlocksSize = 0;
    for (uint32_t i = 0; i < params.nPeers; ++i)
    {
        ns3::Ptr<bns::BitcoinNode> app = apps.Get(i)->GetObject<bns::BitcoinNode>();
        topBlockHeight = std::max(topBlockHeight, app->GetBlockchain()->GetTopBlockHeight());
        nMinedBlocks += app->GetNMinedBlocks();
        totalMinedBlocksSize += app->GetTotalMinedBlocksSize();
    }
    double staleRate = (nMinedBlocks - topBlockHeight) / nMinedBlocks;
    double necessaryTraffic = totalMinedBlocksSize * (params.nPeers - 1);
    double overheadRatio = (totalTraffic - necessaryTraffic) / necessaryTraffic;

    NS_LOG_INFO("Total number of mined blocks: " << nMinedBlocks << ", top block Height: " << topBlockHeight);
    NS_LOG_INFO("Stale rate: " << staleRate << ", totalTraffic: " << totalTraffic << ", overheadRatio: " << overheadRatio);
    res.staleRate = staleRate;
    res.totalTraffic = totalTraffic;
    res.necessaryTraffic = necessaryTraffic;
    res.overheadRatio = overheadRatio;
    return;
}

void writeResults(struct bnsParams &params, struct bnsResults &res)
{
    std::stringstream fileNameStringStream;
    std::string fndel = "_";
    std::string del = ",";
    std::string end = ".csv";

    std::ofstream csv;
    std::string baseStr = "./bns_results";
    fileNameStringStream << baseStr << fndel;
    fileNameStringStream << params.topo << fndel;
    fileNameStringStream << params.netStack << end;

    csv.open(fileNameStringStream.str(), std::ios::app);

    csv << params.seed << del;
    csv << params.nMinutes << del;
    csv << params.nPeers << del;
    csv << params.nMiners << del;
    csv << params.nBootstrap << del;
    csv << params.blockSizeFactor << del;
    csv << params.blockIntervalFactor << del;
    csv << params.byzantineFactor << del;
    csv << params.netStack << del;
    csv << params.topo << del;
    csv << params.kadK << del;
    csv << params.kadAlpha << del;
    csv << params.kadBeta << del;
    csv << params.kadFecOverhead << del;
    csv << res.avgTTFB << del;
    csv << res.avgTTLB << del;
    csv << res.medianTTFB << del;
    csv << res.medianTTLB << del;
    csv << res.staleRate << del;
    csv << res.coverage << del;
    csv << res.overheadRatio << del;
    csv << res.totalTraffic << del;
    csv << res.necessaryTraffic;
    csv << std::endl;
    csv.close();

    std::string ext = "ttfbValues";
    std::stringstream ttfbFileNameStringStream;
    ttfbFileNameStringStream << baseStr << fndel;
    ttfbFileNameStringStream << ext << fndel;
    ttfbFileNameStringStream << params.topo << fndel;
    ttfbFileNameStringStream << params.netStack << end;

    csv.open(ttfbFileNameStringStream.str(), std::ios::app);

    for (auto e : res.ttfbValues)
    {
        csv << params.seed << del;
        csv << params.nMinutes << del;
        csv << params.nPeers << del;
        csv << params.nMiners << del;
        csv << params.nBootstrap << del;
        csv << params.blockSizeFactor << del;
        csv << params.blockIntervalFactor << del;
        csv << params.byzantineFactor << del;
        csv << params.netStack << del;
        csv << params.topo << del;
        csv << params.kadK << del;
        csv << params.kadAlpha << del;
        csv << params.kadBeta << del;
        csv << params.kadFecOverhead << del;
        csv << e;
        csv << std::endl;
    }
    csv.close();

    ext = "ttlbValues";
    std::stringstream ttlbFileNameStringStream;
    ttlbFileNameStringStream << baseStr << fndel;
    ttlbFileNameStringStream << ext << fndel;
    ttlbFileNameStringStream << params.topo << fndel;
    ttlbFileNameStringStream << params.netStack << end;

    csv.open(ttlbFileNameStringStream.str(), std::ios::app);

    for (auto e : res.ttlbValues)
    {
        csv << params.seed << del;
        csv << params.nMinutes << del;
        csv << params.nPeers << del;
        csv << params.nMiners << del;
        csv << params.nBootstrap << del;
        csv << params.blockSizeFactor << del;
        csv << params.blockIntervalFactor << del;
        csv << params.byzantineFactor << del;
        csv << params.netStack << del;
        csv << params.topo << del;
        csv << params.kadK << del;
        csv << params.kadAlpha << del;
        csv << params.kadBeta << del;
        csv << params.kadFecOverhead << del;
        csv << e;
        csv << std::endl;
    }
    csv.close();
}

static void ReceivedPacket(ns3::Ptr<const ns3::Packet> packet)
{
    totalTraffic += packet->GetSize();
}

double median(std::vector<double> scores)
{
    size_t size = scores.size();

    if (size == 0)
    {
        return 0; // Undefined, really.
    }
    else if (size == 1)
    {
        return scores[0];
    }
    else
    {
        sort(scores.begin(), scores.end());
        if (size % 2 == 0)
        {
            return (scores[size / 2 - 1] + scores[size / 2]) / 2;
        }
        else
        {
            return scores[size / 2];
        }
    }
}

void SetReceivedCallback(bns::BitcoinTopologyHelper &topology)
{
    std::vector<bns::Region> regs = {{bns::Region::NA, bns::Region::EU, bns::Region::AS, bns::Region::OC, bns::Region::AF, bns::Region::SA, bns::Region::CN}};
    for (auto r : regs)
    {
        ns3::NetDeviceContainer devs = topology.GetIntracontinentalDevices(r);
        for (uint32_t i = 0; i < devs.GetN(); i++)
        {
            ns3::Ptr<ns3::PointToPointNetDevice> dev = ns3::DynamicCast<ns3::PointToPointNetDevice>(devs.Get(i));
            dev->TraceConnectWithoutContext("MacRx", ns3::MakeCallback(&ReceivedPacket));
        }
    }
}
