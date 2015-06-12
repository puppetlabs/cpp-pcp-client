#include <cthun-client/protocol/schemas.hpp>

namespace CthunClient {
namespace Protocol {

// HERE(ale): this must be kept up to date with
// https://github.com/puppetlabs/cthun-specifications

//
// envelope
//

Schema EnvelopeSchema() {
    Schema schema { ENVELOPE_SCHEMA_NAME, ContentType::Json };
    schema.addConstraint("id", TypeConstraint::String, true);
    schema.addConstraint("message_type", TypeConstraint::String, true);
    schema.addConstraint("expires", TypeConstraint::String, true);
    // TODO(ale): add array item constraint once implemented in Schema
    schema.addConstraint("targets", TypeConstraint::Array, true);
    schema.addConstraint("sender", TypeConstraint::String, true);
    schema.addConstraint("destination_report", TypeConstraint::Bool, false);
    return schema;
}

//
// data - protocol layer
//

Schema AssociateResponseSchema() {
    Schema schema { ASSOCIATE_RESP_TYPE, ContentType::Json };
    schema.addConstraint("id", TypeConstraint::String, true);
    schema.addConstraint("success", TypeConstraint::Bool, true);
    schema.addConstraint("reason", TypeConstraint::String, false);
    return schema;
}

Schema InventoryRequestSchema() {
    Schema schema { INVENTORY_REQ_TYPE, ContentType::Json };
    schema.addConstraint("query", TypeConstraint::String, true);
    return schema;
}

Schema InventoryResponseSchema() {
    Schema schema { INVENTORY_RESP_TYPE, ContentType::Json };
    // TODO(ale): add array item constraint once implemented in Schema
    schema.addConstraint("uris", TypeConstraint::Array, true);
    return schema;
}

Schema ErrorMessageSchema() {
    Schema schema { ERROR_MSG_TYPE, ContentType::Json };
    schema.addConstraint("description", TypeConstraint::String, true);
    schema.addConstraint("id", TypeConstraint::String, false);
    return schema;
}

Schema DestinationReportSchema() {
    Schema schema { DESTINATION_REPORT_TYPE, ContentType::Json };
    schema.addConstraint("id", TypeConstraint::String, true);
    // TODO(ale): add array item constraint once implemented in Schema
    schema.addConstraint("targets", TypeConstraint::Array, true);
    return schema;
}

//
// data - RPC layer
//

Schema BlockingRequestSchema() {
    Schema schema { BLOCKING_REQUEST_TYPE, ContentType::Json };
    // NB: additionalProperties = false
    schema.addConstraint("transaction_id", TypeConstraint::String, true);
    schema.addConstraint("module", TypeConstraint::String, true);
    schema.addConstraint("action", TypeConstraint::String, true);
    schema.addConstraint("params", TypeConstraint::Object, false);
    return schema;
}

Schema BlockingResponseSchema() {
    Schema schema { BLOCKING_RESPONSE_TYPE, ContentType::Json };
    // NB: additionalProperties = true
    schema.addConstraint("transaction_id", TypeConstraint::String, true);
    return schema;
}

Schema NonBlockingRequestSchema() {
    Schema schema { NON_BLOCKING_REQUEST_TYPE, ContentType::Json };
    // NB: additionalProperties = false
    schema.addConstraint("transaction_id", TypeConstraint::String, true);
    schema.addConstraint("notify_outcome", TypeConstraint::Bool, true);
    schema.addConstraint("module", TypeConstraint::String, true);
    schema.addConstraint("action", TypeConstraint::String, true);
    schema.addConstraint("params", TypeConstraint::Object, false);
    return schema;
}

Schema NonBlockingResponseSchema() {
    Schema schema { NON_BLOCKING_RESPONSE_TYPE, ContentType::Json };
    // NB: additionalProperties = true
    schema.addConstraint("transaction_id", TypeConstraint::String, true);
    schema.addConstraint("job_id", TypeConstraint::String, true);
    return schema;
}

Schema ProvisionalResponseSchema() {
    Schema schema { PROVISIONAL_RESPONSE_TYPE, ContentType::Json };
    // NB: additionalProperties = false
    schema.addConstraint("transaction_id", TypeConstraint::String, true);
    schema.addConstraint("success", TypeConstraint::Bool, true);
    schema.addConstraint("job_id", TypeConstraint::String, false);
    schema.addConstraint("error", TypeConstraint::String, false);
    return schema;
}

Schema RPCErrorSchema() {
    Schema schema { DEBUG_ITEM_SCHEMA_NAME, ContentType::Json };
    // NB: additionalProperties = false
    schema.addConstraint("transaction_id", TypeConstraint::String, true);
    schema.addConstraint("id", TypeConstraint::String, true);
    schema.addConstraint("description", TypeConstraint::String, true);
    return schema;
}

//
// debug
//

Schema DebugSchema() {
    Schema schema { DEBUG_SCHEMA_NAME, ContentType::Json };
    schema.addConstraint("hops", TypeConstraint::Array, true);
    return schema;
}

Schema DebugItemSchema() {
    Schema schema { DEBUG_ITEM_SCHEMA_NAME, ContentType::Json };
    schema.addConstraint("server", TypeConstraint::String, true);
    schema.addConstraint("time", TypeConstraint::String, true);
    schema.addConstraint("stage", TypeConstraint::String, false);
    return schema;
}

}  // namespace Protocol
}  // namespace CthunClient
