#include "mincast-messages.h"
NS_LOG_COMPONENT_DEFINE ("BNSMincastMessages");

namespace bns {

ns3::TypeId
MinLengthHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("MinLengthHeader")
	.SetParent<Header> ()
	.AddConstructor<MinLengthHeader> ();
    return tid;
}


ns3::TypeId
MinLengthHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
MinLengthHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return LEN_SIZE;
}


void 
MinLengthHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU32 (m_length);
}


uint32_t 
MinLengthHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_length = start.ReadNtohU32 ();
    return LEN_SIZE; // the number of bytes consumed.
}


void 
MinLengthHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "length=" << m_length;
}

void 
MinLengthHeader::SetLength (uint32_t length)
{
    NS_LOG_FUNCTION(this);
    m_length = length;
}


uint32_t 
MinLengthHeader::GetLength (void) const
{
    NS_LOG_FUNCTION(this);
    return m_length;
}
ns3::TypeId
MinTypeHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("MinTypeHeader")
	.SetParent<Header> ()
	.AddConstructor<MinTypeHeader> ();
    return tid;
}


ns3::TypeId
MinTypeHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
MinTypeHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return TYPE_SIZE;
}


void 
MinTypeHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteU8 (m_type);
}


uint32_t 
MinTypeHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_type = start.ReadU8 ();

    return TYPE_SIZE; // the number of bytes consumed.
}


void 
MinTypeHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "type=" << m_type;
}

void 
MinTypeHeader::SetType (uint8_t type)
{
    NS_LOG_FUNCTION(this);
    m_type = type;
}


uint8_t 
MinTypeHeader::GetType (void) const
{
    NS_LOG_FUNCTION(this);
    return m_type;
}

ns3::TypeId
MinInvHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("MinInvHeader")
	.SetParent<Header> ()
	.AddConstructor<MinInvHeader> ();
    return tid;
}


ns3::TypeId
MinInvHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
MinInvHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return MIN_INV_SIZE;
}


void 
MinInvHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU32 (m_count);

    for (uint64_t obj : m_inventory) {
        start.WriteHtonU64(obj);
    }
}


uint32_t 
MinInvHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_count = start.ReadNtohU32();

    for (uint32_t i = 0; i < m_count; ++i) {
        uint64_t obj = start.ReadNtohU64();
        m_inventory.push_back(obj);
    }
    return MIN_INV_SIZE;
}


void 
MinInvHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "count: " << m_count;
}

void 
MinInvHeader::SetInventory (std::vector<uint64_t> inventory)
{
    NS_LOG_FUNCTION(this);
    m_inventory = inventory;
    m_count = inventory.size();
}

std::vector<uint64_t> 
MinInvHeader::GetInventory (void) const
{
    NS_LOG_FUNCTION(this);
    return m_inventory;
}


ns3::TypeId
MinGetDataHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("MinGetDataHeader")
	.SetParent<Header> ()
	.AddConstructor<MinGetDataHeader> ();
    return tid;
}


ns3::TypeId
MinGetDataHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
MinGetDataHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return MIN_GETDATA_SIZE;
}


void 
MinGetDataHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU32 (m_count);

    for (uint64_t obj : m_inventory) {
        start.WriteHtonU64(obj);
    }
}


uint32_t 
MinGetDataHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_count = start.ReadNtohU32();

    for (uint32_t i = 0; i < m_count; ++i) {
        uint64_t obj = start.ReadNtohU64();
        m_inventory.push_back(obj);
    }
    return MIN_GETDATA_SIZE;
}


void 
MinGetDataHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "count: " << m_count;
}

void 
MinGetDataHeader::SetInventory (std::vector<uint64_t> inventory)
{
    NS_LOG_FUNCTION(this);
    m_inventory = inventory;
    m_count = inventory.size();
}

std::vector<uint64_t> 
MinGetDataHeader::GetInventory (void) const
{
    NS_LOG_FUNCTION(this);
    return m_inventory;
}


ns3::TypeId
MinGetHeadersHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("MinGetHeadersHeader")
	.SetParent<Header> ()
	.AddConstructor<MinGetHeadersHeader> ();
    return tid;
}


ns3::TypeId
MinGetHeadersHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
MinGetHeadersHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return MIN_GETHEADERS_SIZE;
}


void 
MinGetHeadersHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU64 (m_startID);
    start.WriteHtonU64 (m_stopID);
}


uint32_t 
MinGetHeadersHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_startID = start.ReadNtohU64();
    m_stopID = start.ReadNtohU64();

    return MIN_GETHEADERS_SIZE;
}

void 
MinGetHeadersHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "startID: " << m_startID << ", stopID: " << m_stopID;
}

void 
MinGetHeadersHeader::SetStartId (uint64_t startID)
{
    NS_LOG_FUNCTION(this);
    m_startID = startID;
}

uint64_t 
MinGetHeadersHeader::GetStartId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_startID;
}

void 
MinGetHeadersHeader::SetStopId (uint64_t stopID)
{
    NS_LOG_FUNCTION(this);
    m_stopID = stopID;
}

uint64_t 
MinGetHeadersHeader::GetStopId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_stopID;
}

ns3::TypeId
MinHeadersHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("MinHeadersHeader")
	.SetParent<Header> ()
	.AddConstructor<MinHeadersHeader> ();
    return tid;
}


ns3::TypeId
MinHeadersHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
MinHeadersHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return MIN_HEADERS_SIZE;
}


void 
MinHeadersHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU32 (m_count);

    for (uint64_t obj : m_inventory) {
        start.WriteHtonU64(obj);
    }
}


uint32_t 
MinHeadersHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_count = start.ReadNtohU32();

    for (uint32_t i = 0; i < m_count; ++i) {
        uint64_t obj = start.ReadNtohU64();
        m_inventory.push_back(obj);
    }
    return MIN_HEADERS_SIZE;
}


void 
MinHeadersHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "count: " << m_count;
}

void 
MinHeadersHeader::SetInventory (std::vector<uint64_t> inventory)
{
    NS_LOG_FUNCTION(this);
    m_inventory = inventory;
    m_count = inventory.size();
}

std::vector<uint64_t> 
MinHeadersHeader::GetInventory (void) const
{
    NS_LOG_FUNCTION(this);
    return m_inventory;
}

ns3::TypeId
MinGetBlocksHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("MinGetBlocksHeader")
	.SetParent<Header> ()
	.AddConstructor<MinGetBlocksHeader> ();
    return tid;
}


ns3::TypeId
MinGetBlocksHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
MinGetBlocksHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return MIN_GETBLOCKS_SIZE;
}


void 
MinGetBlocksHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU64 (m_startID);
    start.WriteHtonU64 (m_stopID);
}


uint32_t 
MinGetBlocksHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_startID = start.ReadNtohU64();
    m_stopID = start.ReadNtohU64();

    return MIN_GETBLOCKS_SIZE;
}

void 
MinGetBlocksHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "startID: " << m_startID << ", stopID: " << m_stopID;
}

void 
MinGetBlocksHeader::SetStartId (uint64_t startID)
{
    NS_LOG_FUNCTION(this);
    m_startID = startID;
}

uint64_t 
MinGetBlocksHeader::GetStartId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_startID;
}

void 
MinGetBlocksHeader::SetStopId (uint64_t stopID)
{
    NS_LOG_FUNCTION(this);
    m_stopID = stopID;
}

uint64_t 
MinGetBlocksHeader::GetStopId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_stopID;
}

ns3::TypeId
MinBlockHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("MinBlockHeader")
	.SetParent<Header> ()
	.AddConstructor<MinBlockHeader> ();
    return tid;
}


ns3::TypeId
MinBlockHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
MinBlockHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return MIN_BLOCK_SIZE;
}


void 
MinBlockHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU64 (m_blockID);
    start.WriteHtonU64 (m_prevID);
}


uint32_t 
MinBlockHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_blockID = start.ReadNtohU64 ();
    m_prevID = start.ReadNtohU64 ();
    return MIN_BLOCK_SIZE;
}


void 
MinBlockHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "blockID=" << m_blockID;
}

void 
MinBlockHeader::SetBlockId (uint64_t blockID)
{
    NS_LOG_FUNCTION(this);
    m_blockID = blockID;
}

uint64_t 
MinBlockHeader::GetBlockId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_blockID;
}

void 
MinBlockHeader::SetPrevId (uint64_t prevID)
{
    NS_LOG_FUNCTION(this);
    m_prevID = prevID;
}

uint64_t 
MinBlockHeader::GetPrevId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_prevID;
}

}
