#include "vanilla-messages.h"
NS_LOG_COMPONENT_DEFINE ("BNSVanillaMessages");

namespace bns {

ns3::TypeId
VanLengthHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("VanLengthHeader")
	.SetParent<Header> ()
	.AddConstructor<VanLengthHeader> ();
    return tid;
}


ns3::TypeId
VanLengthHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
VanLengthHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return LEN_SIZE;
}


void 
VanLengthHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU32 (m_length);
}


uint32_t 
VanLengthHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_length = start.ReadNtohU32 ();
    return LEN_SIZE; // the number of bytes consumed.
}


void 
VanLengthHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "length=" << m_length;
}

void 
VanLengthHeader::SetLength (uint32_t length)
{
    NS_LOG_FUNCTION(this);
    m_length = length;
}


uint32_t 
VanLengthHeader::GetLength (void) const
{
    NS_LOG_FUNCTION(this);
    return m_length;
}
ns3::TypeId
VanTypeHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("VanTypeHeader")
	.SetParent<Header> ()
	.AddConstructor<VanTypeHeader> ();
    return tid;
}


ns3::TypeId
VanTypeHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
VanTypeHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return TYPE_SIZE;
}


void 
VanTypeHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteU8 (m_type);
}


uint32_t 
VanTypeHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_type = start.ReadU8 ();

    return TYPE_SIZE; // the number of bytes consumed.
}


void 
VanTypeHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "type=" << m_type;
}

void 
VanTypeHeader::SetType (uint8_t type)
{
    NS_LOG_FUNCTION(this);
    m_type = type;
}


uint8_t 
VanTypeHeader::GetType (void) const
{
    NS_LOG_FUNCTION(this);
    return m_type;
}

ns3::TypeId
VanInvHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("VanInvHeader")
	.SetParent<Header> ()
	.AddConstructor<VanInvHeader> ();
    return tid;
}


ns3::TypeId
VanInvHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
VanInvHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return VAN_INV_SIZE;
}


void 
VanInvHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU32 (m_count);

    for (uint64_t obj : m_inventory) {
        start.WriteHtonU64(obj);
    }
}


uint32_t 
VanInvHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_count = start.ReadNtohU32();

    for (uint32_t i = 0; i < m_count; ++i) {
        uint64_t obj = start.ReadNtohU64();
        m_inventory.push_back(obj);
    }
    return VAN_INV_SIZE;
}


void 
VanInvHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "count: " << m_count;
}

void 
VanInvHeader::SetInventory (std::vector<uint64_t> inventory)
{
    NS_LOG_FUNCTION(this);
    m_inventory = inventory;
    m_count = inventory.size();
}

std::vector<uint64_t> 
VanInvHeader::GetInventory (void) const
{
    NS_LOG_FUNCTION(this);
    return m_inventory;
}


ns3::TypeId
VanGetDataHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("VanGetDataHeader")
	.SetParent<Header> ()
	.AddConstructor<VanGetDataHeader> ();
    return tid;
}


ns3::TypeId
VanGetDataHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
VanGetDataHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return VAN_GETDATA_SIZE;
}


void 
VanGetDataHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU32 (m_count);

    for (uint64_t obj : m_inventory) {
        start.WriteHtonU64(obj);
    }
}


uint32_t 
VanGetDataHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_count = start.ReadNtohU32();

    for (uint32_t i = 0; i < m_count; ++i) {
        uint64_t obj = start.ReadNtohU64();
        m_inventory.push_back(obj);
    }
    return VAN_GETDATA_SIZE;
}


void 
VanGetDataHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "count: " << m_count;
}

void 
VanGetDataHeader::SetInventory (std::vector<uint64_t> inventory)
{
    NS_LOG_FUNCTION(this);
    m_inventory = inventory;
    m_count = inventory.size();
}

std::vector<uint64_t> 
VanGetDataHeader::GetInventory (void) const
{
    NS_LOG_FUNCTION(this);
    return m_inventory;
}


ns3::TypeId
VanGetHeadersHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("VanGetHeadersHeader")
	.SetParent<Header> ()
	.AddConstructor<VanGetHeadersHeader> ();
    return tid;
}


ns3::TypeId
VanGetHeadersHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
VanGetHeadersHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return VAN_GETHEADERS_SIZE;
}


void 
VanGetHeadersHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU64 (m_startID);
    start.WriteHtonU64 (m_stopID);
}


uint32_t 
VanGetHeadersHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_startID = start.ReadNtohU64();
    m_stopID = start.ReadNtohU64();

    return VAN_GETHEADERS_SIZE;
}

void 
VanGetHeadersHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "startID: " << m_startID << ", stopID: " << m_stopID;
}

void 
VanGetHeadersHeader::SetStartId (uint64_t startID)
{
    NS_LOG_FUNCTION(this);
    m_startID = startID;
}

uint64_t 
VanGetHeadersHeader::GetStartId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_startID;
}

void 
VanGetHeadersHeader::SetStopId (uint64_t stopID)
{
    NS_LOG_FUNCTION(this);
    m_stopID = stopID;
}

uint64_t 
VanGetHeadersHeader::GetStopId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_stopID;
}

ns3::TypeId
VanHeadersHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("VanHeadersHeader")
	.SetParent<Header> ()
	.AddConstructor<VanHeadersHeader> ();
    return tid;
}


ns3::TypeId
VanHeadersHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
VanHeadersHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return VAN_HEADERS_SIZE;
}


void 
VanHeadersHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU32 (m_count);

    for (uint64_t obj : m_inventory) {
        start.WriteHtonU64(obj);
    }
}


uint32_t 
VanHeadersHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_count = start.ReadNtohU32();

    for (uint32_t i = 0; i < m_count; ++i) {
        uint64_t obj = start.ReadNtohU64();
        m_inventory.push_back(obj);
    }
    return VAN_HEADERS_SIZE;
}


void 
VanHeadersHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "count: " << m_count;
}

void 
VanHeadersHeader::SetInventory (std::vector<uint64_t> inventory)
{
    NS_LOG_FUNCTION(this);
    m_inventory = inventory;
    m_count = inventory.size();
}

std::vector<uint64_t> 
VanHeadersHeader::GetInventory (void) const
{
    NS_LOG_FUNCTION(this);
    return m_inventory;
}

ns3::TypeId
VanGetBlocksHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("VanGetBlocksHeader")
	.SetParent<Header> ()
	.AddConstructor<VanGetBlocksHeader> ();
    return tid;
}


ns3::TypeId
VanGetBlocksHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
VanGetBlocksHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return VAN_GETBLOCKS_SIZE;
}


void 
VanGetBlocksHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU64 (m_startID);
    start.WriteHtonU64 (m_stopID);
}


uint32_t 
VanGetBlocksHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_startID = start.ReadNtohU64();
    m_stopID = start.ReadNtohU64();

    return VAN_GETBLOCKS_SIZE;
}

void 
VanGetBlocksHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "startID: " << m_startID << ", stopID: " << m_stopID;
}

void 
VanGetBlocksHeader::SetStartId (uint64_t startID)
{
    NS_LOG_FUNCTION(this);
    m_startID = startID;
}

uint64_t 
VanGetBlocksHeader::GetStartId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_startID;
}

void 
VanGetBlocksHeader::SetStopId (uint64_t stopID)
{
    NS_LOG_FUNCTION(this);
    m_stopID = stopID;
}

uint64_t 
VanGetBlocksHeader::GetStopId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_stopID;
}

ns3::TypeId
VanBlockHeader::GetTypeId (void)
{
    static ns3::TypeId tid = ns3::TypeId ("VanBlockHeader")
	.SetParent<Header> ()
	.AddConstructor<VanBlockHeader> ();
    return tid;
}


ns3::TypeId
VanBlockHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId ();
}


uint32_t 
VanBlockHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION(this);
    return VAN_BLOCK_SIZE;
}


void 
VanBlockHeader::Serialize (ns3::Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    // The data.
    start.WriteHtonU64 (m_blockID);
    start.WriteHtonU64 (m_prevID);
}


uint32_t 
VanBlockHeader::Deserialize (ns3::Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    m_blockID = start.ReadNtohU64 ();
    m_prevID = start.ReadNtohU64 ();
    return VAN_BLOCK_SIZE;
}


void 
VanBlockHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION(this);
    os << "blockID=" << m_blockID;
}

void 
VanBlockHeader::SetBlockId (uint64_t blockID)
{
    NS_LOG_FUNCTION(this);
    m_blockID = blockID;
}

uint64_t 
VanBlockHeader::GetBlockId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_blockID;
}

void 
VanBlockHeader::SetPrevId (uint64_t prevID)
{
    NS_LOG_FUNCTION(this);
    m_prevID = prevID;
}

uint64_t 
VanBlockHeader::GetPrevId (void) const
{
    NS_LOG_FUNCTION(this);
    return m_prevID;
}

}
