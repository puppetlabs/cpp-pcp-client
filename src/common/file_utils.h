#ifndef CTHUN_CLIENT_SRC_COMMON_FILEUTILS_H_
#define CTHUN_CLIENT_SRC_COMMON_FILEUTILS_H_

#include <string>
#include <wordexp.h>

namespace Cthun {
namespace Common {
namespace FileUtils {

// HERE(ale): copied from Pegasus
std::string expandAsDoneByShell(std::string txt) {
    // This will store the expansion outcome
    wordexp_t result;

    // Expand and check the success
    if (wordexp(txt.c_str(), &result, 0) != 0) {
        wordfree(&result);
        return "";
    }

    // Get the expanded text and free the memory
    std::string expanded_txt { result.we_wordv[0] };
    wordfree(&result);
    return expanded_txt;
}

}  // namespace FileUtils
}  // namespace Common
}  // namespace Cthun

#endif  // CTHUN_CLIENT_SRC_COMMON_FILEUTILS_H_
