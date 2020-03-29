#ifndef VANILLA_MESSAGES_H
#define VANILLA_MESSAGES_H

#include <unordered_map>
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"

// define field sizes for the headers
#define LEN_SIZE 4 // length
#define TYPE_SIZE 1 // type
#define VAN_INV_SIZE 4+(m_count*8) // count + 8 bytes per entry
#define VAN_GETDATA_SIZE 4+(m_count*8) // count + 8 bytes per entry
#define VAN_GETHEADERS_SIZE 8+8 // startID + stopID
#define VAN_HEADERS_SIZE 4+(m_count*8) // count + 8 bytes per entry
#define VAN_GETBLOCKS_SIZE 8+8 // startID + stopID
#define VAN_BLOCK_SIZE 8+8// blockID + prevID + blockSize

/*
 * Real sizes:
 * block: 80 + ~1-9 txn_count = X + X * txns[]
 * getblocks: 36 + ~1-9 count = X + 32 * X
 * getheaders: 36 + ~1-9 count = X + 32 * X
 */

namespace bns {

enum class VanMsgType
{
  INV,              //0
  GETDATA,         //1
  GETHEADERS,      //2
  HEADERS,          //3
  GETBLOCKS,       //4
  BLOCK,            //5
};

class VanLengthHeader : public ns3::Header
{
	public:
		static ns3::TypeId GetTypeId (void);
		virtual ns3::TypeId GetInstanceTypeId (void) const;
		virtual uint32_t GetSerializedSize (void) const;
		virtual void Serialize (ns3::Buffer::Iterator start) const;
		virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
		virtual void Print (std::ostream &os) const;

		void SetLength (uint32_t length);
		uint32_t GetLength (void) const;
	private:
		uint32_t m_length;
};

class VanTypeHeader : public ns3::Header
{
	public:
		static ns3::TypeId GetTypeId (void);
		virtual ns3::TypeId GetInstanceTypeId (void) const;
		virtual uint32_t GetSerializedSize (void) const;
		virtual void Serialize (ns3::Buffer::Iterator start) const;
		virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
		virtual void Print (std::ostream &os) const;

		void SetType (uint8_t type);
		uint8_t GetType (void) const;
	private:
		uint8_t m_type;
};

class VanInvHeader : public ns3::Header
{
	public:
		static ns3::TypeId GetTypeId (void);
		virtual ns3::TypeId GetInstanceTypeId (void) const;
		virtual uint32_t GetSerializedSize (void) const;
		virtual void Serialize (ns3::Buffer::Iterator start) const;
		virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
		virtual void Print (std::ostream &os) const;

		void SetInventory (std::vector<uint64_t> inventory);
        std::vector<uint64_t> GetInventory (void) const;
	private:
		uint32_t m_count; 
        std::vector<uint64_t> m_inventory; 
};

class VanGetDataHeader : public ns3::Header
{
	public:
		static ns3::TypeId GetTypeId (void);
		virtual ns3::TypeId GetInstanceTypeId (void) const;
		virtual uint32_t GetSerializedSize (void) const;
		virtual void Serialize (ns3::Buffer::Iterator start) const;
		virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
		virtual void Print (std::ostream &os) const;

		void SetInventory (std::vector<uint64_t> inventory);
        std::vector<uint64_t> GetInventory (void) const;
	private:
		uint32_t m_count; 
        std::vector<uint64_t> m_inventory; 
};

class VanGetHeadersHeader : public ns3::Header
{
	public:
		static ns3::TypeId GetTypeId (void);
		virtual ns3::TypeId GetInstanceTypeId (void) const;
		virtual uint32_t GetSerializedSize (void) const;
		virtual void Serialize (ns3::Buffer::Iterator start) const;
		virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
		virtual void Print (std::ostream &os) const;

		void SetStartId(uint64_t startID);
        uint64_t GetStartId (void) const;

		void SetStopId(uint64_t stopID);
        uint64_t GetStopId (void) const;

	private:
        uint64_t m_startID;
        uint64_t m_stopID;
};

class VanHeadersHeader : public ns3::Header
{
	public:
		static ns3::TypeId GetTypeId (void);
		virtual ns3::TypeId GetInstanceTypeId (void) const;
		virtual uint32_t GetSerializedSize (void) const;
		virtual void Serialize (ns3::Buffer::Iterator start) const;
		virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
		virtual void Print (std::ostream &os) const;

		void SetInventory (std::vector<uint64_t> inventory);
        std::vector<uint64_t> GetInventory (void) const;
	private:
		uint32_t m_count; 
        std::vector<uint64_t> m_inventory; 
};

class VanGetBlocksHeader : public ns3::Header
{
	public:
		static ns3::TypeId GetTypeId (void);
		virtual ns3::TypeId GetInstanceTypeId (void) const;
		virtual uint32_t GetSerializedSize (void) const;
		virtual void Serialize (ns3::Buffer::Iterator start) const;
		virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
		virtual void Print (std::ostream &os) const;

		void SetStartId(uint64_t startID);
        uint64_t GetStartId (void) const;

		void SetStopId(uint64_t stopID);
        uint64_t GetStopId (void) const;

	private:
        uint64_t m_startID;
        uint64_t m_stopID;
};

class VanBlockHeader : public ns3::Header
{
	public:
		static ns3::TypeId GetTypeId (void);
		virtual ns3::TypeId GetInstanceTypeId (void) const;
		virtual uint32_t GetSerializedSize (void) const;
		virtual void Serialize (ns3::Buffer::Iterator start) const;
		virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
		virtual void Print (std::ostream &os) const;

		void SetBlockId (uint64_t blockID);
		uint64_t GetBlockId (void) const;

		void SetPrevId (uint64_t prevID);
		uint64_t GetPrevId (void) const;

	private:
		uint64_t m_blockID;
        uint64_t m_prevID;
};
}
#endif
