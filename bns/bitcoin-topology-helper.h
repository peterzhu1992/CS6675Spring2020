#ifndef BITCOIN_TOPOLOGY_HELPER_H
#define BITCOIN_TOPOLOGY_HELPER_H

#include <random>
#include <cmath>
#include <unordered_map>
#include <string>
#include <numeric>
#include <typeinfo>
#include <functional>
#include <utility>
#include <algorithm>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/error-rate-model.h"
#include "ns3/error-model.h"
#include "ns3/queue.h"
#include "bitcoin-data.h"

namespace bns {
enum class Region { NA, EU, AS, OC, AF, SA, CN };

template<typename T>
    T mean(const std::vector<T> &vec)
    {
        size_t sz = vec.size();
        if (sz == 0) return 0.0;
        return std::accumulate(vec.begin(), vec.end(), 0.0) / sz;
    }

template<typename T>
    T variance(const std::vector<T> &vec)
    {
        size_t sz = vec.size();
        if (sz == 1)
            return 0.0;

        // Calculate the mean
        T mean = std::accumulate(vec.begin(), vec.end(), 0.0) / sz;

        // Now calculate the variance
        auto variance_func = [&mean, &sz](T accumulator, const T& val)
        {
            return accumulator + ((val - mean)*(val - mean) / (sz - 1));
        };

        return std::accumulate(vec.begin(), vec.end(), 0.0, variance_func);
    }

template<typename T>
    T stddev(const std::vector<T> &vec)
    {
        size_t sz = vec.size();
        if (sz == 1)
            return 0.0;

        // Calculate the mean
        T mean = std::accumulate(vec.begin(), vec.end(), 0.0) / sz;

        // Now calculate the variance
        auto variance_func = [&mean, &sz](T accumulator, const T& val)
        {
            return accumulator + ((val - mean)*(val - mean) / (sz - 1));
        };

        return std::sqrt(std::accumulate(vec.begin(), vec.end(), 0.0, variance_func));
    }
}

namespace std {
template <> struct hash<bns::Region> {
    size_t operator() (const bns::Region &t) const { return static_cast<size_t>(t); }
};

template <> struct hash<std::pair<bns::Region, bns::Region> > {
    size_t operator() (const std::pair<bns::Region, bns::Region> &p) const { return static_cast<size_t>(p.first) ^ static_cast<size_t>(p.second); }
};

template <> struct hash<std::set<bns::Region> > {
    size_t operator() (const std::set<bns::Region> &s) const {
        size_t tmp = 0;
        for (bns::Region i : s) {
            tmp = tmp ^ static_cast<size_t>(i);
        }
        return tmp;
    }
};
}

namespace bns {
class BitcoinTopologyHelper{
    public:
        //Constructor
        BitcoinTopologyHelper(unsigned int nLeafs, uint32_t seed);
        // methods do get individual nodes in the bitcoin topology
        ns3::Ptr<ns3::Node> GetRouter(Region reg);
        ns3::Ptr<ns3::Node> GetLeaf (Region reg, unsigned int index);
        ns3::Ptr<ns3::Node> GetTopologyLeaf(unsigned int index);
        ns3::Ipv4Address GetRouterAddress(Region reg, unsigned int index);
        ns3::Ipv4Address GetLeafAddress(Region reg, unsigned int index);
        ns3::Ipv4Address GetTopologyAddress(unsigned int index);

        // Return all devices on the router side of a region (however, only intracontinental).
        ns3::NetDeviceContainer GetIntracontinentalDevices (Region reg);
        unsigned int GetNumberOfLeafs (Region reg);

        std::string RegionToString(Region reg);
    private:
        /*
           Each continent consists of two nodecontainers, one for the router and one for the leaf nodes.
           Each continent has its own ns3::PointToPointHelper to create links between the router and the leafs in the continent
           Each continent has two ns3::NetDeviceContainers which store the Netdevices on the router and the leafs
           Each continent has two ns3::Ipv4InterfaceContainers to assign IP addresses to the router and the leafs
           */
        unsigned int m_nLeafs;

        std::mt19937 m_generator;
        // TOPOLOGY Containers
        ns3::NodeContainer topologyLeafs;
        ns3::Ipv4InterfaceContainer topologyInterfaces;
        std::unordered_map<Region, unsigned int> numLeafsMap;
        std::unordered_map<Region, ns3::NodeContainer> routerMap;
        std::unordered_map<Region, ns3::NodeContainer> leafMap;
        std::unordered_map<Region, ns3::NetDeviceContainer> routerDevMap;
        std::unordered_map<Region, ns3::NetDeviceContainer> hubDevMap;
        std::unordered_map<Region, ns3::NetDeviceContainer> leafDevMap;
        std::unordered_map<Region, ns3::Ipv4InterfaceContainer> routerInterfaceMap;
        std::unordered_map<Region, ns3::Ipv4InterfaceContainer> leafInterfaceMap;

        std::unordered_map<bns::Region, std::vector<float>> downloadRateMap;
        std::unordered_map<bns::Region, std::vector<float>> uploadRateMap;
        std::unordered_map<bns::Region, std::normal_distribution<double>> downloadRateDistMap;
        std::unordered_map<bns::Region, std::normal_distribution<double>> uploadRateDistMap;

        std::unordered_map<std::pair<Region,Region>, std::piecewise_linear_distribution<double>> latencyDistMap;
        std::unordered_map<bns::Region, std::vector<double>> linkDelayMap;

        void ReadDataRates();
        void ReadLatencies();
        void ReadRegionShares();

        void PopulateTopology ();
        void PopulateRegion (Region);

        void ConnectTopology();
        void ConnectRegionLeafs (Region);
        void ConnectRegionRouters (Region reg0, Region reg1);

        void ConfigureIP ();
        void ConfigureIP (Region reg, ns3::Ipv4AddressHelper& ipHelper);
};
}
#endif
