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

/// Erases the current line on stdout, places the cursor at the
/// beginning of it, and displays a progress message.
void displayProgress(double percent, int len,
                     std::string status = "in progress");

/// Wraps message with the POSIX cyan display code
std::string cyan(std::string const& message);

/// Wraps message with the POSIX green display code
std::string green(std::string const& message);

/// Wraps message with the POSIX yellow display code
std::string yellow(std::string const& message);

/// Wraps message with the POSIX red display code
std::string red(std::string const& message);

}  // namespace StringUtils
}  // namespace Common
}  // namespace Cthun

#endif  // CTHUN_CLIENT_SRC_COMMON_STRINGUTILS_H_
