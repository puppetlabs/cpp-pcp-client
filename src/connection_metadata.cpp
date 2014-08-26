#include "connection_metadata.h"
#include <iostream>

namespace Cthun {

ConnectionMetadata::ConnectionMetadata(std::string url)
    : url_ { url } {
    std::cout << "### Inside ConnectionMetadata init! Url = " << url_ << "\n";
}

}  // namespace Cthun
