#include "util.h"

uint64_t 
pow2(uint64_t exp)
{
    if (exp >= 64) return std::numeric_limits<uint64_t>::max();
    uint64_t base = 2ULL;
    uint64_t result = 1ULL;
    while( exp )
    {
        if ( exp & 1 )
        {
            result *= base;
        }
        exp >>= 1;
        base *= base;
    }
    return result;
}

std::string show_errno (enum ns3::Socket::SocketErrno e)
{
    switch (e) {
        case ns3::Socket::ERROR_NOTERROR: return "ERROR_NOTERROR";
        case ns3::Socket::ERROR_ISCONN: return "ERROR_ISCONN";
        case ns3::Socket::ERROR_NOTCONN: return "ERROR_NOTCONN";
        case ns3::Socket::ERROR_MSGSIZE: return "ERROR_MSGSIZE";
        case ns3::Socket::ERROR_AGAIN: return "ERROR_AGAIN";
        case ns3::Socket::ERROR_SHUTDOWN: return "ERROR_SHUTDOWN";
        case ns3::Socket::ERROR_OPNOTSUPP: return "ERROR_OPNOTSUPP";
        case ns3::Socket::ERROR_AFNOSUPPORT: return "ERROR_AFNOSUPPORT";
        case ns3::Socket::ERROR_INVAL: return "ERROR_INVAL";
        case ns3::Socket::ERROR_BADF: return "ERROR_BADF";
        case ns3::Socket::ERROR_NOROUTETOHOST: return "ERROR_NOROUTETOHOST";
        case ns3::Socket::ERROR_NODEV: return "ERROR_NODEV";
        case ns3::Socket::ERROR_ADDRNOTAVAIL: return "ERROR_ADDRNOTAVAIL";
        case ns3::Socket::ERROR_ADDRINUSE: return "ERROR_ADDRINUSE";
        case ns3::Socket::SOCKET_ERRNO_LAST: return "SOCKET_ERRNO_LAST";
        default: return "ERROR_NOTERROR";
    }
}
