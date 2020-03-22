#include "mincast-messages.h"
NS_LOG_COMPONENT_DEFINE ("BNSMincastMessages");

namespace bns {

ns3::TypeId
MincastTypeHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("MincastTypeHeader")
	.SetParent<Header> ()
	.AddConstructor<MincastTypeHeader> ();
    return tid;
}


ns3::TypeId
MincastTypeHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
MincastTypeHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return TYPE_SIZE;
}


void 
MincastTypeHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteU8 (m_type);
}


uint32_t 
MincastTypeHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_type = start.ReadU8 ();

    return TYPE_SIZE; // the number of bytes consumed.
}


void 
MincastTypeHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "type=" << m_type;
}

void 
MincastTypeHeader::SetType (uint8_t type)
{
    NS_LOG_FUNCTION(this);
    m_type = type;
}


uint8_t 
MincastTypeHeader::GetType (void) const
{
    NS_LOG_FUNCTION(this);
    return m_type;
}


ns3::TypeId
MincastPingHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("MincastPingHeader")
	.SetParent<Header> ()
	.AddConstructor<MincastPingHeader> ();
    return tid;
}


ns3::TypeId
MincastPingHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
MincastPingHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return MINCAST_PING_SIZE;
}


void 
MincastPingHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU64 (m_senderID);
}


uint32_t 
MincastPingHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_senderID = start.ReadNtohU64 ();
    return MINCAST_PING_SIZE; // the number of bytes consumed.
}


void 
MincastPingHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "senderID=" << m_senderID;
}

void 
MincastPingHeader::SetSenderId (uint64_t senderID)
{
    NS_LOG_FUNCTION(this);
    m_senderID = senderID;
}


uint64_t 
MincastPingHeader::GetSenderId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_senderID;
}

ns3::TypeId
MincastFindNodeHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("MincastFindNodeHeader")
	.SetParent<Header> ()
	.AddConstructor<MincastFindNodeHeader> ();
    return tid;
}


ns3::TypeId
MincastFindNodeHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
MincastFindNodeHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return MINCAST_FINDNODE_SIZE;
}


void 
MincastFindNodeHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU64 (m_senderID);
    start.WriteHtonU64 (m_targetID);
}


uint32_t 
MincastFindNodeHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_senderID = start.ReadNtohU64 ();
    m_targetID = start.ReadNtohU64 ();
    return MINCAST_FINDNODE_SIZE; // the number of bytes consumed.
}


void 
MincastFindNodeHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "senderID=" << m_senderID << " targetID=" << m_targetID;
}

void 
MincastFindNodeHeader::SetSenderId (uint64_t senderID)
{
    NS_LOG_FUNCTION(this);
    m_senderID = senderID;
}

uint64_t 
MincastFindNodeHeader::GetSenderId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_senderID;
}

void 
MincastFindNodeHeader::SetTargetId (uint64_t targetID)
{
    NS_LOG_FUNCTION(this);
    m_targetID = targetID;
}


uint64_t 
MincastFindNodeHeader::GetTargetId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_targetID;
}

ns3::TypeId
MincastNodesHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("MincastNodesHeader")
	.SetParent<Header> ()
	.AddConstructor<MincastNodesHeader> ();
    return tid;
}


ns3::TypeId
MincastNodesHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
MincastNodesHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return MINCAST_NODES_SIZE; 
}


void 
MincastNodesHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU64 (m_senderID);
    start.WriteHtonU64 (m_targetID);
    start.WriteHtonU16 (m_nodeCount);

    uint8_t tmp_addrBuf[4];
    for (auto it : m_nodes) {
        // write node id
        start.WriteHtonU64(it.first);
        
        // serialize and write ip address
        ns3::Ipv4Address& addr = it.second;
        addr.Serialize(tmp_addrBuf);
        for (int j = 0; j < 4; ++j) start.WriteU8(tmp_addrBuf[j]); 
    }
}


uint32_t 
MincastNodesHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_senderID = start.ReadNtohU64 ();
    m_targetID = start.ReadNtohU64 ();
    m_nodeCount = start.ReadNtohU16 ();
    uint8_t tmp_addrBuf[4];
    ns3::Ipv4Address tmp_addr;
    uint64_t tmp_nodeID;
    for (int i = 0; i < m_nodeCount; ++i) {
        // read node id
        tmp_nodeID = start.ReadNtohU64();

        // read serialized ip address
        for (int j = 0; j < 4; ++j) tmp_addrBuf[j] = start.ReadU8(); 
        tmp_addr = ns3::Ipv4Address::Deserialize(tmp_addrBuf);

        // add to map
        m_nodes[tmp_nodeID] = tmp_addr;
    }
    return MINCAST_NODES_SIZE; // the number of bytes consumed.
}


void 
MincastNodesHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "senderID=" << m_senderID << " targetID=" << m_targetID;
}

void 
MincastNodesHeader::SetSenderId (uint64_t senderID)
{
    NS_LOG_FUNCTION(this);
    m_senderID = senderID;
}

uint64_t 
MincastNodesHeader::GetSenderId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_senderID;
}

void 
MincastNodesHeader::SetTargetId (uint64_t targetID)
{
    NS_LOG_FUNCTION(this);
    m_targetID = targetID;
}


uint64_t 
MincastNodesHeader::GetTargetId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_targetID;
}

void 
MincastNodesHeader::SetNodes (std::unordered_map<uint64_t, ns3::Ipv4Address> nodes)
{
    NS_LOG_FUNCTION(this);
    m_nodes = nodes;
    m_nodeCount = nodes.size();
}


std::unordered_map<uint64_t, ns3::Ipv4Address> 
MincastNodesHeader::GetNodes (void) const
{
    NS_LOG_FUNCTION(this);
    return m_nodes;
}

ns3::TypeId
MincastChunkHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("MincastChunkHeader")
	.SetParent<Header> ()
	.AddConstructor<MincastChunkHeader> ();
    return tid;
}


ns3::TypeId
MincastChunkHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
MincastChunkHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return MINCAST_BROADCAST_SIZE;
}


void 
MincastChunkHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU64 (m_senderID);

    start.WriteHtonU64 (m_blockID);
    start.WriteHtonU16 (m_chunkID);
    start.WriteHtonU64 (m_prevID);
    start.WriteHtonU32 (m_blockSize);
    start.WriteHtonU16 (m_nChunks);

    start.WriteHtonU16 (m_height);
}


uint32_t 
MincastChunkHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_senderID = start.ReadNtohU64 ();

    m_blockID = start.ReadNtohU64 ();
    m_chunkID = start.ReadNtohU16 ();
    m_prevID = start.ReadNtohU64 ();
    m_blockSize = start.ReadNtohU32 ();
    m_nChunks = start.ReadNtohU16 ();

    m_height = start.ReadNtohU16 ();
    return MINCAST_BROADCAST_SIZE; // the number of bytes consumed.
}


void 
MincastChunkHeader::Print (std::ostream &os) const
{
    os << "senderID=" << m_senderID << " blockID=" << m_blockID << " chunkID=" << m_chunkID << " blockSize=" << m_blockSize;
}

void 
MincastChunkHeader::SetSenderId (uint64_t senderID)
{
    NS_LOG_FUNCTION(this);
    m_senderID = senderID;
}

uint64_t 
MincastChunkHeader::GetSenderId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_senderID;
}

void 
MincastChunkHeader::SetBlockId (uint64_t blockID)
{
    NS_LOG_FUNCTION(this);
    m_blockID = blockID;
}

uint64_t 
MincastChunkHeader::GetBlockId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_blockID;
}

void 
MincastChunkHeader::SetChunkId (uint16_t chunkID)
{
    NS_LOG_FUNCTION(this);
    m_chunkID = chunkID;
}

uint16_t 
MincastChunkHeader::GetChunkId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_chunkID;
}

void 
MincastChunkHeader::SetPrevId (uint64_t prevID)
{
    NS_LOG_FUNCTION(this);
    m_prevID = prevID;
}

void 
MincastChunkHeader::SetBlockSize (uint32_t blockSize)
{
    NS_LOG_FUNCTION(this);
    m_blockSize = blockSize;
}

uint32_t 
MincastChunkHeader::GetBlockSize (void) const
{
    NS_LOG_FUNCTION(this);
    return m_blockSize;
}

uint16_t 
MincastChunkHeader::GetNChunks (void) const
{
    NS_LOG_FUNCTION(this);
    return m_nChunks;
}

void 
MincastChunkHeader::SetNChunks (uint16_t nChunks)
{
    NS_LOG_FUNCTION(this);
    m_nChunks = nChunks;
}


uint64_t 
MincastChunkHeader::GetPrevId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_prevID;
}

void 
MincastChunkHeader::SetHeight (uint16_t height)
{
    NS_LOG_FUNCTION(this);
    m_height = height;
}

uint16_t 
MincastChunkHeader::GetHeight (void) const
{
    NS_LOG_FUNCTION(this);
    return m_height;
}

ns3::TypeId
MincastReqHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("MincastReqHeader")
	.SetParent<Header> ()
	.AddConstructor<MincastReqHeader> ();
    return tid;
}


ns3::TypeId
MincastReqHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
MincastReqHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return MINCAST_REQUEST_SIZE;
}


void 
MincastReqHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU64 (m_senderID);
    start.WriteHtonU64 (m_blockID);
}


uint32_t 
MincastReqHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_senderID = start.ReadNtohU64 ();
    m_blockID = start.ReadNtohU64 ();

    return MINCAST_REQUEST_SIZE; // the number of bytes consumed.
}


void 
MincastReqHeader::Print (std::ostream &os) const
{
    os << "senderID=" << m_senderID << " blockID=" << m_blockID;
}

void 
MincastReqHeader::SetSenderId (uint64_t senderID)
{
    NS_LOG_FUNCTION(this);
    m_senderID = senderID;
}

uint64_t 
MincastReqHeader::GetSenderId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_senderID;
}

void 
MincastReqHeader::SetBlockId (uint64_t blockID)
{
    NS_LOG_FUNCTION(this);
    m_blockID = blockID;
}

uint64_t 
MincastReqHeader::GetBlockId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_blockID;
}

ns3::TypeId
MincastInformHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("MincastInformHeader")
	.SetParent<Header> ()
	.AddConstructor<MincastInformHeader> ();
    return tid;
}


ns3::TypeId
MincastInformHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
MincastInformHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return MINCAST_INFORM_SIZE;
}


void 
MincastInformHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU64 (m_senderID);
    start.WriteHtonU64 (m_blockID);
}


uint32_t 
MincastInformHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_senderID = start.ReadNtohU64 ();
    m_blockID = start.ReadNtohU64 ();

    return MINCAST_INFORM_SIZE; // the number of bytes consumed.
}


void 
MincastInformHeader::Print (std::ostream &os) const
{
    os << "senderID=" << m_senderID << " blockID=" << m_blockID;
}

void 
MincastInformHeader::SetSenderId (uint64_t senderID)
{
    NS_LOG_FUNCTION(this);
    m_senderID = senderID;
}

uint64_t 
MincastInformHeader::GetSenderId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_senderID;
}

void 
MincastInformHeader::SetBlockId (uint64_t blockID)
{
    NS_LOG_FUNCTION(this);
    m_blockID = blockID;
}

uint64_t 
MincastInformHeader::GetBlockId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_blockID;
}
}
