#ifndef MINCAST_MESSAGES_H
#define MINCAST_MESSAGES_H

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
#define LEN_SIZE 4																						 // length
#define TYPE_SIZE 1																						 // type
#define MINCAST_PING_SIZE 8																		 // senderID
#define MINCAST_FINDNODE_SIZE 8 + 8														 // senderID + targetID
#define MINCAST_NODES_SIZE 8 + 8 + 2 + (m_nodeCount * (8 + 4)) // senderID + targetID + nodeCount + 12 byte per node
#define MINCAST_BROADCAST_SIZE 8 + 8 + 2 + 8 + 4 + 2 + 2			 // senderID+blockID+chunkID+prevID+blockSize+nChunks+height
#define MINCAST_REQUEST_SIZE 8 + 8														 // senderID+blockID
#define MINCAST_INFORM_SIZE 8 + 8															 // senderID+blockID
namespace bns
{

enum class MincastMsgType
{
	PING,			 //0
	PONG,			 //1
	FINDNODE,	//2
	NODES,		 //3
	BROADCAST, //4
	REQUEST,	 //5
	INFORM,		 //6
};

class MincastTypeHeader : public ns3::Header
{
public:
	static ns3::TypeId GetTypeId(void);
	virtual ns3::TypeId GetInstanceTypeId(void) const;
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Serialize(ns3::Buffer::Iterator start) const;
	virtual uint32_t Deserialize(ns3::Buffer::Iterator start);
	virtual void Print(std::ostream &os) const;

	void SetType(uint8_t type);
	uint8_t GetType(void) const;

private:
	uint8_t m_type;
};

class MincastPingHeader : public ns3::Header
{
public:
	static ns3::TypeId GetTypeId(void);
	virtual ns3::TypeId GetInstanceTypeId(void) const;
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Serialize(ns3::Buffer::Iterator start) const;
	virtual uint32_t Deserialize(ns3::Buffer::Iterator start);
	virtual void Print(std::ostream &os) const;

	void SetSenderId(uint64_t senderID);
	uint64_t GetSenderId(void) const;

private:
	uint64_t m_senderID;
};

class MincastFindNodeHeader : public ns3::Header
{
public:
	static ns3::TypeId GetTypeId(void);
	virtual ns3::TypeId GetInstanceTypeId(void) const;
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Serialize(ns3::Buffer::Iterator start) const;
	virtual uint32_t Deserialize(ns3::Buffer::Iterator start);
	virtual void Print(std::ostream &os) const;

	void SetSenderId(uint64_t senderID);
	uint64_t GetSenderId(void) const;

	void SetTargetId(uint64_t targetID);
	uint64_t GetTargetId(void) const;

private:
	uint64_t m_senderID;
	uint64_t m_targetID;
};

class MincastNodesHeader : public ns3::Header
{
public:
	static ns3::TypeId GetTypeId(void);
	virtual ns3::TypeId GetInstanceTypeId(void) const;
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Serialize(ns3::Buffer::Iterator start) const;
	virtual uint32_t Deserialize(ns3::Buffer::Iterator start);
	virtual void Print(std::ostream &os) const;

	void SetSenderId(uint64_t senderID);
	uint64_t GetSenderId(void) const;

	void SetTargetId(uint64_t targetID);
	uint64_t GetTargetId(void) const;

	void SetNodes(std::unordered_map<uint64_t, ns3::Ipv4Address> nodes);
	std::unordered_map<uint64_t, ns3::Ipv4Address> GetNodes(void) const;

private:
	uint64_t m_senderID;
	uint64_t m_targetID;
	uint16_t m_nodeCount;
	std::unordered_map<uint64_t, ns3::Ipv4Address> m_nodes;
};

class MincastChunkHeader : public ns3::Header
{
public:
	static ns3::TypeId GetTypeId(void);
	virtual ns3::TypeId GetInstanceTypeId(void) const;
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Serialize(ns3::Buffer::Iterator start) const;
	virtual uint32_t Deserialize(ns3::Buffer::Iterator start);
	virtual void Print(std::ostream &os) const;

	void SetSenderId(uint64_t senderID);
	uint64_t GetSenderId(void) const;

	void SetBlockId(uint64_t blockID);
	uint64_t GetBlockId(void) const;

	void SetChunkId(uint16_t chunkID);
	uint16_t GetChunkId(void) const;

	void SetPrevId(uint64_t prevID);
	uint64_t GetPrevId(void) const;

	void SetBlockSize(uint32_t blockSize);
	uint32_t GetBlockSize(void) const;

	void SetNChunks(uint16_t nChunks);
	uint16_t GetNChunks(void) const;

	void SetHeight(uint16_t nonce);
	uint16_t GetHeight(void) const;

private:
	uint64_t m_senderID;

	uint64_t m_blockID;
	uint16_t m_chunkID;
	uint64_t m_prevID;
	uint32_t m_blockSize;
	uint16_t m_nChunks;

	uint16_t m_height;
};

class MincastReqHeader : public ns3::Header
{
public:
	static ns3::TypeId GetTypeId(void);
	virtual ns3::TypeId GetInstanceTypeId(void) const;
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Serialize(ns3::Buffer::Iterator start) const;
	virtual uint32_t Deserialize(ns3::Buffer::Iterator start);
	virtual void Print(std::ostream &os) const;

	void SetSenderId(uint64_t senderID);
	uint64_t GetSenderId(void) const;

	void SetBlockId(uint64_t blockID);
	uint64_t GetBlockId(void) const;

private:
	uint64_t m_senderID;

	uint64_t m_blockID;
};

class MincastInformHeader : public ns3::Header
{
public:
	static ns3::TypeId GetTypeId(void);
	virtual ns3::TypeId GetInstanceTypeId(void) const;
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Serialize(ns3::Buffer::Iterator start) const;
	virtual uint32_t Deserialize(ns3::Buffer::Iterator start);
	virtual void Print(std::ostream &os) const;

	void SetSenderId(uint64_t senderID);
	uint64_t GetSenderId(void) const;

	void SetBlockId(uint64_t blockID);
	uint64_t GetBlockId(void) const;

private:
	uint64_t m_senderID;

	uint64_t m_blockID;
};
} // namespace bns
#endif
