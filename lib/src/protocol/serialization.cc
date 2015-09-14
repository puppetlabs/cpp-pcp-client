#include <cpp-pcp-client/protocol/serialization.hpp>
#if _WIN32
#include <Winsock2.h>
#else
#include <netinet/in.h>  // endianess functions: htonl, ntohl
#endif

namespace PCPClient {

#ifdef BOOST_LITTLE_ENDIAN

uint32_t getNetworkNumber(const uint32_t& number) {
    return htonl(number);
}

uint32_t getHostNumber(const uint32_t& number) {
    return ntohl(number);
}

#endif  // BOOST_LITTLE_ENDIAN

}  // namespace PCPClient
