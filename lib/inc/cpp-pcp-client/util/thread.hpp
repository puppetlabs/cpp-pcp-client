#ifndef CPP_PCP_CLIENT_SRC_UTIL_THREAD_HPP_
#define CPP_PCP_CLIENT_SRC_UTIL_THREAD_HPP_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <boost/thread/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/throw_exception.hpp>
#pragma GCC diagnostic pop

/* This header encapsulates our use of threads and locking structures.
   During PCP-53 we were forced to switch from using std::thread to boost::thread.
   This encapsulation means that we can swtich back by changing boost::thread, etc
   to std::thread, etc here and leave the rest of the code untouched.

   PCP-582 modifies this further to include exception_ptr, which libstdc++ doesn't
   support on all architectures.
*/

namespace PCPClient {
namespace Util {

using thread = boost::thread;
using mutex = boost::mutex;
using condition_variable = boost::condition_variable;
const boost::defer_lock_t defer_lock {};

template <class T>
using lock_guard = boost::lock_guard<T>;

template <class T>
using unique_lock = boost::unique_lock<T>;

namespace this_thread = boost::this_thread;

using exception_ptr = boost::exception_ptr;

}  // namespace Util
}  // namespace PCPClient

#endif  // CPP_PCP_CLIENT_SRC_UTIL_THREAD_HPP_
