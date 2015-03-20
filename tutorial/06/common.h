#include <string>

#include <cthun-client/validator/schema.h>     // Schema, ContentType

namespace Tutorial {

static const std::string AGENT_CLIENT_TYPE { "tutorial-agent" };
static const std::string CONTROLLER_CLIENT_TYPE { "tutorial-controller" };

static const std::string SERVER_URL { "wss://127.0.0.1:8090/cthun/" };

static const std::string REQUEST_SCHEMA_NAME { "http://puppetlabs.com/tutorial_request_schema" };

static const int MSG_TIMEOUT_S { 10 };

static CthunClient::Schema getRequestMessageSchema() {
    CthunClient::Schema schema { REQUEST_SCHEMA_NAME,
                                 CthunClient::ContentType::Json };
    // Add constraints to the request message schema
    using T_C = CthunClient::TypeConstraint;
    schema.addConstraint("request", T_C::Object, true);   // mandatory
    schema.addConstraint("details", T_C::String, false);  // optional
    return schema;
}

}  // namespace Tutorial
