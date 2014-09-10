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

}  // namespace StringUtils
}  // namespace Common
}  // namespace Cthun

#endif  // CTHUN_CLIENT_SRC_COMMON_STRINGUTILS_H_
