#ifndef UTIL_H
#define UTIL_H

#include "ns3/socket.h"

uint64_t pow2(uint64_t exp);
std::string show_errno (enum ns3::Socket::SocketErrno e);
#endif /* UTIL_H */

