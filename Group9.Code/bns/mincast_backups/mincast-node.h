#ifndef MINCAST_NODE_H
#define MINCAST_NODE_H

#include <unordered_map>
#include <deque>
#include <algorithm>

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/boolean.h"
#include "ns3/random-variable-stream.h"

#include "bitcoin-node.h"
#include "mincast-messages.h"

#define MIN_PORT 8334
#define MIN_MAXCONN_OUT 8
#define MIN_MAXCONN_IN 117

namespace std {
template <>
struct hash<ns3::Ipv4Address>
{
    size_t operator()(const ns3::Ipv4Address& addr) const
    {
        return hash<uint32_t>()(addr.Get());
    }
};
}

namespace bns {

class Address;
class Socket;
class Packet;

enum class PeerType { IN, OUT };
enum class BroadcastType { UNSOLICITED, SENDHEADERS, INV };

struct Peer
{
    ns3::Ipv4Address address;
    ns3::Ptr<ns3::Socket> socket;
    PeerType type;
};

class MincastNode : public BitcoinNode
{
    public:
        MincastNode (ns3::Ipv4Address address, bool isMiner, double hashRate);

        virtual ~MincastNode (void);

        static BroadcastType minBroadcastType;
    protected:
        virtual void DoDispose (void);           // inherited from Application base class.

        virtual void StartApplication (void);    // Called at time specified by Start
        virtual void StopApplication (void);     // Called at time specified by Stop

		/**
		 * \brief Setup the listenSocket, start listening and register the required handlers.
		 */
		void InitListenSocket (void);

		/** 
		 * \brief Connect to other peers.
		 */
		void InitOutgoingConnection(void);

        /**
         * \brief Initialize a broadcast operation
         */
        void InitBroadcast (Block& b);

		/**
		 * \brief Returns a random known address
		 */
		ns3::Ipv4Address const RandomKnownAddress();

		/**
		 * \brief Add the packet to the send queue and send
		 * \param packet the packet to send
		 */
		void SendPacket (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet);

        /**
         * \brief Sends the available number of bytes to the socket.
         * Called by SendPacket and HandleSend
         */
        void SendAvailable (ns3::Ptr<ns3::Socket> socketPtr);

        /** 
         * \brief Send a INV message
         */
        void SendInvMessage (ns3::Ptr<ns3::Socket> socketPtr, std::vector<uint64_t> inventory);

        /** 
         * \brief Send a GETHEADERS message
         */
        void SendGetHeadersMessage (ns3::Ptr<ns3::Socket> socketPtr, uint64_t startID, uint64_t stopID);

        /** 
         * \brief Send a HEADERS message
         */
        void SendHeadersMessage (ns3::Ptr<ns3::Socket> socketPtr, std::vector<uint64_t> inventory);

        /** 
         * \brief Send a GETDATA message
         */
        void SendGetDataMessage (ns3::Ptr<ns3::Socket> socketPtr, std::vector<uint64_t> inventory);

        /** 
         * \brief Send a BLOCK message
         */
        void SendBlockMessage (ns3::Ptr<ns3::Socket> socketPtr, Block b);


		/**
		 * \brief Handle an incoming connection
		 * \param socketPtr the incoming connection socketPtr
		 * \param from the address the connection is from
		 */
		void HandleAccept (ns3::Ptr<ns3::Socket> socketPtr, const ns3::Address& from);

		/**
		 * \brief Handle a successful outgoing connection
		 * \param socketPtr the outgoing connection socketPtr
		 */
		void HandleConnect (ns3::Ptr<ns3::Socket> socketPtr);


        /**
         * \brief Callback which is called when space is available in send buffer
         * \param socketPtr the sending socketPtr
         * \param avail_bytes the number of bytes available
         */
        void HandleSent (ns3::Ptr<ns3::Socket> socketPtr, uint32_t avail_bytes);

        /**
         * \brief Handle data which can be received on a socket
         * \param socketPtr the receiving socketPtr
         */
        void HandleRead (ns3::Ptr<ns3::Socket> socketPtr);

        /**
         * \brief Handle an connection close
         * \param socketPtr the connected socketPtr
         */
        void HandlePeerClose (ns3::Ptr<ns3::Socket> socketPtr);

        /**
         * \brief Handle an connection error
         * \param socketPtr the connected socketPtr
         */
        void HandlePeerError (ns3::Ptr<ns3::Socket> socketPtr);

        /** 
         * \brief Handle a received packet
         */
        void HandlePacket (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet);

        /** 
         * \brief Process a received packet
         */
        void ProcessPacket (ns3::Ptr<ns3::Socket> socketPtr);

        /**
         * \brief Handle an INV message
         */
        void HandleInvMessage (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet);

        /**
         * \brief Handle a GETDATA message
         */
        void HandleGetDataMessage (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet);

        /**
         * \brief Handle a GETHEADERS message
         */
        void HandleGetHeadersMessage (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet);

        /**
         * \brief Handle a HEADERS message
         */
        void HandleHeadersMessage (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet);

        /**
         * \brief Handle a GETBLOCKS message
         */
        void HandleGetBlocksMessage (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet);

        /**
         * \brief Handle a BLOCK message
         */
        void HandleBlockMessage (ns3::Ptr<ns3::Socket> socketPtr, ns3::Ptr<ns3::Packet> packet);

        /**
         * \brief Set if a peer already knows a block
         */
        void SetBlockKnown (ns3::Ipv4Address peerAddr, uint64_t blockID);

        /**
         * \brief Check if a peer knows a block
         */
        bool PeerKnowsBlock (ns3::Ipv4Address peerAddr, uint64_t blockID);

        std::unordered_map<ns3::Ipv4Address, Peer>      m_peers;
        uint32_t m_nInPeers;
        uint32_t m_nOutPeers;
        
        std::unordered_map<ns3::Ipv4Address, ns3::Ptr<ns3::Packet>>	m_recvQueues;

        std::unordered_map<ns3::Ipv4Address, std::deque<ns3::Ptr<ns3::Packet>>> m_sendQueues;

        std::unordered_map<uint64_t, std::vector<ns3::Ipv4Address>> m_knownBlocks;

        std::set<uint64_t> m_requestedBlocks;
};



ns3::Ipv4Address GetSocketAddress (ns3::Ptr<ns3::Socket> socketPtr);
}
#endif
