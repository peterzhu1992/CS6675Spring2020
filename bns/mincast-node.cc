#include <cmath>
#include <limits>
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/tcp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

#include "bitcoin-node.h"
#include "mincast-node.h"
#include "bitcoin-miner.h"
#include "util.h"

NS_LOG_COMPONENT_DEFINE ("BNSMincastNode");

namespace bns {

BroadcastType MincastNode::minBroadcastType = BroadcastType::SENDHEADERS;

MincastNode::MincastNode (ns3::Ipv4Address address, bool isMiner, double hashRate) : BitcoinNode(address, isMiner, hashRate), m_nInPeers(0), m_nOutPeers(0) 
{
    NS_LOG_FUNCTION (this);
}

MincastNode::~MincastNode(void)
{
    NS_LOG_FUNCTION (this);
}


void 
MincastNode::DoDispose (void)
{
    NS_LOG_FUNCTION (this);

    m_socket = 0;

    // chain up
    Application::DoDispose ();
}


// Application Methods
void 
MincastNode::StartApplication ()    // Called at time specified by Start
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Starting node " << GetNode()->GetId() << ": " << m_address);
    m_isRunning = true;

    InitListenSocket();

    InitOutgoingConnection(); 
    
    if (m_isMiner) {
        ns3::Simulator::Schedule (ns3::Seconds(200), &BitcoinMiner::StartMining, m_miner);
    }
}

void 
MincastNode::StopApplication ()     // Called at time specified by Stop
{
    NS_LOG_FUNCTION (this);

    m_isRunning = false;

    // accept no incoming connections anymore
    m_socket->Close();

    // close outgoing connections
    for (auto pEntry : m_peers) {
        ns3::Ptr<ns3::Socket> socketPtr = pEntry.second.socket;
        socketPtr->Close();
    }
    m_peers.clear();
}

void 
MincastNode::InitListenSocket(void)
{
	NS_LOG_FUNCTION (this);
    //NS_LOG_INFO("Initializing listening socket..");
    if (!m_socket) {
        m_socket = ns3::Socket::CreateSocket (GetNode (), ns3::TcpSocketFactory::GetTypeId ());
        m_socket->Bind (ns3::InetSocketAddress (ns3::Ipv4Address::GetAny (), MIN_PORT));
        m_socket->Listen ();
        m_socket->SetAcceptCallback (
                ns3::MakeNullCallback<bool, ns3::Ptr<ns3::Socket>, const ns3::Address &> (),
                ns3::MakeCallback (&MincastNode::HandleAccept, this));
    }
}

void
MincastNode::InitOutgoingConnection(void)
{
	NS_LOG_FUNCTION (this);
	uint32_t size = m_knownAddresses.size();

    if (size == 0) return;

	if (m_nOutPeers < MIN_MAXCONN_OUT) {
		ns3::Ipv4Address const randAddr = RandomKnownAddress();
		if ((m_address != randAddr) && m_peers.count(randAddr) == 0) {
            NS_LOG_INFO("Connecting to address " << randAddr);
            ns3::InetSocketAddress iAddr = ns3::InetSocketAddress(randAddr, MIN_PORT);
			ns3::Ptr<ns3::Socket> socketPtr = ns3::Socket::CreateSocket (GetNode (), ns3::TcpSocketFactory::GetTypeId ());
			socketPtr-> Bind();
			socketPtr->SetConnectCallback (
					ns3::MakeCallback (&MincastNode::HandleConnect, this),
					ns3::MakeNullCallback<void, ns3::Ptr<ns3::Socket>> ());
			socketPtr-> Connect (iAddr);
		}

        if (m_isRunning) {
            ns3::Time tNext (ns3::MilliSeconds (100));
            ns3::Simulator::Schedule (tNext, &MincastNode::InitOutgoingConnection, this);
        }
	}
}

void 
MincastNode::InitBroadcast (Block& b)
{
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("InitBroadcast");

            //NS_LOG_INFO ("Broadcasting BLOCK " << b.blockID << ", prevID: " << b.prevID << ", size: " << b.blockSize << ").");
    for (auto p : m_peers) {
        ns3::Ipv4Address peerAddr = p.first;
        Peer& peer = p.second;

        if (!PeerKnowsBlock(peerAddr, b.blockID)) {
            std::vector<uint64_t> inventory;
            inventory.push_back(b.blockID);
            switch (MincastNode::minBroadcastType)
            {
                case BroadcastType::UNSOLICITED:
                    SendBlockMessage(peer.socket, b);
                    break;
                case BroadcastType::SENDHEADERS:
                    SendHeadersMessage(peer.socket, inventory);
                    break;
                case BroadcastType::INV:
                    SendInvMessage(peer.socket, inventory);
                    break;
            }
            SetBlockKnown(peerAddr, b.blockID);
        }
    }
    return;
}

ns3::Ipv4Address const
MincastNode::RandomKnownAddress()
{
	auto const size = m_knownAddresses.size();

	ns3::Ptr<ns3::UniformRandomVariable> indexVar = ns3::CreateObject<ns3::UniformRandomVariable> ();	
	indexVar->SetAttribute ("Min", ns3::DoubleValue (0));
	indexVar->SetAttribute ("Max", ns3::DoubleValue (size-1));
	auto const steps = indexVar->GetInteger ();

	auto iter = std::begin(m_knownAddresses);
	std::advance(iter, steps);
	return *iter;
}

void 
MincastNode::HandleAccept (ns3::Ptr<ns3::Socket> socketPtr, const ns3::Address& from)
{
	NS_LOG_FUNCTION (this);

    if(!m_isRunning) return;

    ns3::Ipv4Address peerAddr = GetSocketAddress(socketPtr);

	if (m_address == peerAddr) {
        NS_LOG_INFO("Incoming connection REFUSED (own address): " <<  peerAddr);
		socketPtr->Close ();
		return;
	}

	if (m_peers.count(peerAddr) == 1) {
        NS_LOG_INFO("Incoming connection REFUSED (already connected): " <<  peerAddr);
		socketPtr->Close ();
		return;
	}

	if (m_nInPeers >= MIN_MAXCONN_IN) {
        NS_LOG_INFO("Incoming connection REFUSED (max connections): " <<  peerAddr);
		socketPtr->Close ();
		return;
	}

    NS_LOG_INFO("Incoming connection ESTABLISHED: " <<  peerAddr);

    ns3::Ptr<ns3::TcpSocket> sock = ns3::DynamicCast<ns3::TcpSocket> (socketPtr);
    uint32_t bufSize = 1.5 * 1024 * 1024 * BitcoinMiner::blockSizeFactor;
    sock->SetAttribute("SndBufSize", ns3::UintegerValue(bufSize));
	socketPtr->SetCloseCallbacks (
			ns3::MakeCallback (&MincastNode::HandlePeerClose, this),
			ns3::MakeCallback (&MincastNode::HandlePeerError, this));
	socketPtr->SetRecvCallback (ns3::MakeCallback (&MincastNode::HandleRead, this));
	socketPtr->SetDataSentCallback (ns3::MakeCallback (&MincastNode::HandleSent, this));




    Peer p;
    p.socket = socketPtr;
    p.address = peerAddr;
    p.type = PeerType::IN;
    m_peers[peerAddr] = p;
    m_nInPeers++;

    assert(m_peers.size() == m_nInPeers + m_nOutPeers);
}

void 
MincastNode::HandleConnect (ns3::Ptr<ns3::Socket> socketPtr)
{
    NS_LOG_FUNCTION(this);
    if(!m_isRunning) return;
    ns3::Ipv4Address peerAddr = GetSocketAddress(socketPtr);

	if (m_address == peerAddr || m_peers.count(peerAddr) == 1 || m_nInPeers >= MIN_MAXCONN_IN) {
        NS_LOG_INFO("Outgoing connection CANCELED: " <<  peerAddr);
		socketPtr->Close ();
		return;
	}
    NS_LOG_FUNCTION(this);

    ns3::Ptr<ns3::TcpSocket> sock = ns3::DynamicCast<ns3::TcpSocket> (socketPtr);
    uint32_t bufSize = 1.5 * 1024 * 1024 * BitcoinMiner::blockSizeFactor;
    sock->SetAttribute("SndBufSize", ns3::UintegerValue(bufSize));
	socketPtr->SetCloseCallbacks (
			ns3::MakeCallback (&MincastNode::HandlePeerClose, this),
			ns3::MakeCallback (&MincastNode::HandlePeerError, this));
	socketPtr->SetRecvCallback (ns3::MakeCallback (&MincastNode::HandleRead, this));
	socketPtr->SetDataSentCallback (ns3::MakeCallback (&MincastNode::HandleSent, this));


    NS_LOG_INFO("Outgoing connection ESTABLISHED: " <<  peerAddr);
    
    Peer p;
    p.socket = socketPtr;
    p.address = peerAddr;
    p.type = PeerType::OUT;
    m_peers[peerAddr] = p;
    m_nOutPeers++;

    assert(m_peers.size() == m_nInPeers + m_nOutPeers);
}

void 
MincastNode::SendPacket (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet)
{
	NS_LOG_FUNCTION (this << packet);

    ns3::Ipv4Address peerAddr = GetSocketAddress (socketPtr);

    m_sendQueues[peerAddr].push_back(packet);

	SendAvailable(socketPtr);
}

void
MincastNode::SendAvailable (ns3::Ptr<ns3::Socket> socketPtr)
{
	NS_LOG_FUNCTION (this);

    ns3::Ipv4Address peerAddr = GetSocketAddress (socketPtr);
    if (m_sendQueues.count(peerAddr) == 0) return;
    if (m_sendQueues[peerAddr].empty()) return;

    uint32_t availBytes = socketPtr->GetTxAvailable();
    ns3::Ptr<ns3::Packet> next = m_sendQueues[peerAddr].front();

    if (availBytes < next->GetSize()) return;

    socketPtr->Send(next);
    m_sendQueues[peerAddr].pop_front();
}

void 
MincastNode::SendInvMessage (ns3::Ptr<ns3::Socket> socketPtr, std::vector<uint64_t> inventory)
{
    //ns3::Ipv4Address peerAddr = GetSocketAddress (socketPtr);
	//NS_LOG_DEBUG ("Send INV to " << peerAddr);

    if (inventory.empty()) return;

    MinInvHeader ih;
    ih.SetInventory(inventory);

	ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet> ();
    packet->AddHeader(ih);

	MinTypeHeader th;
    th.SetType (static_cast<uint8_t>(MinMsgType::INV));
	packet->AddHeader(th);

	MinLengthHeader lh;
	lh.SetLength (packet->GetSize ());
	packet->AddHeader(lh);

	SendPacket(socketPtr, packet);
}

void 
MincastNode::SendGetHeadersMessage (ns3::Ptr<ns3::Socket> socketPtr, uint64_t startID, uint64_t stopID)
{
    //ns3::Ipv4Address peerAddr = GetSocketAddress (socketPtr);
	//NS_LOG_DEBUG ("Send GETHEADERS to " << peerAddr);


    MinGetHeadersHeader ghh;
    ghh.SetStartId(startID);
    ghh.SetStopId(stopID);

	ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet> ();
    packet->AddHeader(ghh);

	MinTypeHeader th;
    th.SetType (static_cast<uint8_t>(MinMsgType::GETHEADERS));
	packet->AddHeader(th);

	MinLengthHeader lh;
	lh.SetLength (packet->GetSize ());
	packet->AddHeader(lh);

	SendPacket(socketPtr, packet);
}

void 
MincastNode::SendHeadersMessage (ns3::Ptr<ns3::Socket> socketPtr, std::vector<uint64_t> inventory)
{
    //ns3::Ipv4Address peerAddr = GetSocketAddress (socketPtr);
	//NS_LOG_DEBUG ("Send HEADERS to " << peerAddr);

    if (inventory.empty()) return;

    MinHeadersHeader hh;
    hh.SetInventory(inventory);

	ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet> ();
    packet->AddHeader(hh);

	MinTypeHeader th;
    th.SetType (static_cast<uint8_t>(MinMsgType::HEADERS));
	packet->AddHeader(th);

	MinLengthHeader lh;
	lh.SetLength (packet->GetSize ());
	packet->AddHeader(lh);

	SendPacket(socketPtr, packet);
}

void 
MincastNode::SendGetDataMessage (ns3::Ptr<ns3::Socket> socketPtr, std::vector<uint64_t> inventory)
{
    //ns3::Ipv4Address peerAddr = GetSocketAddress (socketPtr);
	//NS_LOG_DEBUG ("Send GETDATA to " << peerAddr);
    
    if (inventory.empty()) return;

    MinGetDataHeader gdh;
    gdh.SetInventory(inventory);

	ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet> ();
    packet->AddHeader(gdh);

	MinTypeHeader th;
    th.SetType (static_cast<uint8_t>(MinMsgType::GETDATA));
	packet->AddHeader(th);

	MinLengthHeader lh;
	lh.SetLength (packet->GetSize ());
	packet->AddHeader(lh);

	SendPacket(socketPtr, packet);
}

void 
MincastNode::SendBlockMessage (ns3::Ptr<ns3::Socket> socketPtr, Block b)
{
    //ns3::Ipv4Address peerAddr = GetSocketAddress (socketPtr);
	//NS_LOG_DEBUG ("Send BLOCK to " << peerAddr);


    MinBlockHeader bh;
    bh.SetBlockId(b.blockID);
    bh.SetPrevId(b.prevID);

	ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet> (b.blockSize);
    packet->AddHeader(bh);

	MinTypeHeader th;
    th.SetType (static_cast<uint8_t>(MinMsgType::BLOCK));
	packet->AddHeader(th);

	MinLengthHeader lh;
	lh.SetLength (packet->GetSize ());
	packet->AddHeader(lh);

	SendPacket(socketPtr, packet);
}

void 
MincastNode::HandleSent (ns3::Ptr<ns3::Socket> socketPtr, uint32_t availBytes)
{
	NS_LOG_FUNCTION (this << socketPtr << availBytes);
    if(!m_isRunning) return;
	SendAvailable (socketPtr);
}

void 
MincastNode::HandleRead (ns3::Ptr<ns3::Socket> socketPtr)
{
	NS_LOG_FUNCTION (this << socketPtr);
    if(!m_isRunning) return;
    
    ns3::Ipv4Address peerAddr = GetSocketAddress (socketPtr);

	ns3::Ptr<ns3::Packet> p = socketPtr->Recv ();
    if(m_recvQueues.count(peerAddr) == 0) m_recvQueues[peerAddr] = ns3::Create<ns3::Packet> ();

	while (p != 0) {
        m_recvQueues[peerAddr]->AddAtEnd (p);

		ProcessPacket(socketPtr); 

		p = socketPtr->Recv ();
	}
}

void 
MincastNode::HandlePeerClose (ns3::Ptr<ns3::Socket> socketPtr)
{
	NS_LOG_FUNCTION (this << socketPtr);
    if(!m_isRunning) return;

    ns3::Ipv4Address peerAddr = GetSocketAddress(socketPtr);
    Peer &p = m_peers[peerAddr];
    if (p.type == PeerType::OUT) {
        NS_LOG_INFO("Outgoing connection CLOSED: " <<  peerAddr);
        m_nOutPeers--;
    } else {
        NS_LOG_INFO("Incoming connection CLOSED: " <<  peerAddr);
        m_nInPeers--;
    }
    m_peers.erase(peerAddr);
    assert(m_peers.size() == m_nInPeers + m_nOutPeers);
}


void 
MincastNode::HandlePeerError (ns3::Ptr<ns3::Socket> socketPtr)
{
	NS_LOG_FUNCTION (this << socketPtr);
    if(!m_isRunning) return;

    ns3::Ipv4Address peerAddr = GetSocketAddress (socketPtr);
    Peer &p = m_peers[peerAddr];
    if (p.type == PeerType::OUT) {
        NS_LOG_WARN("Outgoing connection ERROR: " <<  peerAddr);
        NS_LOG_WARN("Error: " << show_errno(socketPtr->GetErrno()));
        m_nOutPeers--;
    } else {
        NS_LOG_WARN("Incoming connection ERROR: " <<  peerAddr);
        NS_LOG_WARN("Error: " << show_errno(socketPtr->GetErrno()));
        m_nInPeers--;
    }
    m_peers.erase(peerAddr);
    assert(m_peers.size() == m_nInPeers + m_nOutPeers);
}


void
MincastNode::HandlePacket (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet)
{
	MinTypeHeader th;
	packet->RemoveHeader(th);
    uint8_t type = th.GetType();
    MinMsgType packetType = static_cast<MinMsgType>(type);

    switch (packetType) {
        case MinMsgType::INV:
            HandleInvMessage(socketPtr, packet);
            break;
        case MinMsgType::GETDATA:
            HandleGetDataMessage(socketPtr, packet);
            break;
        case MinMsgType::GETHEADERS:
            HandleGetHeadersMessage(socketPtr, packet);
            break;
        case MinMsgType::HEADERS:
            HandleHeadersMessage(socketPtr, packet);
            break;
        case MinMsgType::GETBLOCKS:
            HandleGetBlocksMessage(socketPtr, packet);
            break;
        case MinMsgType::BLOCK:
            HandleBlockMessage(socketPtr, packet);
            break;
		default: NS_LOG_DEBUG ("Got unknown message: " << packet);
	}
}

void 
MincastNode::ProcessPacket (ns3::Ptr<ns3::Socket> socketPtr)
{
	NS_LOG_FUNCTION (this);

    
    ns3::Ipv4Address peerAddr = GetSocketAddress (socketPtr);
    //NS_LOG_INFO("Processing from: " << peerAddr);

    ns3::Ptr<ns3::Packet> curPacket = m_recvQueues[peerAddr];
    MinLengthHeader lh;
    if (curPacket->GetSize() < lh.GetSerializedSize()) return; // we can't go on further, not enough to read length header

    curPacket->PeekHeader(lh);
    uint32_t length = lh.GetLength();
    //NS_LOG_INFO("length is: " << length << " packet size: " << curPacket->GetSize() << " my addr: " << m_address);
    //
    MinTypeHeader th;
    if (curPacket->GetSize() >= lh.GetSerializedSize() + th.GetSerializedSize()) {
        ns3::Ptr<ns3::Packet> p = curPacket->CreateFragment (lh.GetSerializedSize(), th.GetSerializedSize());
        p->RemoveHeader(th);
    }
    if (curPacket->GetSize() < lh.GetSerializedSize() + length) return; // we can't go on further, not enough for the whole package

    curPacket->RemoveHeader(lh);

    // we have enough data process next packet
    ns3::Ptr<ns3::Packet> p = curPacket->CreateFragment (0, length);
    curPacket->RemoveAtStart(length);
    
    HandlePacket(socketPtr, p);

    if (curPacket->GetSize() > 0) ProcessPacket(socketPtr); // repeat for next packet
}


void 
MincastNode::HandleInvMessage (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet)
{
    ns3::Ipv4Address peerAddr = GetSocketAddress(socketPtr);
    MinInvHeader ih;
    packet->RemoveHeader(ih);

    std::vector<uint64_t> inventory = ih.GetInventory();

    for (uint64_t blockID : inventory) {
        SetBlockKnown(peerAddr, blockID);
    }

    // remove all entries we already have
    inventory.erase(std::remove_if(std::begin(inventory), std::end(inventory), [this](uint64_t id) { return this->m_blockchain->HasBlock(id); }), std::end(inventory));

    // remove all entries we already have requested
    inventory.erase(std::remove_if(std::begin(inventory), std::end(inventory), [this](uint64_t id) { return this->m_requestedBlocks.count(id) == 1; }), std::end(inventory));

    std::sort(std::begin(inventory), std::end(inventory));

    // send getheaders for blocks
    SendGetHeadersMessage(socketPtr, inventory.front(), inventory.back());

    // send getdata for blocks
    if (!inventory.empty()) {
        SendGetDataMessage(socketPtr, inventory);
        for (auto id : inventory) {
            m_requestedBlocks.insert(id);
        }
    }
}

void 
MincastNode::HandleGetDataMessage (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet)
{
    ns3::Ipv4Address peerAddr = GetSocketAddress(socketPtr);
    MinGetDataHeader gdh;
    packet->RemoveHeader(gdh);

    std::vector<uint64_t> inventory = gdh.GetInventory();
    for (auto blockID : inventory) {
        if (m_blockchain->HasBlock(blockID)) {
            Block b = m_blockchain->GetBlockById(blockID);
            SendBlockMessage(socketPtr, b);
            SetBlockKnown(peerAddr, blockID);
        } else {
            NS_LOG_INFO ("Could not find requested BLOCK!!");
        }
    }
}

void 
MincastNode::HandleGetHeadersMessage (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet)
{
    MinGetHeadersHeader ghh;
	packet->RemoveHeader(ghh);

    uint64_t startID = ghh.GetStartId();
    // find startID. if not, assume id 0
    if (!m_blockchain->HasBlock(startID)) startID = 0;

    uint64_t stopID = ghh.GetStopId();
    // find stopID. if not, assume topBlock.
    if(!m_blockchain->HasBlock(stopID)) stopID = m_blockchain->GetTopBlockID();
    if(stopID == startID) return; // do not send anything, if we do not know anything newer

    std::vector<uint64_t> inventory;
    uint64_t curID = stopID;
    while (curID != startID && curID != 0) {
        inventory.push_back(curID);
        curID = m_blockchain->GetBlockById(curID).prevID;
    }
    SendHeadersMessage (socketPtr, inventory);
}

void 
MincastNode::HandleHeadersMessage (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet)
{
    ns3::Ipv4Address peerAddr = GetSocketAddress(socketPtr);
    MinHeadersHeader hh;
    packet->RemoveHeader(hh);

    std::vector<uint64_t> inventory = hh.GetInventory();

    for (uint64_t blockID : inventory) {
        SetBlockKnown(peerAddr, blockID);
    }

    // remove all entries we already have
    inventory.erase(std::remove_if(std::begin(inventory), std::end(inventory), [this](uint64_t id) { return this->m_blockchain->HasBlock(id); }), std::end(inventory));

    // remove all entries we already have requested
    inventory.erase(std::remove_if(std::begin(inventory), std::end(inventory), [this](uint64_t id) { return this->m_requestedBlocks.count(id) == 1; }), std::end(inventory));

    // send getdata for blocks
    if (!inventory.empty()) {
        SendGetDataMessage(socketPtr, inventory);
        for (auto id : inventory) {
            m_requestedBlocks.insert(id);
        }
    }
}

void 
MincastNode::HandleGetBlocksMessage (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet)
{
    MinGetBlocksHeader gbh;
	packet->RemoveHeader(gbh);

    uint64_t startID = gbh.GetStartId();
    // find startID. if not, assume id 0
    if (!m_blockchain->HasBlock(startID)) startID = 0;

    uint64_t stopID = gbh.GetStopId();
    // find stopID. if not, assume topBlock.
    if(!m_blockchain->HasBlock(stopID)) stopID = m_blockchain->GetTopBlockID();
    if(stopID == startID) return; // do not send anything, if we do not know anything newer

    std::vector<uint64_t> inventory;
    uint64_t curID = stopID;
    while (curID != startID && curID != 0) {
        inventory.push_back(curID);
        curID = m_blockchain->GetBlockById(curID).prevID;
    }
    SendInvMessage (socketPtr, inventory);
}

void 
MincastNode::HandleBlockMessage (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet)
{
    //ns3::Ipv4Address peerAddr = GetSocketAddress(socketPtr);

    MinBlockHeader bh;
    packet->RemoveHeader(bh);

    uint64_t newBlockID = bh.GetBlockId();
    uint64_t newPrevID = bh.GetPrevId(); 
    uint32_t newBlockSize = packet->GetSize();
    Block newBlock = Blockchain::GetNewBlock(newBlockID, newPrevID, newBlockSize);

    if (!m_receivedFirstPartBlock || !m_receivedFirstFullBlock) {
        m_receivedFirstPartBlock = true;
        m_receivedFirstFullBlock = true;
        if (newBlock.prevID == 0) {
            SetTTFB(ns3::Simulator::Now());
            SetTTLB(ns3::Simulator::Now());
        }
    }

    // Remove from requested blocks
    m_requestedBlocks.erase(newBlockID);

    ns3::Time delay = GetValidationDelay(newBlock);
    ns3::Simulator::Schedule(delay, &MincastNode::NotifyNewBlock, this, newBlock, false);
}

void
MincastNode::SetBlockKnown (ns3::Ipv4Address peerAddr, uint64_t blockID)
{
    std::vector<ns3::Ipv4Address> &alreadyKnown = m_knownBlocks[blockID];
    auto it = std::find(std::begin(alreadyKnown), std::end(alreadyKnown), peerAddr);
    if (it == std::end(alreadyKnown)) {
        alreadyKnown.push_back(peerAddr);
    }
}

bool 
MincastNode::PeerKnowsBlock (ns3::Ipv4Address peerAddr, uint64_t blockID)
{
    std::vector<ns3::Ipv4Address> &alreadyKnown = m_knownBlocks[blockID];
    auto it = std::find(std::begin(alreadyKnown), std::end(alreadyKnown), peerAddr);
    return (it != std::end(alreadyKnown));
}

ns3::Ipv4Address
GetSocketAddress (ns3::Ptr<ns3::Socket> socketPtr)
{
	ns3::Address addr;
	socketPtr->GetPeerName (addr);
	ns3::InetSocketAddress iaddr = ns3::InetSocketAddress::ConvertFrom (addr);
    return iaddr.GetIpv4();
}
}
