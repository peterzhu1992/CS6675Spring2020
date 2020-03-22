/**
 * This file contains declares the simple MincastNode class.
 */

#ifndef MINCAST_NODE_H
#define MINCAST_NODE_H

#include <algorithm>
#include <bitset>
#include <set>
#include <unordered_map>
#include <deque>
#include <mutex>
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/boolean.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"

#include "bitcoin-node.h"
#include "mincast-messages.h"
#include "util.h"

#define MINCAST_ID_LEN 64 // FIXME: Using node ID length of 64 bits for now, since it fits a uint64_t distance variable.
#define MINCAST_PING_TIMEOUT 10.0
#define MINCAST_BUCKET_REFRESH_TIMEOUT 3600.0
#define MINCAST_PORT 8334
#define MINCAST_PACKET_SIZE 1433

namespace bns {

class Address;
class Socket;
class Packet;

typedef std::bitset<64> nodeid_t;
typedef std::pair<ns3::Ipv4Address, nodeid_t> bentry_t;

struct MinChunk
{
    uint16_t chunkID;
    uint64_t blockID;
    uint64_t prevID;
    uint32_t blockSize;
    uint16_t chunkSize;
    uint32_t blockHeight;
    uint16_t nChunks;
};


class MincastNode : public BitcoinNode
{
    public:
        MincastNode (ns3::Ipv4Address address, bool isMiner, double hashRate);

        virtual ~MincastNode (void);


        /**
         * \brief Schedule Broadcast
         */
        //void ScheduleBroadcast(int context, ns3::Time time, uint64_t blockID);

        static uint16_t kadK;
        static uint16_t kadAlpha;
        static uint16_t kadBeta;
        static double kadFecOverhead;

    protected:
        virtual void DoDispose (void);           // inherited from Application base class.

        virtual void StartApplication (void);    // Called at time specified by Start
        virtual void StopApplication (void);     // Called at time specified by Stop


        /**
         * \brief Generate a new (pseudo-)random node ID
         */
        nodeid_t GenerateNodeID ();

        /**
         * \brief Generate a random ID in given interval.
         */
        nodeid_t RandomIDInInterval (uint64_t min, uint64_t max);

        /**
         * \brief Returns a random address from a bucket.
         */
        ns3::Ipv4Address RandomAddressFromBucket (short i);

        /**
         * \brief Calculate the distance between two IDs
         */
        uint64_t Distance (nodeid_t node1, nodeid_t node2);

        /**
         * \brief Calculate the appropriate bucket index for a nodeid_t
         */
        uint16_t BucketIndexFromID (nodeid_t node);

        /**
         * \brief Update the appropriate k-bucket when we see a node
         */
        void UpdateBucket (ns3::Ipv4Address address, nodeid_t node);

        /**
         * \brief Handle a packet received by the application
         * \param socket the receiving socket
         */
        void HandleRead (ns3::Ptr<ns3::Socket> socket);
        void HandleSent (ns3::Ptr<ns3::Socket> socketPtr, uint32_t availBytes);
        void SendAvailable();

        /**
         * \brief Handle a received ping message.
         */
        void HandlePingMessage(ns3::Ipv4Address &addr, nodeid_t &senderID);

        /**
         * \brief Handle a received pong message.
         */
        void HandlePongMessage(ns3::Ipv4Address &addr, nodeid_t &senderID);

        /**
         * \brief Handle a received find_node message.
         */
        void HandleFindNodeMessage(ns3::Ipv4Address &sender, nodeid_t &senderID, nodeid_t &targetID);

        /**
         * \brief Handle a received nodes message.
         */
        void HandleNodesMessage(ns3::Ipv4Address &sender, nodeid_t& senderID, nodeid_t& targetID, std::vector<bentry_t> &nodes);

        /**
         * \brief Handle a received broadcast message.
         */
        void HandleChunkMessage(ns3::Ipv4Address &senderAddr, nodeid_t& senderID, MinChunk c, uint16_t height);

        /**
         * \brief Handle a received block request message.
         */
        void HandleRequestMessage(ns3::Ipv4Address &senderAddr, nodeid_t& senderID, uint64_t blockID);

        /**
         * \brief Handle a inform block request message.
         */
        void HandleInformMessage(ns3::Ipv4Address &senderAddr, nodeid_t& senderID, uint64_t blockID);

        /** 
         * \brief Send a ping message to a node
         */
        void SendPingMessage(ns3::Ipv4Address &outgoingAddress);

        /** 
         * \brief Send a pong message to a node
         */
        void SendPongMessage(ns3::Ipv4Address &outgoingAddress);

        /** 
         * \brief Send a find_node message to a node
         */
        void SendFindNodeMessage(ns3::Ipv4Address &outgoingAddress, nodeid_t& targetID);

        /**
         * \brief Send a nodes message containing the k closest new nodes
         */
        void SendNodesMessage(ns3::Ipv4Address &outgoingAddress, nodeid_t& targetID, std::vector<bentry_t> &nodes);

        /**
         * \brief Send a broadcast message
         */
        void SendChunkMessage(ns3::Ipv4Address &outgoingAddress, MinChunk c, uint16_t height);

        /**
         * \brief Send a block request message
         */
        void SendRequestMessage(ns3::Ipv4Address &outgoingAddress, uint64_t blockID);

        /**
         * \brief Send a block inform message
         */
        void SendInformMessage(ns3::Ipv4Address &outgoingAddress, uint64_t blockID);

        /**
         * \brief Initializ a node lookup
         */
        void InitLookupNode (nodeid_t &targetID);

        /**
         * \brief Lookup a node based on the current state of m_nodeLookups map
         */
        void LookupNode (nodeid_t &targetID, bool queryAll = false);

        /**
         * \brief Initialize a broadcast operation
         */
        void InitBroadcast (Block& b);

        /**
         * \brief Broadcast a chunk in the subtree of height
         */
        void BroadcastBlock (Block& b);


        /**
         * \brief Actually chunkify and send a block
         */
        void SendBlock (ns3::Ipv4Address &outgoingAddress, Block& b, uint16_t height);

        /**
         * \brief Refresh a known node
         */
        void RefreshNode(ns3::Ipv4Address &oldAddress, nodeid_t oldID, ns3::Ipv4Address &newAddress, nodeid_t newID);

        /**
         * \brief Is called when a kademlia ping message is expired.
         */
        void RefreshTimeoutExpired (ns3::Ipv4Address &addr, nodeid_t node);

        /**
         * \brief Periodically refresh buckets (every KAD_BUCKET_REFRESH_TIMEOUT seconds)
         */
        void PeriodicRefresh();

        void RequestMissingBlock(ns3::Ipv4Address& senderAddr, uint64_t blockID);

        /**
         * \brief Refresh all buckets
         */
        void RefreshBuckets();

        /**
         * \brief Creates a fresh bucket or returns the existing one.
         */
        std::unordered_map<uint16_t, std::vector<bentry_t>>::iterator InitBucket (uint16_t i, std::unordered_map<uint16_t, std::vector<bentry_t>> &bm);

        /**
         * \brief Find a node in a bucket
         */
        std::vector<bentry_t>::iterator FindInBucket (ns3::Ipv4Address &addr, std::vector<bentry_t> &bucket);

        /**
         * \brief Find pending node refresh data
         */
        std::unordered_map<nodeid_t, std::tuple<ns3::EventId, ns3::Ipv4Address, nodeid_t> >::iterator FindInRefreshes (nodeid_t nodeID);

        /**
         * \brief Encode a node id for transmission.
         */
        uint64_t EncodeID (std::bitset<MINCAST_ID_LEN> &bs);

        /**
         * \brief Decode a trasmitted node ID.
         */
        std::bitset<MINCAST_ID_LEN> DecodeID (uint64_t encoded);

        /**
         * \brief Find the K closest nodes to a given node id
         */
        std::map<uint64_t, bentry_t> FindKClosestNodes (nodeid_t targetID);

        /**
         * \brief Mark that a node has already been queried for a specified find_node targetID
         */
        void MarkQueried (nodeid_t& targetID, nodeid_t& nodeID);

        /**
         * \brief Check if a node has already been queried for a targetID
         */
        bool IsQueried (nodeid_t& targetID, nodeid_t& nodeID);

        /**
         * \brief Terminate lookup
         */
        void TerminateLookup(nodeid_t& targetID);

        void PrintBuckets();

        /**
         * \brief Create chunks out of blocks.
         */
        std::map<uint16_t, MinChunk> Chunkify (Block b);

        /**
         * \brief Create blocks from chunks.
         */
        Block Dechunkify (std::map<uint16_t, MinChunk> chunks);

        nodeid_t                                             m_nodeID;                         //!< The Kademlia node id of the Mincast peer.
        std::unordered_map<uint16_t, std::vector<bentry_t>>                                           m_buckets; //!< The k-buckets (one for every 0 =< i =< MINCAST_ID_LEN)
        std::unordered_map<nodeid_t, std::tuple<ns3::EventId, ns3::Ipv4Address, nodeid_t> > m_pendingRefreshes; //!< Pending refreshed nodes.
        std::unordered_map<nodeid_t, std::map<uint64_t, std::tuple<ns3::Ipv4Address, nodeid_t, bool>>> m_nodeLookups; //!< Lists all running node lookups, currently known k closest nodes by distance, and if they were queried

        std::unordered_map<uint64_t, std::map<uint32_t, bool>> m_seenBroadcasts;
        std::unordered_map<uint64_t, std::map<uint16_t, MinChunk>> m_receivedChunks;
        std::unordered_map<uint64_t, uint16_t> m_maxSeenHeight;
        std::unordered_map<uint64_t, bool> m_doneBlocks;

        std::unordered_map<uint64_t, bool> m_requestedBlocks;

        std::deque<std::pair<ns3::Ipv4Address, ns3::Ptr<ns3::Packet>>> m_sendQueue;

        std::set<uint16_t> m_activeBuckets;

        bool m_sending;
        ns3::EventId m_nextSend;
};

}
#endif /* MINCAST_NODE_H */
