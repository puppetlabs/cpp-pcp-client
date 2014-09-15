#ifndef CTHUN_CLIENT_SRC_COMMON_STRINGUTILS_H_
#define CTHUN_CLIENT_SRC_COMMON_STRINGUTILS_H_

#include <string>
#include <vector>

namespace Cthun {
namespace Common {
namespace StringUtils {

/// Return the "s" string in case of more than one things,
/// an empty string otherwise.
std::string plural(int num_of_things);

template<typename T>
std::string plural(std::vector<T> things);

/// Adds the specified expiry_minutes to the current time and returns
/// the related date time string in UTC format.
/// Returns an empty string in case it fails to allocate the buffer.
std::string getExpiryDatetime(int expiry_minutes = 1);

}  // namespace StringUtils
}  // namespace Common
}  // namespace Cthun

#endif  // CTHUN_CLIENT_SRC_COMMON_STRINGUTILS_H_
