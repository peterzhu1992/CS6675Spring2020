#include <cmath>
#include <limits>
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "mincast-node.h"

NS_LOG_COMPONENT_DEFINE("BNSMincastNode");

namespace bns
{

uint16_t MincastNode::kadK = 20;

uint16_t MincastNode::kadAlpha = 3;

uint16_t MincastNode::kadBeta = 3;

double MincastNode::kadFecOverhead = 0.25;

MincastNode::MincastNode(ns3::Ipv4Address address, bool isMiner, double hashRate) : BitcoinNode(address, isMiner, hashRate), m_sending(false)
{
    NS_LOG_FUNCTION(this);
    m_nodeID = GenerateNodeID();
    m_doneBlocks[0] = true;
}

MincastNode::~MincastNode(void)
{
    NS_LOG_FUNCTION(this);
}

void MincastNode::DoDispose(void)
{
    NS_LOG_FUNCTION(this);

    m_socket = 0;

    // chain up
    Application::DoDispose();
}

// Application Methods
void MincastNode::StartApplication() // Called at time specified by Start
{
    NS_LOG_INFO("Starting node " << GetNode()->GetId() << ": " << m_address << " / " << EncodeID(m_nodeID));
    m_isRunning = true;
    if (!m_socket)
    {
        ns3::InetSocketAddress local = ns3::InetSocketAddress(m_address, MINCAST_PORT);
        m_socket = ns3::Socket::CreateSocket(GetNode(), ns3::UdpSocketFactory::GetTypeId());
        m_socket->Bind(local);

        ns3::Ptr<ns3::UdpSocket> sock = ns3::DynamicCast<ns3::UdpSocket>(m_socket);
        uint32_t bufSize = 5 * 1024 * 1024 * BitcoinMiner::blockSizeFactor;
        sock->SetAttribute("RcvBufSize", ns3::UintegerValue(bufSize));

        m_socket->SetRecvCallback(MakeCallback(&MincastNode::HandleRead, this));
        m_socket->SetDataSentCallback(ns3::MakeCallback(&MincastNode::HandleSent, this));
    }

    // Add bootstrap peers
    //NS_LOG_INFO("Boostrapping from " << m_knownAddresses.size() << " peers.");
    ns3::Ptr<ns3::NormalRandomVariable> p = ns3::CreateObject<ns3::NormalRandomVariable>();
    p->SetAttribute("Mean", ns3::DoubleValue(10));
    p->SetAttribute("Variance", ns3::DoubleValue(5));

    for (auto it : m_knownAddresses)
    {
        if (it != m_address)
        {

            ns3::Time temp = ns3::Seconds(p->GetValue());
            while (temp < 0)
            {
                temp = ns3::Seconds(p->GetValue());
            }
            ns3::Simulator::Schedule(temp, &MincastNode::SendPingMessage, this, it);
        }
    }

    ns3::Ptr<ns3::NormalRandomVariable> l = ns3::CreateObject<ns3::NormalRandomVariable>();
    l->SetAttribute("Mean", ns3::DoubleValue(30));
    l->SetAttribute("Variance", ns3::DoubleValue(10));

    // Lookup own node id
    ns3::Time temp = ns3::Seconds(l->GetValue());
    while (temp < 0)
    {
        temp = ns3::Seconds(l->GetValue());
    }
    ns3::Simulator::Schedule(temp, &MincastNode::InitLookupNode, this, m_nodeID);

    // Refresh buckets periodically
    ns3::Ptr<ns3::NormalRandomVariable> x = ns3::CreateObject<ns3::NormalRandomVariable>();
    x->SetAttribute("Mean", ns3::DoubleValue(100));
    x->SetAttribute("Variance", ns3::DoubleValue(30));
    ns3::Time refreshTime = ns3::Seconds(x->GetValue());
    while (refreshTime < 0)
    {
        refreshTime = ns3::Seconds(x->GetValue());
    }
    // ns3::Simulator::Schedule(refreshTime, &MincastNode::PeriodicRefresh, this);

    if (m_isMiner)
    {
        ns3::Simulator::Schedule(ns3::Seconds(200), &BitcoinMiner::StartMining, m_miner);
    }
}

void MincastNode::StopApplication() // Called at time specified by Stop
{
    NS_LOG_FUNCTION(this);

    m_isRunning = false;
    //PrintBuckets();

    for (auto e : m_pendingRefreshes)
    {
        ns3::EventId event = std::get<0>(e.second);
        ns3::Simulator::Remove(event);
    }
    m_pendingRefreshes.clear();
    m_socket->Close();
}

nodeid_t
MincastNode::GenerateNodeID()
{
    NS_LOG_FUNCTION(this);
    return RandomIDInInterval(pow2(0), pow2(MINCAST_ID_LEN));
}

nodeid_t
MincastNode::RandomIDInInterval(uint64_t min, uint64_t max)
{
    NS_LOG_FUNCTION(this);
    double dmin = static_cast<double>(min);
    double dmax = static_cast<double>(max);

    ns3::Ptr<ns3::UniformRandomVariable> x = ns3::CreateObject<ns3::UniformRandomVariable>();

    // GetValue returns a value in [min, max)
    uint64_t encoded_id = x->GetValue(dmin, dmax);

    return DecodeID(encoded_id);
}

ns3::Ipv4Address
MincastNode::RandomAddressFromBucket(short i)
{
    std::vector<bentry_t> &cur_bucket = m_buckets[i];

    ns3::Ptr<ns3::UniformRandomVariable> x = ns3::CreateObject<ns3::UniformRandomVariable>();
    x->SetAttribute("Min", ns3::DoubleValue(0));
    x->SetAttribute("Max", ns3::DoubleValue(cur_bucket.size() - 1));
    auto steps = x->GetInteger();

    //NS_LOG_INFO("size: " << cur_bucket.size() << " steps: " << steps);

    auto it = std::begin(cur_bucket);
    std::advance(it, steps);

    return it->first;
}

uint64_t
MincastNode::Distance(nodeid_t node1, nodeid_t node2)
{
    NS_LOG_FUNCTION(this);
    std::bitset<MINCAST_ID_LEN> bs = node1 ^= node2;
    return bs.to_ulong();
}

uint16_t
MincastNode::BucketIndexFromID(nodeid_t node)
{
    NS_LOG_FUNCTION(this);
    // take distance from local node
    uint64_t d = Distance(m_nodeID, node);

    // iterate all possible intervals
    for (short i = 0; i < MINCAST_ID_LEN; ++i)
    {
        if (d >= pow2(i) && d < pow2(i + 1))
        {
            return i;
        }
    }
    return 0;
}

void MincastNode::UpdateBucket(ns3::Ipv4Address addr, nodeid_t nodeID)
{
    NS_LOG_FUNCTION(this);
    if (nodeID == m_nodeID)
        return;

    // delete pending refreshes.
    auto rit = FindInRefreshes(nodeID);
    if (rit != std::end(m_pendingRefreshes))
    {
        m_pendingRefreshes.erase(rit);
    }

    uint16_t i = BucketIndexFromID(nodeID);
    m_activeBuckets.insert(i);

    auto bit = InitBucket(i, m_buckets);
    std::vector<bentry_t> &bucket = bit->second;

    auto it = FindInBucket(addr, bucket);

    if (it != std::end(bucket))
    {
        // 1. if it is in the bucket, move it to the end
        //if (GetNode()->GetId() == 52) NS_LOG_INFO("Add to the end of bucket " << i);
        bucket.erase(it);
        bucket.push_back(std::make_pair(addr, nodeID));
    }
    else
    {
        // 2. if node id not in bucket
        if (bucket.size() < MincastNode::kadK)
        {
            // 2.1. if fewer than k entries, add it to the end
            //    if (GetNode()->GetId() == 52) NS_LOG_INFO("Just add it to bucket " << i);
            bucket.push_back(std::make_pair(addr, nodeID));
        }
        else
        {
            // 2.2. else ping least recently seen node
            //auto old = bucket.front();
            bucket.erase(bucket.begin());
            bucket.push_back(std::make_pair(addr, nodeID));
            //     if (GetNode()->GetId() == 52) NS_LOG_INFO("Checking least recently seen " << i);
            //RefreshNode(old.first, old.second, addr, nodeID);
        }
    }

    //if (GetNode()->GetId() == 52) {
    //    uint32_t nNodes = 0;
    //    for (auto b : m_buckets) {
    //        nNodes += b.second.size();
    //    }
    //    NS_LOG_INFO ("nNodes total: " << nNodes);
    //}
}

void MincastNode::InitBroadcast(Block &b)
{
    uint16_t startHeight = MINCAST_ID_LEN;

    m_doneBlocks[b.blockID] = true;

    NS_LOG_INFO("InitBroadcast");
    //NS_LOG_INFO ("Initializing Broadcast " << b.blockID);
    if (m_maxSeenHeight.find(b.blockID) == std::end(m_maxSeenHeight))
    {
        m_maxSeenHeight[b.blockID] = startHeight;
    }
    BroadcastBlock(b);
}

void MincastNode::BroadcastBlock(Block &b)
{
    //if (GetNode()->GetId() == 52) NS_LOG_INFO("Broadcast of height: " << height);
    //if (GetNode()->GetId() == 52) {
    //uint32_t nNodes = 0;
    //for (int i = 0; i < MINCAST_ID_LEN; i++) {
    //        NS_LOG_INFO("Bucket " << i << " size: " << m_buckets[i].size());
    //nNodes += m_buckets[i].size();
    //}
    //NS_LOG_INFO ("nNodes total: " << nNodes);
    //}
    uint16_t height = m_maxSeenHeight[b.blockID];
    m_maxSeenHeight.erase(b.blockID);

    if (height == 0)
        return;
    //NS_LOG_INFO ("Initializing Broadcast for block " << b.blockID << ". Height: " << height);

    for (uint16_t bIndex = height - 1; bIndex >= 0 && bIndex < MINCAST_ID_LEN; --bIndex)
    {
        //if (m_buckets[i].size() == 0) NS_LOG_INFO("Bucket " << i << ": empty! (height: " << height << ")");
        if (m_buckets[bIndex].size() == 0)
            continue;

        std::vector<ns3::Ipv4Address> nodeAddresses;
        // Pick MincastNode::kadBeta nodes
        uint16_t toQuery = MincastNode::kadBeta < m_buckets[bIndex].size() ? MincastNode::kadBeta : m_buckets[bIndex].size();
        //if (GetNode()->GetId() == 52) NS_LOG_INFO("Bucket " << i << ": Will query " << toQuery << "/" << m_buckets[i].size() << " nodes.");
        while (toQuery > 0)
        {
            ns3::Ipv4Address nodeAddr = RandomAddressFromBucket(bIndex);
            if (nodeAddr == m_address)
                continue;
            auto it = std::find(std::begin(nodeAddresses), std::end(nodeAddresses), nodeAddr);
            if (it == std::end(nodeAddresses))
            {
                nodeAddresses.push_back(nodeAddr);
                toQuery--;
            }
        }
        std::random_shuffle(nodeAddresses.begin(), nodeAddresses.end());
        // NS_LOG_INFO("will broadcast to " << nodeAddresses.size() << " nodes");

        int nodeAddrLimit = nodeAddresses.size() == kadBeta ? nodeAddresses.size() - 2 : nodeAddresses.size() - 1, ct = 0;
        NS_LOG_INFO(kadBeta << " " << ct << " " << nodeAddresses.size());

        for (auto nAddr : nodeAddresses)
        {
            if (ct <= nodeAddrLimit)
                SendBlock(nAddr, b, bIndex);
            else
                SendInformMessage(nAddr, b.blockID);
            ct++;
        }
    }

    return;
}

void MincastNode::SendBlock(ns3::Ipv4Address &outgoingAddress, Block &b, uint16_t height)
{
    NS_LOG_INFO("Sending block: " << b.blockID << " to: " << outgoingAddress);
    std::map<uint16_t, MinChunk> chunkMap = Chunkify(b);

    std::vector<uint16_t> chunksToSend;
    for (uint16_t chunkID = 0; chunkID < chunkMap.size(); chunkID++)
    {
        chunksToSend.push_back(chunkID);
    }

    while (!chunksToSend.empty())
    {
        ns3::Ptr<ns3::UniformRandomVariable> x = ns3::CreateObject<ns3::UniformRandomVariable>();
        x->SetAttribute("Min", ns3::DoubleValue(0));
        x->SetAttribute("Max", ns3::DoubleValue(chunksToSend.size() - 1));
        auto steps = x->GetInteger();
        auto it = std::begin(chunksToSend);
        std::advance(it, steps);

        SendChunkMessage(outgoingAddress, chunkMap[*it], height);

        m_seenBroadcasts[b.blockID][*it] = true;
        chunksToSend.erase(it);
    }
}

std::unordered_map<uint16_t, std::vector<bentry_t>>::iterator
MincastNode::InitBucket(uint16_t i, std::unordered_map<uint16_t, std::vector<bentry_t>> &bm)
{
    NS_LOG_FUNCTION(this);

    std::pair<uint16_t, std::vector<bentry_t>> p(i, std::vector<bentry_t>());
    auto r = bm.insert(p);
    return r.first;
}

std::vector<bentry_t>::iterator
MincastNode::FindInBucket(ns3::Ipv4Address &addr, std::vector<bentry_t> &bucket)
{
    NS_LOG_FUNCTION(this);

    auto pIsNode = [&addr](const bentry_t &e) { return e.first == addr; };
    auto it = std::find_if(std::begin(bucket), std::end(bucket), pIsNode);
    return it;
}

std::unordered_map<nodeid_t, std::tuple<ns3::EventId, ns3::Ipv4Address, nodeid_t>>::iterator
MincastNode::FindInRefreshes(nodeid_t nodeID)
{
    NS_LOG_FUNCTION(this);

    auto pIsNode = [&nodeID](const std::pair<nodeid_t, std::tuple<ns3::EventId, ns3::Ipv4Address, nodeid_t>> &e) { return e.first == nodeID; };
    auto it = std::find_if(std::begin(m_pendingRefreshes), std::end(m_pendingRefreshes), pIsNode);
    return it;
}

void MincastNode::HandleRead(ns3::Ptr<ns3::Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    if (!m_isRunning)
        return;

    ns3::Ptr<ns3::Packet> packet;
    ns3::Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() == 0)
        { //EOF
            NS_LOG_WARN("Received packet of size 0!");
            break;
        }

        ns3::InetSocketAddress sock_addr = ns3::InetSocketAddress::ConvertFrom(from);
        ns3::Ipv4Address senderAddr = sock_addr.GetIpv4();

        MincastTypeHeader th;
        packet->RemoveHeader(th);
        MincastMsgType packetType = static_cast<MincastMsgType>(th.GetType());

        switch (packetType)
        {
        case MincastMsgType::PING:
        {

            MincastPingHeader ph;
            packet->RemoveHeader(ph);

            uint64_t eSenderID = ph.GetSenderId();
            nodeid_t senderID = DecodeID(eSenderID);

            // Do not act on messages from self
            if (senderID == m_nodeID)
                continue;

            //NS_LOG_INFO("Got PING from node: " << eSenderID << " / " << senderAddr);

            HandlePingMessage(senderAddr, senderID);
            break;
        }
        case MincastMsgType::PONG:
        {
            MincastPingHeader ph;
            packet->RemoveHeader(ph);

            uint64_t eSenderID = ph.GetSenderId();
            nodeid_t senderID = DecodeID(eSenderID);

            // Do not act on messages from self
            if (senderID == m_nodeID)
                continue;

            //if (GetNode()->GetId() == 52) NS_LOG_INFO("Got PONG from node: " << eSenderID << " / " << senderAddr);
            HandlePongMessage(senderAddr, senderID);

            break;
        }
        case MincastMsgType::FINDNODE:
        {
            MincastFindNodeHeader fh;
            packet->RemoveHeader(fh);

            uint64_t eSenderID = fh.GetSenderId();
            nodeid_t senderID = DecodeID(eSenderID);

            uint64_t eTargetID = fh.GetTargetId();
            nodeid_t targetID = DecodeID(eTargetID);

            // Do not act on messages from self
            if (senderID == m_nodeID)
                continue;

            //NS_LOG_INFO("Got FIND_NODE from node " << eSenderID << " / " << senderAddr << ": " << eTargetID);
            HandleFindNodeMessage(senderAddr, senderID, targetID);
            break;
        }
        case MincastMsgType::NODES:
        {
            MincastNodesHeader nh;
            packet->RemoveHeader(nh);

            uint64_t eSenderID = nh.GetSenderId();
            nodeid_t senderID = DecodeID(eSenderID);

            uint64_t eTargetID = nh.GetTargetId();
            nodeid_t targetID = DecodeID(eTargetID);

            // Do not act on messages from self
            if (senderID == m_nodeID)
                continue;

            //NS_LOG_INFO("Got NODES from node: " << eSenderID << " / " << senderAddr);

            std::unordered_map<uint64_t, ns3::Ipv4Address> nodeMap = nh.GetNodes();
            std::vector<bentry_t> nodes;

            nodeid_t nID;
            ns3::Ipv4Address nAddr;

            for (auto it : nodeMap)
            {
                nID = DecodeID(it.first);
                nAddr = it.second;
                nodes.push_back(std::make_pair(nAddr, nID));
            }

            HandleNodesMessage(senderAddr, senderID, targetID, nodes);

            break;
        }
        case MincastMsgType::BROADCAST:
        {
            MincastChunkHeader bh;
            packet->RemoveHeader(bh);

            uint64_t eSenderID = bh.GetSenderId();
            nodeid_t senderID = DecodeID(eSenderID);

            //NS_LOG_INFO("Got BROADCAST from node: " << eSenderID << " / " << senderAddr);

            MinChunk c;
            c.blockID = bh.GetBlockId();
            c.chunkID = bh.GetChunkId();
            c.prevID = bh.GetPrevId();
            c.blockSize = bh.GetBlockSize();
            c.chunkSize = packet->GetSize();
            c.nChunks = bh.GetNChunks();

            uint16_t height = bh.GetHeight();

            HandleChunkMessage(senderAddr, senderID, c, height);
            break;
        }
        case MincastMsgType::REQUEST:
        {
            MincastReqHeader rh;
            packet->RemoveHeader(rh);

            uint64_t eSenderID = rh.GetSenderId();
            nodeid_t senderID = DecodeID(eSenderID);

            NS_LOG_INFO("Got REQUEST from node: " << eSenderID << " / " << senderAddr);

            uint64_t blockID = rh.GetBlockId();

            HandleRequestMessage(senderAddr, senderID, blockID);
            break;
        }
        case MincastMsgType::INFORM:
        {
            MincastReqHeader rh;
            packet->RemoveHeader(rh);

            uint64_t eSenderID = rh.GetSenderId();
            nodeid_t senderID = DecodeID(eSenderID);

            NS_LOG_INFO("Got INFORM from node: " << eSenderID << " / " << senderAddr);

            uint64_t blockID = rh.GetBlockId();

            HandleInformMessage(senderAddr, senderID, blockID);
            break;
        }
        default:
        {
            NS_LOG_WARN("Unrecognized packet received! This should never happen!");
            continue;
        }
        }
    }
}

void MincastNode::HandleSent(ns3::Ptr<ns3::Socket> socketPtr, uint32_t availBytes)
{
    NS_LOG_FUNCTION(this << socketPtr << availBytes);
    SendAvailable();
}

void MincastNode::SendAvailable()
{
    if (!m_socket)
        return;
    if (m_sendQueue.empty())
        return;
    if (m_sending)
        return;
    m_sending = true;

    ns3::Ptr<ns3::PointToPointNetDevice> dev = ns3::DynamicCast<ns3::PointToPointNetDevice>(GetNode()->GetDevice(0));
    ns3::Ptr<ns3::DropTailQueue<ns3::Packet>> queue = ns3::DynamicCast<ns3::DropTailQueue<ns3::Packet>>(dev->GetQueue());

    uint32_t spaceInDevQueue = queue->GetCurrentSize().GetValue() - queue->GetMaxSize().GetValue();
    uint32_t queueSize = m_sendQueue.size();
    uint32_t toSend = std::min(queueSize, spaceInDevQueue);

    //NS_LOG_INFO ("Sending " << toSend << " packets");
    for (uint32_t i = 0; i < toSend; i++)
    {
        ns3::Ipv4Address nextAddr = m_sendQueue.front().first;
        ns3::Ptr<ns3::Packet> nextPacket = m_sendQueue.front().second;
        uint32_t nextSize = nextPacket->GetSize();

        int sent = m_socket->SendTo(nextPacket, 0, ns3::InetSocketAddress(nextAddr, MINCAST_PORT));
        if (sent == -1)
            NS_LOG_WARN("Error sending packet: " << show_errno(m_socket->GetErrno()));
        if (sent != (int32_t)nextSize)
            NS_LOG_WARN("Couldn't send whole packet! Sent " << sent << " / " << nextSize << " bytes.");
        m_sendQueue.pop_front();
    }

    ns3::Simulator::Cancel(m_nextSend);
    ns3::Ptr<ns3::NormalRandomVariable> x = ns3::CreateObject<ns3::NormalRandomVariable>();
    x->SetAttribute("Mean", ns3::DoubleValue(100));
    x->SetAttribute("Variance", ns3::DoubleValue(25));
    ns3::Time sendTime = ns3::MilliSeconds(x->GetValue());
    while (sendTime < 0)
    {
        sendTime = ns3::MilliSeconds(x->GetValue());
    }
    m_nextSend = ns3::Simulator::Schedule(sendTime, &MincastNode::SendAvailable, this);

    m_sending = false;
}

void MincastNode::HandlePingMessage(ns3::Ipv4Address &senderAddr, nodeid_t &senderID)
{
    NS_LOG_FUNCTION(this);
    UpdateBucket(senderAddr, senderID);
    SendPongMessage(senderAddr);
    return;
}

void MincastNode::HandlePongMessage(ns3::Ipv4Address &senderAddr, nodeid_t &senderID)
{
    NS_LOG_FUNCTION(this);
    UpdateBucket(senderAddr, senderID);
    return;
}

void MincastNode::HandleFindNodeMessage(ns3::Ipv4Address &senderAddr, nodeid_t &senderID, nodeid_t &targetID)
{
    NS_LOG_FUNCTION(this);
    UpdateBucket(senderAddr, senderID);

    std::map<uint64_t, bentry_t> kClosest = FindKClosestNodes(targetID);

    std::vector<bentry_t> nodeList;
    for (auto e : kClosest)
    {
        nodeList.push_back(e.second);
    }

    SendNodesMessage(senderAddr, targetID, nodeList);
    return;
}

void MincastNode::HandleNodesMessage(ns3::Ipv4Address &senderAddr, nodeid_t &senderID, nodeid_t &targetID, std::vector<bentry_t> &nodes)
{
    NS_LOG_FUNCTION(this);
    if (m_nodeLookups.count(targetID) == 0)
        return; // we aren't actually looking for this id
    std::map<uint64_t, std::tuple<ns3::Ipv4Address, nodeid_t, bool>> queryMap = m_nodeLookups[targetID];

    bool queryAll = true; // query all formerly unqueried nodes, if we do not make progress
    for (bentry_t e : nodes)
    {
        ns3::Ipv4Address nodeAddr = e.first;
        nodeid_t nodeID = e.second;

        UpdateBucket(nodeAddr, nodeID);

        if (nodeID == targetID)
        {
            NS_LOG_INFO("Found node ID: " << EncodeID(targetID));
            m_nodeLookups.erase(targetID);
            return;
        }

        // 5. Upon receiving NODES msg: if there are closest nodes, update kClosest nodes structure

        uint64_t dist = Distance(nodeID, targetID);
        if (queryMap.count(dist) == 1)
            continue; // we actually already know this node

        std::tuple<ns3::Ipv4Address, nodeid_t, bool> nodeEntry(nodeAddr, nodeID, false);
        queryMap[dist] = nodeEntry;

        if (queryMap.size() > MincastNode::kadK)
        {
            if (queryMap.find(dist) == std::prev(std::end(queryMap)))
            {
                // we didn't add to the end => will not be directly removed => progress happened
                queryAll = false;
            }
            // if there are too many entries, remove entries on the end of the map (aka largest entries)
            queryMap.erase(std::prev(std::end(queryMap)));
        }
    }
    m_nodeLookups[targetID] = queryMap;

    LookupNode(targetID, queryAll);
}

void MincastNode::HandleChunkMessage(ns3::Ipv4Address &senderAddr, nodeid_t &senderID, MinChunk c, uint16_t height)
{
    NS_LOG_FUNCTION(this);
    if (c.blockID == 0)
        return;
    m_receivedFirstPartBlock = true;
    SetTTFB(c.blockID, ns3::Simulator::Now());

    if (!m_doneBlocks[c.prevID] && !m_blockchain->HasBlock(c.prevID))
    {
        if (!m_requestedBlocks[c.prevID])
        {
            RequestMissingBlock(senderAddr, c.prevID);
            m_requestedBlocks[c.prevID] = true;
        }
    }

    if (m_seenBroadcasts[c.blockID][c.chunkID])
        return;
    m_seenBroadcasts[c.blockID][c.chunkID] = true;

    if (m_doneBlocks[c.blockID])
        return;

    m_maxSeenHeight[c.blockID] = std::max(height, m_maxSeenHeight[c.blockID]);

    std::map<uint16_t, MinChunk> &chunkMap = m_receivedChunks[c.blockID];
    if (chunkMap.count(c.chunkID) == 0)
    {
        // only insert chunks once
        chunkMap[c.chunkID] = c;
    }

    if (chunkMap.size() >= c.nChunks && !m_doneBlocks[c.blockID])
    {
        m_doneBlocks[c.blockID] = true;

        m_requestedBlocks.erase(c.blockID);

        // we have all chunks
        Block b = Dechunkify(chunkMap);

        NS_LOG_INFO("Got all Chunks (ID: " << b.blockID << ", prevID: " << b.prevID << ", size: " << b.blockSize << ").");

        m_receivedFirstFullBlock = true;
        SetTTLB(c.blockID, ns3::Simulator::Now());
        ns3::Time delay = GetValidationDelay(b);
        while (delay < 0)
        {
            delay = GetValidationDelay(b);
        }
        ns3::Simulator::Schedule(delay, &MincastNode::NotifyNewBlock, this, b, false);
        chunkMap.clear();
        m_receivedChunks.erase(c.blockID);
    }
    else
    {
        //NS_LOG_INFO("Received " << chunkMap.size() << "/" << c.nChunks << " chunks so far.");
    }

    return;
}

void MincastNode::HandleRequestMessage(ns3::Ipv4Address &senderAddr, nodeid_t &senderID, uint64_t blockID)
{
    if (!m_blockchain->HasBlock(blockID))
    {
        NS_LOG_INFO("Requested block I do not have. This should never happen!");
        return;
    }
    Block b = m_blockchain->GetBlockById(blockID);
    SendBlock(senderAddr, b, 0);

    return;
}

void MincastNode::HandleInformMessage(ns3::Ipv4Address &senderAddr, nodeid_t &senderID, uint64_t blockID)
{
    if (m_seenBroadcasts[blockID].size() > 0)
    {
        NS_LOG_INFO("Already started download of block");
        return;
    }
    int r = 0;
    ns3::Time delay = ns3::Seconds(r);
    while (delay < 0)
    {
        delay = ns3::Seconds(r);
    }
    ns3::Simulator::Schedule(delay, &MincastNode::RequestInformedBlock, this, senderAddr, blockID);
    return;
}

void MincastNode::SendPingMessage(ns3::Ipv4Address &outgoingAddress)
{
    NS_LOG_FUNCTION(this);

    if (outgoingAddress == m_address)
        return; // do not send to self

    ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet>();

    uint64_t eSenderID = EncodeID(m_nodeID);

    MincastPingHeader ph;
    ph.SetSenderId(eSenderID);
    packet->AddHeader(ph);

    MincastTypeHeader th;
    th.SetType(static_cast<uint8_t>(MincastMsgType::PING));
    packet->AddHeader(th);

    //NS_LOG_INFO("Sending PING to " << outgoingAddress);
    //m_socket->SendTo (packet, 0, ns3::InetSocketAddress(outgoingAddress, MINCAST_PORT));
    m_sendQueue.push_back(std::make_pair(outgoingAddress, packet));
    SendAvailable();
}

void MincastNode::SendPongMessage(ns3::Ipv4Address &outgoingAddress)
{
    NS_LOG_FUNCTION(this);

    if (outgoingAddress == m_address)
        return; // do not send to self

    ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet>();

    uint64_t eSenderID = EncodeID(m_nodeID);

    MincastPingHeader ph;
    ph.SetSenderId(eSenderID);
    packet->AddHeader(ph);

    MincastTypeHeader th;
    th.SetType(static_cast<uint8_t>(MincastMsgType::PONG));
    packet->AddHeader(th);

    //NS_LOG_INFO("Replying PONG to " << outgoingAddress);
    //m_socket->SendTo (packet, 0, ns3::InetSocketAddress(outgoingAddress, MINCAST_PORT));
    m_sendQueue.push_back(std::make_pair(outgoingAddress, packet));
    SendAvailable();
}

void MincastNode::SendFindNodeMessage(ns3::Ipv4Address &outgoingAddress, nodeid_t &targetID)
{
    NS_LOG_FUNCTION(this);

    if (outgoingAddress == m_address)
        return; // do not send to self

    ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet>();

    uint64_t eSenderID = EncodeID(m_nodeID);
    uint64_t eTargetID = EncodeID(targetID);

    MincastFindNodeHeader fn_header;
    fn_header.SetSenderId(eSenderID);
    fn_header.SetTargetId(eTargetID);
    packet->AddHeader(fn_header);

    MincastTypeHeader th;
    th.SetType(static_cast<uint8_t>(MincastMsgType::FINDNODE));
    packet->AddHeader(th);

    //NS_LOG_INFO("Sending FIND_NODE to " << outgoingAddress << ": " << packet);
    //m_socket->SendTo (packet, 0, ns3::InetSocketAddress(outgoingAddress, MINCAST_PORT));
    m_sendQueue.push_back(std::make_pair(outgoingAddress, packet));
    SendAvailable();
}

void MincastNode::SendNodesMessage(ns3::Ipv4Address &outgoingAddress, nodeid_t &targetID, std::vector<bentry_t> &nodes)
{
    NS_LOG_FUNCTION(this);

    if (outgoingAddress == m_address)
        return; // do not send to self

    ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet>();

    std::unordered_map<uint64_t, ns3::Ipv4Address> nodeMap;
    for (auto e : nodes)
    {
        uint64_t eNodeID = EncodeID(e.second);
        nodeMap[eNodeID] = e.first;
    }

    uint64_t eSenderID = EncodeID(m_nodeID);
    uint64_t eTargetID = EncodeID(targetID);

    MincastNodesHeader nh;
    nh.SetSenderId(eSenderID);
    nh.SetTargetId(eTargetID);
    nh.SetNodes(nodeMap);
    packet->AddHeader(nh);

    MincastTypeHeader th;
    th.SetType(static_cast<uint8_t>(MincastMsgType::NODES));
    packet->AddHeader(th);

    //NS_LOG_INFO("Sending NODES to " << outgoingAddress << ": " << packet);
    //m_socket->SendTo (packet, 0, ns3::InetSocketAddress(outgoingAddress, MINCAST_PORT));
    m_sendQueue.push_back(std::make_pair(outgoingAddress, packet));
    SendAvailable();
}

void MincastNode::SendChunkMessage(ns3::Ipv4Address &outgoingAddress, MinChunk c, uint16_t height)
{
    NS_LOG_FUNCTION(this);

    if (outgoingAddress == m_address)
        return; // do not send to self

    ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet>(c.chunkSize);

    uint64_t eSenderID = EncodeID(m_nodeID);

    MincastChunkHeader ch;
    ch.SetSenderId(eSenderID);
    ch.SetBlockId(c.blockID);
    ch.SetChunkId(c.chunkID);
    ch.SetPrevId(c.prevID);
    ch.SetBlockSize(c.blockSize);
    ch.SetNChunks(c.nChunks);
    ch.SetHeight(height);
    packet->AddHeader(ch);

    MincastTypeHeader th;
    th.SetType(static_cast<uint8_t>(MincastMsgType::BROADCAST));
    packet->AddHeader(th);

    //int sent = m_socket->SendTo (packet, 0, ns3::InetSocketAddress(outgoingAddress, MINCAST_PORT));
    //NS_LOG_INFO("Sending BROADCAST to " << outgoingAddress << " sent: " << sent << "/" << packet->GetSize() << ".");
    //if (sent != packet->GetSize()) NS_LOG_INFO("Could not send a complete chunk!");
    m_sendQueue.push_back(std::make_pair(outgoingAddress, packet));
    SendAvailable();
}

void MincastNode::SendRequestMessage(ns3::Ipv4Address &outgoingAddress, uint64_t blockID)
{
    NS_LOG_FUNCTION(this);

    if (outgoingAddress == m_address)
        return; // do not send to self

    ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet>();

    uint64_t eSenderID = EncodeID(m_nodeID);
    NS_LOG_INFO("Send REQUEST to node " << eSenderID << " / " << outgoingAddress);

    MincastReqHeader rh;
    rh.SetSenderId(eSenderID);
    rh.SetBlockId(blockID);
    packet->AddHeader(rh);

    MincastTypeHeader th;
    th.SetType(static_cast<uint8_t>(MincastMsgType::REQUEST));
    packet->AddHeader(th);

    m_sendQueue.push_back(std::make_pair(outgoingAddress, packet));
    SendAvailable();
}

void MincastNode::SendInformMessage(ns3::Ipv4Address &outgoingAddress, uint64_t blockID)
{
    NS_LOG_FUNCTION(this);

    if (outgoingAddress == m_address)
        return; // do not send to self

    ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet>();

    uint64_t eSenderID = EncodeID(m_nodeID);
    NS_LOG_INFO("Send INFORM to node " << eSenderID << " / " << outgoingAddress);

    MincastReqHeader rh;
    rh.SetSenderId(eSenderID);
    rh.SetBlockId(blockID);
    packet->AddHeader(rh);

    MincastTypeHeader th;
    th.SetType(static_cast<uint8_t>(MincastMsgType::INFORM));
    packet->AddHeader(th);

    m_sendQueue.push_back(std::make_pair(outgoingAddress, packet));
    SendAvailable();
}

void MincastNode::InitLookupNode(nodeid_t &targetID)
{
    NS_LOG_FUNCTION(this);
    //NS_LOG_INFO("Initializing node lookup: " << EncodeID(targetID));

    // 1. Create LookupNode data structure
    // if already looking up, skip
    if (m_nodeLookups.count(targetID) == 1)
        return;

    // 2. Retrieve kClosest nodes, add to data structure with distance
    std::map<uint64_t, bentry_t> kClosest = FindKClosestNodes(targetID);
    std::map<uint64_t, std::tuple<ns3::Ipv4Address, nodeid_t, bool>> queryMap;
    for (auto e : kClosest)
    {
        ns3::Ipv4Address &nodeAddr = e.second.first;
        nodeid_t &nodeID = e.second.second;
        std::tuple<ns3::Ipv4Address, nodeid_t, bool> p(nodeAddr, nodeID, false);
        queryMap[e.first] = p;
    }
    m_nodeLookups[targetID] = queryMap;

    LookupNode(targetID);
}

void MincastNode::LookupNode(nodeid_t &targetID, bool queryAll)
{
    NS_LOG_FUNCTION(this);
    if (targetID == m_nodeID)
        return;
    if (m_nodeLookups.count(targetID) == 0)
        return; // Make sure we do not acidentally create new entries

    // 3. Check in local buckets
    auto bucket_index = BucketIndexFromID(targetID);
    std::vector<bentry_t> &bucket = m_buckets[bucket_index];

    for (auto e : bucket)
    {
        if (e.second == targetID)
        {
            NS_LOG_INFO("Found node ID in local bucket: " << EncodeID(targetID));
            TerminateLookup(targetID);
            return;
        }
    }

    std::map<uint64_t, std::tuple<ns3::Ipv4Address, nodeid_t, bool>> queryMap = m_nodeLookups[targetID];

    // 4. Query alpha not-yet-queried closest nodes
    unsigned int toQuery = MincastNode::kadAlpha;
    unsigned int nQueried = 0;

    for (auto e : queryMap)
    {
        if (!queryAll && toQuery == 0)
            break;
        ns3::Ipv4Address nodeAddr;
        nodeid_t nodeID;
        bool queried;
        std::tie(nodeAddr, nodeID, queried) = e.second;

        if (queried)
        {
            nQueried++;
            continue;
        }

        SendFindNodeMessage(nodeAddr, targetID);
        MarkQueried(targetID, nodeID);
        nQueried++;
        toQuery--;
    }

    if (nQueried < MincastNode::kadK)
        return;
    //NS_LOG_INFO ("Terminating node lookup!");
    // terminate after timeout
    ns3::Simulator::Schedule(ns3::Seconds(10), &MincastNode::TerminateLookup, this, targetID);
}

void MincastNode::PeriodicRefresh()
{
    if (!m_isRunning)
        return;
    NS_LOG_INFO("Conducting periodic refresh!");

    RefreshBuckets();

    ns3::Ptr<ns3::NormalRandomVariable> x = ns3::CreateObject<ns3::NormalRandomVariable>();
    x->SetAttribute("Mean", ns3::DoubleValue(MINCAST_BUCKET_REFRESH_TIMEOUT));
    x->SetAttribute("Variance", ns3::DoubleValue(100));
    ns3::Time refreshTime = ns3::Seconds(x->GetValue());
    while (refreshTime < 0)
    {
        refreshTime = ns3::Seconds(x->GetValue());
    }
    ns3::Simulator::Schedule(refreshTime, &MincastNode::PeriodicRefresh, this);
}

void MincastNode::RequestInformedBlock(ns3::Ipv4Address &senderAddr, uint64_t blockID)
{
    if (m_doneBlocks[blockID] || m_blockchain->HasBlock(blockID))
    {
        NS_LOG_INFO("Caught up to block: " << blockID);
        return;
    }
    if (m_seenBroadcasts[blockID].size() > 0)
    {
        NS_LOG_INFO("Already started download of block");
        return;
    }

    SendRequestMessage(senderAddr, blockID);

    ns3::Ptr<ns3::NormalRandomVariable> x = ns3::CreateObject<ns3::NormalRandomVariable>();
    x->SetAttribute("Mean", ns3::DoubleValue(3));
    x->SetAttribute("Variance", ns3::DoubleValue(1));

    ns3::Time nextRequestTime = ns3::Seconds(x->GetValue());
    while (nextRequestTime < 0)
    {
        nextRequestTime = ns3::Seconds(x->GetValue());
    }
    ns3::Simulator::Schedule(nextRequestTime, &MincastNode::RequestInformedBlock, this, senderAddr, blockID);

    NS_LOG_INFO("Requesting INFORMed block " << blockID << " from " << senderAddr);
}

void MincastNode::RequestMissingBlock(ns3::Ipv4Address &senderAddr, uint64_t blockID)
{
    if (m_doneBlocks[blockID] || m_blockchain->HasBlock(blockID))
    {
        NS_LOG_INFO("Caught up to block: " << blockID);
        return;
    }
    if (m_seenBroadcasts[blockID].size() > 0)
    {
        NS_LOG_INFO("Already started download of block");
        return;
    }

    SendRequestMessage(senderAddr, blockID);

    ns3::Ptr<ns3::NormalRandomVariable> x = ns3::CreateObject<ns3::NormalRandomVariable>();
    x->SetAttribute("Mean", ns3::DoubleValue(5000));
    x->SetAttribute("Variance", ns3::DoubleValue(3000));

    ns3::Time nextRequestTime = ns3::MilliSeconds(x->GetValue());
    while (nextRequestTime < 0)
    {
        nextRequestTime = ns3::MilliSeconds(x->GetValue());
    }
    ns3::Simulator::Schedule(nextRequestTime, &MincastNode::RequestMissingBlock, this, senderAddr, blockID);

    NS_LOG_INFO("Requesting missing block " << blockID << " from " << senderAddr << ". Next: " << nextRequestTime);
}

void MincastNode::RefreshBuckets()
{
    NS_LOG_FUNCTION(this);
    for (short i = 0; i < MINCAST_ID_LEN; ++i)
    {
        auto it = m_activeBuckets.find(i);
        if (it != std::end(m_activeBuckets))
            continue;
        uint64_t min = pow2(i);
        uint64_t max = pow2(i + 1);
        nodeid_t random_id = RandomIDInInterval(min, max);
        //NS_LOG_INFO("Looking up random node in [" << min << "," << max << "): " << EncodeID(random_id));
        InitLookupNode(random_id);
    }
    m_activeBuckets.clear();
}

void MincastNode::RefreshNode(ns3::Ipv4Address &oldAddress, nodeid_t oldID, ns3::Ipv4Address &newAddress, nodeid_t newID)
{
    NS_LOG_FUNCTION(this);
    auto it = FindInRefreshes(oldID);
    if (it != std::end(m_pendingRefreshes))
    {
        // already refreshing
        return;
    }

    SendPingMessage(oldAddress);

    if (newID == m_nodeID)
        return;
    if (ns3::Seconds(MINCAST_PING_TIMEOUT) <= 0)
    {
        NS_LOG_INFO("Shit");
    }
    ns3::EventId event = ns3::Simulator::Schedule(ns3::Seconds(MINCAST_PING_TIMEOUT), &MincastNode::RefreshTimeoutExpired, this, oldAddress, oldID);
    std::tuple<ns3::EventId, ns3::Ipv4Address, nodeid_t> eventTuple = std::make_tuple(event, newAddress, newID);
    m_pendingRefreshes[oldID] = eventTuple;
}

void MincastNode::RefreshTimeoutExpired(ns3::Ipv4Address &addr, nodeid_t nodeID)
{
    NS_LOG_FUNCTION(this);

    uint16_t i = BucketIndexFromID(nodeID);

    // Retrieve & erase old peer
    auto bit = InitBucket(i, m_buckets);
    std::vector<bentry_t> bucket = bit->second;
    auto nit = FindInBucket(addr, bucket);
    if (nit != std::end(bucket))
    {
        bucket.erase(nit);
    }

    // Retrieve & push new peer
    auto rit = FindInRefreshes(nodeID);
    if (rit != std::end(m_pendingRefreshes))
    {
        std::tuple<ns3::EventId, ns3::Ipv4Address, nodeid_t> t = rit->second;
        ns3::Ipv4Address newAddr = std::get<1>(t);
        nodeid_t newID = std::get<2>(t);
        if (newID != m_nodeID)
            bucket.push_back(std::make_pair(newAddr, newID));
        m_pendingRefreshes.erase(rit);
    }

    m_buckets[i] = bucket;
}

uint64_t
MincastNode::EncodeID(std::bitset<MINCAST_ID_LEN> &bs)
{
    return bs.to_ullong();
}

std::bitset<MINCAST_ID_LEN>
MincastNode::DecodeID(uint64_t encoded)
{
    std::bitset<MINCAST_ID_LEN> bs(encoded);
    return bs;
}

std::map<uint64_t, bentry_t>
MincastNode::FindKClosestNodes(nodeid_t targetID)
{
    std::map<uint64_t, bentry_t> closestNodes;

    uint64_t dist;

    // Get nodes from the target's bucket
    uint16_t bucket_index = BucketIndexFromID(targetID);
    std::vector<bentry_t> &exactBucket = m_buckets[bucket_index];
    for (bentry_t e : exactBucket)
    {
        dist = Distance(targetID, e.second);
        closestNodes[dist] = e;
    }

    // Get additional closest nodes from other buckets
    uint16_t nNeeded = MincastNode::kadK - closestNodes.size();
    if (nNeeded > 0)
    {
        for (auto i = 0; i < MINCAST_ID_LEN; ++i)
        {
            // iterate all buckets
            std::vector<bentry_t> &cur_bucket = m_buckets[i];
            for (bentry_t e : cur_bucket)
            {
                // iterate all entries, add them to the map and prune the end
                dist = Distance(targetID, e.second);
                if (closestNodes.count(dist) == 0)
                {
                    // if we did not already add the entry, calculate distance and insert to map
                    uint64_t dist = Distance(targetID, e.second);
                    closestNodes[dist] = e;
                    if (closestNodes.size() > MincastNode::kadK)
                    {
                        // if there are too many entries, remove entries on the end of the map (aka largest entries)
                        closestNodes.erase(std::prev(std::end(closestNodes)));
                    }
                }
            }
        }
    }
    return closestNodes;
}

void MincastNode::MarkQueried(nodeid_t &targetID, nodeid_t &nodeID)
{
    if (m_nodeLookups.count(targetID) == 0)
        return; // Make sure we do not acidentally create new entries

    uint64_t dist = Distance(nodeID, targetID);
    if (m_nodeLookups[targetID].count(dist) == 0)
        return; // Make sure we do not acidentally add to the k closest nodes

    std::get<2>(m_nodeLookups[targetID][dist]) = true;
}

bool MincastNode::IsQueried(nodeid_t &targetID, nodeid_t &nodeID)
{
    if (m_nodeLookups.count(targetID) == 0)
        return false; // Make sure we do not acidentally create new entries

    uint64_t dist = Distance(nodeID, targetID);
    if (m_nodeLookups[targetID].count(dist) == 0)
        return false; // Make sure we do not acidentally add to the k closest nodes

    return std::get<2>(m_nodeLookups[targetID][dist]);
}

void MincastNode::PrintBuckets()
{
    for (uint16_t i = 0; i < MINCAST_ID_LEN; ++i)
    {
        std::vector<bentry_t> &cur_bucket = m_buckets[i];
        for (auto iit : cur_bucket)
        {
            NS_LOG_INFO("Bucket " << i << ": " << std::get<0>(iit));
        }
    }
}

std::map<uint16_t, MinChunk>
MincastNode::Chunkify(Block b)
{
    MincastTypeHeader th;
    MincastChunkHeader ch;
    uint16_t packetSize = MINCAST_PACKET_SIZE - th.GetSerializedSize() - ch.GetSerializedSize();

    uint16_t nChunks = b.blockSize / packetSize;
    if (b.blockSize % packetSize != 0)
        nChunks++; // one more chunk

    uint32_t toProcess = b.blockSize;

    std::map<uint16_t, MinChunk> chunks;

    for (unsigned int i = 0; i < nChunks; i++)
    {
        MinChunk c;
        c.chunkID = i;
        c.blockID = b.blockID;
        c.prevID = b.prevID;
        c.blockSize = b.blockSize;

        c.chunkSize = (toProcess >= packetSize ? packetSize : toProcess);
        assert(c.chunkSize <= packetSize);

        c.nChunks = nChunks;

        chunks[i] = c;
        toProcess -= c.chunkSize;
    }

    // checks from here
    assert(toProcess == 0);

    uint32_t blockSize = 0;
    for (auto e : chunks)
    {
        blockSize += e.second.chunkSize;
    }
    assert(blockSize == b.blockSize);

    // add additional chunks to model FEC
    uint16_t nAdditional = (nChunks * MincastNode::kadFecOverhead);
    for (unsigned int i = nChunks; i < nChunks + nAdditional; i++)
    {
        MinChunk c;
        c.chunkID = i;
        c.blockID = b.blockID;
        c.prevID = b.prevID;
        c.blockSize = b.blockSize;

        c.chunkSize = packetSize;
        assert(c.chunkSize <= packetSize);

        c.nChunks = nChunks;

        chunks[i] = c;
    }
    return chunks;
}

Block MincastNode::Dechunkify(std::map<uint16_t, MinChunk> chunks)
{
    assert(chunks.size() > 0);

    MinChunk firstChunk = std::begin(chunks)->second;

    assert(firstChunk.blockID != 0);
    assert(firstChunk.nChunks <= chunks.size());

    uint64_t newBlockID = firstChunk.blockID;
    uint64_t newPrevID = firstChunk.prevID;
    uint32_t newBlockSize = firstChunk.blockSize;
    Block newBlock = Blockchain::GetNewBlock(newBlockID, newPrevID, newBlockSize);

    return newBlock;
}

void MincastNode::TerminateLookup(nodeid_t &targetID)
{
    if (m_nodeLookups.count(targetID) == 0)
        return;
    m_nodeLookups.erase(targetID);
}
} // namespace bns
