#include "string_utils.h"

#include <iostream>
#include <ctime>
#include <time.h>

namespace Cthun {
namespace Common {

std::string StringUtils::plural(int num_of_things) {
    return num_of_things > 1 ? "s" : "";
}

template<>
std::string StringUtils::plural<std::string>(std::vector<std::string> things) {
    return plural(things.size());
}

}  // namespace Common
}  // namespace Cthun
