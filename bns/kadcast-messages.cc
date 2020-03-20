#include "kadcast-messages.h"
NS_LOG_COMPONENT_DEFINE ("BNSKadcastMessages");

namespace bns {

ns3::TypeId
KadTypeHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("KadTypeHeader")
	.SetParent<Header> ()
	.AddConstructor<KadTypeHeader> ();
    return tid;
}


ns3::TypeId
KadTypeHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
KadTypeHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return TYPE_SIZE;
}


void 
KadTypeHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteU8 (m_type);
}


uint32_t 
KadTypeHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_type = start.ReadU8 ();

    return TYPE_SIZE; // the number of bytes consumed.
}


void 
KadTypeHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "type=" << m_type;
}

void 
KadTypeHeader::SetType (uint8_t type)
{
    NS_LOG_FUNCTION(this);
    m_type = type;
}


uint8_t 
KadTypeHeader::GetType (void) const
{
    NS_LOG_FUNCTION(this);
    return m_type;
}


ns3::TypeId
KadPingHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("KadPingHeader")
	.SetParent<Header> ()
	.AddConstructor<KadPingHeader> ();
    return tid;
}


ns3::TypeId
KadPingHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
KadPingHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return KAD_PING_SIZE;
}


void 
KadPingHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU64 (m_senderID);
}


uint32_t 
KadPingHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_senderID = start.ReadNtohU64 ();
    return KAD_PING_SIZE; // the number of bytes consumed.
}


void 
KadPingHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "senderID=" << m_senderID;
}

void 
KadPingHeader::SetSenderId (uint64_t senderID)
{
    NS_LOG_FUNCTION(this);
    m_senderID = senderID;
}


uint64_t 
KadPingHeader::GetSenderId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_senderID;
}

ns3::TypeId
KadFindNodeHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("KadFindNodeHeader")
	.SetParent<Header> ()
	.AddConstructor<KadFindNodeHeader> ();
    return tid;
}


ns3::TypeId
KadFindNodeHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
KadFindNodeHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return KAD_FINDNODE_SIZE;
}


void 
KadFindNodeHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU64 (m_senderID);
    start.WriteHtonU64 (m_targetID);
}


uint32_t 
KadFindNodeHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_senderID = start.ReadNtohU64 ();
    m_targetID = start.ReadNtohU64 ();
    return KAD_FINDNODE_SIZE; // the number of bytes consumed.
}


void 
KadFindNodeHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "senderID=" << m_senderID << " targetID=" << m_targetID;
}

void 
KadFindNodeHeader::SetSenderId (uint64_t senderID)
{
    NS_LOG_FUNCTION(this);
    m_senderID = senderID;
}

uint64_t 
KadFindNodeHeader::GetSenderId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_senderID;
}

void 
KadFindNodeHeader::SetTargetId (uint64_t targetID)
{
    NS_LOG_FUNCTION(this);
    m_targetID = targetID;
}


uint64_t 
KadFindNodeHeader::GetTargetId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_targetID;
}

ns3::TypeId
KadNodesHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("KadNodesHeader")
	.SetParent<Header> ()
	.AddConstructor<KadNodesHeader> ();
    return tid;
}


ns3::TypeId
KadNodesHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
KadNodesHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return KAD_NODES_SIZE; 
}


void 
KadNodesHeader::Serialize (ns3::Buffer::Iterator start) const
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
KadNodesHeader::Deserialize (ns3::Buffer::Iterator start)
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
    return KAD_NODES_SIZE; // the number of bytes consumed.
}


void 
KadNodesHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "senderID=" << m_senderID << " targetID=" << m_targetID;
}

void 
KadNodesHeader::SetSenderId (uint64_t senderID)
{
    NS_LOG_FUNCTION(this);
    m_senderID = senderID;
}

uint64_t 
KadNodesHeader::GetSenderId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_senderID;
}

void 
KadNodesHeader::SetTargetId (uint64_t targetID)
{
    NS_LOG_FUNCTION(this);
    m_targetID = targetID;
}


uint64_t 
KadNodesHeader::GetTargetId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_targetID;
}

void 
KadNodesHeader::SetNodes (std::unordered_map<uint64_t, ns3::Ipv4Address> nodes)
{
    NS_LOG_FUNCTION(this);
    m_nodes = nodes;
    m_nodeCount = nodes.size();
}


std::unordered_map<uint64_t, ns3::Ipv4Address> 
KadNodesHeader::GetNodes (void) const
{
    NS_LOG_FUNCTION(this);
    return m_nodes;
}

ns3::TypeId
KadChunkHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("KadChunkHeader")
	.SetParent<Header> ()
	.AddConstructor<KadChunkHeader> ();
    return tid;
}


ns3::TypeId
KadChunkHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
KadChunkHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return KAD_BROADCAST_SIZE;
}


void 
KadChunkHeader::Serialize (ns3::Buffer::Iterator start) const
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
KadChunkHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_senderID = start.ReadNtohU64 ();

    m_blockID = start.ReadNtohU64 ();
    m_chunkID = start.ReadNtohU16 ();
    m_prevID = start.ReadNtohU64 ();
    m_blockSize = start.ReadNtohU32 ();
    m_nChunks = start.ReadNtohU16 ();

    m_height = start.ReadNtohU16 ();
    return KAD_BROADCAST_SIZE; // the number of bytes consumed.
}


void 
KadChunkHeader::Print (std::ostream &os) const
{
    os << "senderID=" << m_senderID << " blockID=" << m_blockID << " chunkID=" << m_chunkID << " blockSize=" << m_blockSize;
}

void 
KadChunkHeader::SetSenderId (uint64_t senderID)
{
    NS_LOG_FUNCTION(this);
    m_senderID = senderID;
}

uint64_t 
KadChunkHeader::GetSenderId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_senderID;
}

void 
KadChunkHeader::SetBlockId (uint64_t blockID)
{
    NS_LOG_FUNCTION(this);
    m_blockID = blockID;
}

uint64_t 
KadChunkHeader::GetBlockId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_blockID;
}

void 
KadChunkHeader::SetChunkId (uint16_t chunkID)
{
    NS_LOG_FUNCTION(this);
    m_chunkID = chunkID;
}

uint16_t 
KadChunkHeader::GetChunkId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_chunkID;
}

void 
KadChunkHeader::SetPrevId (uint64_t prevID)
{
    NS_LOG_FUNCTION(this);
    m_prevID = prevID;
}

void 
KadChunkHeader::SetBlockSize (uint32_t blockSize)
{
    NS_LOG_FUNCTION(this);
    m_blockSize = blockSize;
}

uint32_t 
KadChunkHeader::GetBlockSize (void) const
{
    NS_LOG_FUNCTION(this);
    return m_blockSize;
}

uint16_t 
KadChunkHeader::GetNChunks (void) const
{
    NS_LOG_FUNCTION(this);
    return m_nChunks;
}

void 
KadChunkHeader::SetNChunks (uint16_t nChunks)
{
    NS_LOG_FUNCTION(this);
    m_nChunks = nChunks;
}


uint64_t 
KadChunkHeader::GetPrevId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_prevID;
}

void 
KadChunkHeader::SetHeight (uint16_t height)
{
    NS_LOG_FUNCTION(this);
    m_height = height;
}

uint16_t 
KadChunkHeader::GetHeight (void) const
{
    NS_LOG_FUNCTION(this);
    return m_height;
}

ns3::TypeId
KadReqHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("KadReqHeader")
	.SetParent<Header> ()
	.AddConstructor<KadReqHeader> ();
    return tid;
}


ns3::TypeId
KadReqHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
KadReqHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return KAD_REQUEST_SIZE;
}


void 
KadReqHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU64 (m_senderID);
    start.WriteHtonU64 (m_blockID);
}


uint32_t 
KadReqHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_senderID = start.ReadNtohU64 ();
    m_blockID = start.ReadNtohU64 ();

    return KAD_REQUEST_SIZE; // the number of bytes consumed.
}


void 
KadReqHeader::Print (std::ostream &os) const
{
    os << "senderID=" << m_senderID << " blockID=" << m_blockID;
}

void 
KadReqHeader::SetSenderId (uint64_t senderID)
{
    NS_LOG_FUNCTION(this);
    m_senderID = senderID;
}

uint64_t 
KadReqHeader::GetSenderId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_senderID;
}

void 
KadReqHeader::SetBlockId (uint64_t blockID)
{
    NS_LOG_FUNCTION(this);
    m_blockID = blockID;
}

uint64_t 
KadReqHeader::GetBlockId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_blockID;
}

}
