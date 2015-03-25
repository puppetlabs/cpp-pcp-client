#ifndef CTHUN_CLIENT_SRC_CONNECTOR_UUID_H_
#define CTHUN_CLIENT_SRC_CONNECTOR_UUID_H_

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <string>

// TODO(ale): move this to leatherman

namespace CthunClient {
namespace UUID {

static std::string getUUID() __attribute__((unused));

/// Return a new UUID string
static std::string getUUID() {
    static boost::uuids::random_generator gen;
    boost::uuids::uuid uuid = gen();
    return boost::uuids::to_string(uuid);
}

}  // namespace UUID
}  // namespace CthunClient


#endif  // CTHUN_CLIENT_SRC_CONNECTOR_UUID_H_
