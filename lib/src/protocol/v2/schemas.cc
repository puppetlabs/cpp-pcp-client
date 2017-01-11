#include <cpp-pcp-client/protocol/v2/schemas.hpp>

namespace PCPClient {
namespace v2 {
namespace Protocol {

// HERE(ale): this must be kept up to date with
// https://github.com/puppetlabs/pcp-specifications

Schema EnvelopeSchema() {
    Schema schema { ENVELOPE_SCHEMA_NAME, ContentType::Json };
    schema.addConstraint("id", TypeConstraint::String, true);
    schema.addConstraint("message_type", TypeConstraint::String, true);
    schema.addConstraint("target", TypeConstraint::String, false);
    schema.addConstraint("sender", TypeConstraint::String, false);
    schema.addConstraint("in_reply_to", TypeConstraint::String, false);
    schema.addConstraint("data", TypeConstraint::Any, false);
    return schema;
}

Schema InventoryRequestSchema() {
    Schema schema { INVENTORY_REQ_TYPE, ContentType::Json };
    // TODO(ale): add array item constraint once implemented in Schema
    schema.addConstraint("query", TypeConstraint::Array, true);
    schema.addConstraint("subscribe", TypeConstraint::Bool, false);
    return schema;
}

Schema InventoryResponseSchema() {
    Schema schema { INVENTORY_RESP_TYPE, ContentType::Json };
    // TODO(ale): add array item constraint once implemented in Schema
    schema.addConstraint("uris", TypeConstraint::Array, true);
    return schema;
}

Schema ErrorMessageSchema() {
    Schema schema { ERROR_MSG_TYPE, ContentType::Json, TypeConstraint::String };
    return schema;
}

}  // namespace Protocol
}  // namespace v2
}  // namespace PCPClient
