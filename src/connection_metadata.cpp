#include "connection_metadata.h"
#include <iostream>

namespace Cthun_Test_Client {

ConnectionMetadata::ConnectionMetadata(std::string url)
    : url_ { url } {
    std::cout << "### Inside ConnectionMetadata init! Url = " << url_ << "\n";
}

}  // namespace Cthun_Test_Client
