#include "bitcoin-topology-helper.h"
NS_LOG_COMPONENT_DEFINE ("BNSBitcoinTopologyHelper");

namespace bns {

ns3::Ptr<ns3::Node>
BitcoinTopologyHelper::GetRouter (Region reg)
{
    return routerMap[reg].Get(0);
};

ns3::Ptr<ns3::Node>
BitcoinTopologyHelper::GetLeaf (Region reg, unsigned int index)
{
    return leafMap[reg].Get(index);
};

ns3::Ptr<ns3::Node>
BitcoinTopologyHelper::GetTopologyLeaf(unsigned int index)
{
    return topologyLeafs.Get(index);
};

ns3::Ipv4Address
BitcoinTopologyHelper::GetRouterAddress (Region reg, unsigned int index)
{
    return routerInterfaceMap[reg].GetAddress(index);
};

ns3::Ipv4Address
BitcoinTopologyHelper::GetLeafAddress (Region reg, unsigned int index)
{
    return leafInterfaceMap[reg].GetAddress(index);
};

ns3::Ipv4Address
BitcoinTopologyHelper::GetTopologyAddress(unsigned int index)
{
    return topologyInterfaces.GetAddress(index);
};

ns3::NetDeviceContainer 
BitcoinTopologyHelper::GetIntracontinentalDevices (Region reg)
{
    return hubDevMap[reg];
};

unsigned int
BitcoinTopologyHelper::GetNumberOfLeafs (Region reg)
{
    return numLeafsMap[reg];
}

BitcoinTopologyHelper::BitcoinTopologyHelper(unsigned int nLeafs, uint32_t seed) : m_nLeafs(nLeafs), m_generator(seed)
{
    ReadRegionShares();
    ReadDataRates();
    ReadLatencies();

    PopulateTopology();

    ConnectTopology();

    ConfigureIP();

    //Merge all containers into one big container for easier indexing in the mail application. Using container is optional.
    std::vector<bns::Region> regs = {{bns::Region::NA, bns::Region::EU, bns::Region::AS, bns::Region::OC, bns::Region::AF, bns::Region::SA, bns::Region::CN}};

    for (auto r : regs) {
        topologyLeafs.Add(leafMap[r]);
    }

    for (auto r : regs) {
        topologyInterfaces.Add(leafInterfaceMap[r]);
    }

    NS_LOG_INFO("All Continents successfully connected");
    NS_LOG_INFO("Amount of Nodes in each Continent:");
    NS_LOG_INFO("NA: " << numLeafsMap[Region::NA]);
    NS_LOG_INFO("EU: " << numLeafsMap[Region::EU]);
    NS_LOG_INFO("AS: " << numLeafsMap[Region::AS]);
    NS_LOG_INFO("OC: " << numLeafsMap[Region::OC]);
    NS_LOG_INFO("AF: " << numLeafsMap[Region::AF]);
    NS_LOG_INFO("SA: " << numLeafsMap[Region::SA]);
    NS_LOG_INFO("CN: " << numLeafsMap[Region::CN]);
    NS_LOG_INFO("TOTAL: " << numLeafsMap[Region::NA] + numLeafsMap[Region::EU] + numLeafsMap[Region::AS] + numLeafsMap[Region::OC] + numLeafsMap[Region::AF] + numLeafsMap[Region::SA] + numLeafsMap[Region::CN]);
}

void
BitcoinTopologyHelper::PopulateTopology ()
{
    std::vector<bns::Region> regs = {{bns::Region::NA, bns::Region::EU, bns::Region::AS, bns::Region::OC, bns::Region::AF, bns::Region::SA, bns::Region::CN}};

    for (auto r : regs) {
        PopulateRegion(r);
    }
}

void
BitcoinTopologyHelper::PopulateRegion (bns::Region reg)
{
    // create router
    routerMap[reg].Create(1);
    // create leafs
    leafMap[reg].Create(numLeafsMap[reg]);
}

void
BitcoinTopologyHelper::ReadDataRates ()
{
    downloadRateMap[Region::NA] = GEO_NA_DOWNLOAD;
    downloadRateMap[Region::EU] = GEO_EU_DOWNLOAD;
    downloadRateMap[Region::AS] = GEO_AS_DOWNLOAD;
    downloadRateMap[Region::OC] = GEO_OC_DOWNLOAD;
    downloadRateMap[Region::AF] = GEO_AF_DOWNLOAD;
    downloadRateMap[Region::SA] = GEO_SA_DOWNLOAD;
    downloadRateMap[Region::CN] = GEO_CN_DOWNLOAD;

    uploadRateMap[Region::NA] = GEO_NA_UPLOAD;
    uploadRateMap[Region::EU] = GEO_EU_UPLOAD;
    uploadRateMap[Region::AS] = GEO_AS_UPLOAD;
    uploadRateMap[Region::OC] = GEO_OC_UPLOAD;
    uploadRateMap[Region::AF] = GEO_AF_UPLOAD;
    uploadRateMap[Region::SA] = GEO_SA_UPLOAD;
    uploadRateMap[Region::CN] = GEO_CN_UPLOAD;

    std::vector<bns::Region> regs = {{bns::Region::NA, bns::Region::EU, bns::Region::AS, bns::Region::OC, bns::Region::AF, bns::Region::SA, bns::Region::CN}};

    for (auto r : regs) {
        double downloadMean = mean(downloadRateMap[r]);
        double downloadSD = stddev(downloadRateMap[r]);
        downloadRateDistMap[r] = std::normal_distribution<double> (downloadMean, downloadSD);

        double uploadMean = mean(uploadRateMap[r]);
        double uploadSD = stddev(uploadRateMap[r]);
        uploadRateDistMap[r] = std::normal_distribution<double> (uploadMean, uploadSD);
    }
}

void
BitcoinTopologyHelper::ReadLatencies ()
{
    latencyDistMap[std::make_pair(Region::NA, Region::NA)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_NA_NA_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::NA, Region::EU)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_NA_EU_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::NA, Region::AS)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_NA_AS_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::NA, Region::OC)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_NA_OC_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::NA, Region::AF)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_NA_AF_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::NA, Region::SA)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_NA_SA_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::NA, Region::CN)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_NA_CN_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::EU, Region::EU)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_EU_EU_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::EU, Region::AS)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_EU_AS_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::EU, Region::OC)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_EU_OC_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::EU, Region::AF)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_EU_AF_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::EU, Region::SA)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_EU_SA_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::EU, Region::CN)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_EU_CN_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::AS, Region::AS)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_AS_AS_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::AS, Region::OC)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_AS_OC_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::AS, Region::AF)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_AS_AF_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::AS, Region::SA)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_AS_SA_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::AS, Region::CN)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_AS_CN_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::OC, Region::OC)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_OC_OC_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::OC, Region::AF)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_OC_AF_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::OC, Region::SA)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_OC_SA_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::OC, Region::CN)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_OC_CN_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::SA, Region::SA)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_SA_SA_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::SA, Region::SA)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_SA_AF_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::SA, Region::CN)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_SA_CN_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::AF, Region::AF)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_AF_AF_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::AF, Region::CN)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_AF_CN_LATENCIES.begin());
    latencyDistMap[std::make_pair(Region::CN, Region::CN)] = std::piecewise_linear_distribution<double> (GEO_INTERVALS.begin(), GEO_INTERVALS.end(), GEO_CN_CN_LATENCIES.begin());
}

void
BitcoinTopologyHelper::ReadRegionShares ()
{
    /* Calculate Shares of nodes for each continent (Percentage variables taken from bitcoinNodes.h against nLeafs argument)
       Additionally, the n/a (not available) nodes, denoted as NODES_IN_PERCENT_UNKNOWN are divided by 7 (once for each continent + china) and evenly distributed across all continents. */

    std::vector<double> weights;
    weights.push_back(std::round(GEO_NODES_IN_PERCENT_NA + GEO_NODES_IN_PERCENT_UNKNOWN/7));
    weights.push_back(std::round(GEO_NODES_IN_PERCENT_EU + GEO_NODES_IN_PERCENT_UNKNOWN/7));
    weights.push_back(std::round(GEO_NODES_IN_PERCENT_AS + GEO_NODES_IN_PERCENT_UNKNOWN/7));
    weights.push_back(std::round(GEO_NODES_IN_PERCENT_OC + GEO_NODES_IN_PERCENT_UNKNOWN/7));
    weights.push_back(std::round(GEO_NODES_IN_PERCENT_AF + GEO_NODES_IN_PERCENT_UNKNOWN/7));
    weights.push_back(std::round(GEO_NODES_IN_PERCENT_SA + GEO_NODES_IN_PERCENT_UNKNOWN/7));
    weights.push_back(std::round(GEO_NODES_IN_PERCENT_CN + GEO_NODES_IN_PERCENT_UNKNOWN/7));

    std::discrete_distribution<> dist(weights.begin(), weights.end());
    for (uint16_t i = 0; i < m_nLeafs; ++i) {
        int res = dist(m_generator);
        switch (res) {
            case 0: numLeafsMap[Region::NA]++;
                break;
            case 1: numLeafsMap[Region::EU]++;
                break;
            case 2: numLeafsMap[Region::AS]++;
                break;
            case 3: numLeafsMap[Region::OC]++;
                break;
            case 4: numLeafsMap[Region::AF]++;
                break;
            case 5: numLeafsMap[Region::SA]++;
                break;
            case 6: numLeafsMap[Region::CN]++;
                break;
        }
    }
}

void
BitcoinTopologyHelper::ConnectTopology ()
{
    std::vector<Region> regs = {{Region::NA, Region::EU, Region::AS, Region::OC, Region::AF, Region::SA, Region::CN}};

    for (auto r : regs) {
        ConnectRegionLeafs(r);
    }

    std::vector<Region> otherRegions;
    std::copy(std::begin(regs), std::end(regs), std::back_inserter(otherRegions));

    for (auto r1 : regs) {
        otherRegions.erase(std::remove(std::begin(otherRegions), std::end(otherRegions), r1));
        for (auto r2 : otherRegions) {
            ConnectRegionRouters(r1, r2);
        }
    }
}

void
BitcoinTopologyHelper::ConnectRegionLeafs (Region reg)
{
    for(unsigned int i = 0; i < numLeafsMap[reg]; i++) {
        // pick values randomly from the distributions
        double downloadRate = std::abs(downloadRateDistMap[reg](m_generator));
        double uploadRate = std::abs(uploadRateDistMap[reg](m_generator));
        double linkDelay = std::abs(latencyDistMap[std::make_pair(reg,reg)](m_generator) * 0.25);
        linkDelayMap[reg].push_back(linkDelay);

///        if (reg == Region::CN) {
///NS_LOG_INFO("China: DL " << downloadRate << " / UL " << uploadRate << " / D " << linkDelay);
///        }
///
        ns3::PointToPointHelper internalHelper;
        // Set initial attributes for the link that is assigned by the point to point helper to the channel and the netdevices.
        internalHelper.SetDeviceAttribute("DataRate", ns3::StringValue((std::to_string(downloadRate) + "Mbps")));
        internalHelper.SetChannelAttribute("Delay", ns3::StringValue((std::to_string(linkDelay) + "ms")));


        ns3::QueueSize qs ("10MB");
        internalHelper.SetQueue("ns3::DropTailQueue",  "MaxSize", ns3::QueueSizeValue(qs));
        internalHelper.SetDeviceAttribute ("Mtu", ns3::UintegerValue (1500));

        ns3::NetDeviceContainer temp = internalHelper.Install(GetRouter(reg), GetLeaf(reg, i));

        NS_LOG_INFO("Node " << GetLeaf(reg,i)->GetId() << "(" << RegionToString(reg) << "):");
        NS_LOG_INFO("Setting downloadRate: " << downloadRate);
        NS_LOG_INFO("Setting uploadRate: " << uploadRate);
        NS_LOG_INFO("Setting linkDelay: " << linkDelay);

        // DynamicCasts to assign upload and download datarate separately to the netdevices.
        //Here: temp.Get(0) is the router and temp.Get(1) is the leaf.
        ns3::Ptr<ns3::PointToPointNetDevice> dev0 = ns3::DynamicCast<ns3::PointToPointNetDevice> (temp.Get(0));
        dev0->SetDataRate(ns3::DataRate((std::to_string(downloadRate) + "Mbps")));
        //dev0->TraceConnectWithoutContext("MacRx", ns3::MakeCallback(&ReceivedPacket));

        ns3::Ptr<ns3::PointToPointNetDevice> dev1 = ns3::DynamicCast<ns3::PointToPointNetDevice> (temp.Get(1));
        dev1->SetDataRate(ns3::DataRate((std::to_string(uploadRate) + "Mbps")));

        routerDevMap[reg].Add(dev0);
        hubDevMap[reg].Add(dev0);
        leafDevMap[reg].Add(dev1);
    }
}

void
BitcoinTopologyHelper::ConnectRegionRouters (Region reg0, Region reg1)
{
    ////Calculate intercontinental latencies.
    ////Note: Latencies from the distributions below contain intra-continental latencies too, because they are based on real measurements.
    ////Therefore, the average of each continent is subtracted from the total measured latency and halved, because only one direction is considered.
    short avgLinkDelay0 = mean(linkDelayMap[reg0]);
    short avgLinkDelay1 = mean(linkDelayMap[reg1]);

    unsigned int noOfConnections = GetNumberOfLeafs(reg0) * GetNumberOfLeafs(reg1);

    std::vector<double> intercontinentalLatencies;
    for (unsigned int i = 0; i < noOfConnections; i++) {
        intercontinentalLatencies.push_back(latencyDistMap[std::make_pair(reg0,reg1)](m_generator));
    }
    double avgLatency = mean(intercontinentalLatencies);

    short intercontinentalLinkDelay = std::abs((avgLatency * 0.5) - avgLinkDelay0 - avgLinkDelay1);

    ns3::PointToPointHelper p2pHelper;

    p2pHelper.SetChannelAttribute("Delay", ns3::StringValue((std::to_string(intercontinentalLinkDelay) + "ms")));
    p2pHelper.SetDeviceAttribute("DataRate", ns3::StringValue("1000Gbps"));
    p2pHelper.SetDeviceAttribute ("Mtu", ns3::UintegerValue (1500));

    ns3::QueueSize qs ("100MB");
    p2pHelper.SetQueue("ns3::DropTailQueue",  "MaxSize", ns3::QueueSizeValue(qs));


    //Connect Continent Routers with each other
    ns3::NetDeviceContainer temp;
    temp = p2pHelper.Install(GetRouter(reg0), GetRouter(reg1));

    ns3::Ptr<ns3::PointToPointNetDevice> dev0 = ns3::DynamicCast<ns3::PointToPointNetDevice> (temp.Get(0));
    ns3::Ptr<ns3::PointToPointNetDevice> dev1 = ns3::DynamicCast<ns3::PointToPointNetDevice> (temp.Get(1));

    ns3::Ptr<ns3::ErrorModel> em = ns3::CreateObject<ns3::RateErrorModel> ();
    ns3::Ptr<ns3::RateErrorModel> rem = ns3::DynamicCast<ns3::RateErrorModel> (em);

    ns3::Ptr<ns3::UniformRandomVariable> uv = ns3::CreateObject<ns3::UniformRandomVariable> ();
    rem->SetRandomVariable (uv);
    rem->SetRate (0.069); // Packet Loss Rate

    if (reg0 == Region::CN) {
        dev0->SetReceiveErrorModel(rem);
    } 

    if(reg1 == Region::CN) {
        dev1->SetReceiveErrorModel(rem);
    } 

    routerDevMap[reg0].Add(temp.Get(0));
    routerDevMap[reg1].Add(temp.Get(1));
}

void
BitcoinTopologyHelper::ConfigureIP ()
{
    ns3::Ipv4AddressHelper ipHelper;
    ipHelper.SetBase("10.0.0.0", "255.255.0.0", "0.0.0.1");

    std::vector<bns::Region> regs = {{bns::Region::NA, bns::Region::EU, bns::Region::AS, bns::Region::OC, bns::Region::AF, bns::Region::SA, bns::Region::CN}};
    for (auto r : regs) {
        ConfigureIP(r, ipHelper);
    }
    ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();
}

void
BitcoinTopologyHelper::ConfigureIP (Region reg, ns3::Ipv4AddressHelper& ipHelper)
{
    //// INSTALL IPv4 Stacks
    ns3::InternetStackHelper stack;
    stack.Install(routerMap[reg]);
    stack.Install(leafMap[reg]);

    ////Assign IP Addresses
    routerInterfaceMap[reg] = ipHelper.Assign(routerDevMap[reg]);
    ipHelper.NewNetwork();
    ns3::Ipv4InterfaceContainer con = ipHelper.Assign(leafDevMap[reg]);
    leafInterfaceMap[reg] = con;
    ipHelper.NewNetwork();
}

std::string
BitcoinTopologyHelper::RegionToString(Region reg)
{
    switch(reg) {
        case Region::NA:
            return "NA";
        case Region::EU:
            return "EU";
        case Region::AS:
            return "AS";
        case Region::OC:
            return "OC";
        case Region::AF:
            return "AF";
        case Region::SA:
            return "SA";
        case Region::CN:
            return "CN";
    }
}
}
