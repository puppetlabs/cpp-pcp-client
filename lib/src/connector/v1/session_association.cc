#include <cpp-pcp-client/connector/v1/session_association.hpp>

namespace PCPClient {
namespace v1 {

SessionAssociation::SessionAssociation(uint32_t assoc_timeout_s)
        : success { false },
          in_progress { false },
          got_messaging_failure { false },
          request_id {},
          error {},
          mtx {},
          cond_var {},
          association_timeout_s { assoc_timeout_s }
{
}

void SessionAssociation::reset()
{
    success = false;
    in_progress = false;
    got_messaging_failure = false;
    request_id.clear();
    error.clear();
}

}  // namespace v1
}  // namespace PCPClient
