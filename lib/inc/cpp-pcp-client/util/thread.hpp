#ifndef CPP_PCP_CLIENT_SRC_UTIL_THREAD_HPP_
#define CPP_PCP_CLIENT_SRC_UTIL_THREAD_HPP_

#include <boost/thread/thread.hpp>

/* This header encapsulates our use of threads and locking structures.
   During PCP-53 we were forced to switch from using std::thread to boost::thread.
   This encapsulation means that we can swtich back by changing boost::thread, etc
   to std::thread, etc here and leave the rest of the code untouched.
*/

namespace PCPClient {
namespace Util {

using thread = boost::thread;
using mutex = boost::mutex;
using condition_variable = boost::condition_variable;

template <class T>
using lock_guard = boost::lock_guard<T>;

template <class T>
using unique_lock = boost::unique_lock<T>;

namespace this_thread = boost::this_thread;

}  // namespace Util
}  // namespace PCPClient

#endif  // CPP_PCP_CLIENT_SRC_UTIL_THREAD_HPP_
