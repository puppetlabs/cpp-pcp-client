#include <cpp-pcp-client/connector/session_association.hpp>

namespace PCPClient {

SessionAssociation::SessionAssociation()
        : success { false },
          in_progress { false },
          got_messaging_failure { false },
          request_id {},
          error {},
          mtx {},
          cond_var {}
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

}  // namespace PCPClient
