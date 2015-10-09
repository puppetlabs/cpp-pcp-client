#ifndef CPP_PCP_CLIENT_SRC_UTIL_CHRONO_HPP_
#define CPP_PCP_CLIENT_SRC_UTIL_CHRONO_HPP_

#include <boost/chrono/chrono.hpp>

/* This header encapsulates our use of chrono. This exists to support the changes
   that had to be made during PCP-53 where we were forced to switch from using
   std::thread to boost::thread. This encapsulation means that we can swtich back
   by changing boost::chrono to std::chrono.
*/

namespace PCPClient {
namespace Util {

namespace chrono = boost::chrono;

}  // namespace Util
}  // namespace PCPClient

#endif  // CPP_PCP_CLIENT_SRC_UTIL_CHRONO_HPP_
