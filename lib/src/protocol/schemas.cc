#include <cpp-pcp-client/protocol/schemas.hpp>

namespace PCPClient {
namespace Protocol {

// HERE(ale): this must be kept up to date with
// https://github.com/puppetlabs/pcp-specifications

lth_jc::Schema EnvelopeSchema() {
    lth_jc::Schema schema { ENVELOPE_SCHEMA_NAME, lth_jc::ContentType::Json };
    schema.addConstraint("id", lth_jc::TypeConstraint::String, true);
    schema.addConstraint("message_type", lth_jc::TypeConstraint::String, true);
    schema.addConstraint("expires", lth_jc::TypeConstraint::String, true);
    // TODO(ale): add array item constraint once implemented in Schema
    schema.addConstraint("targets", lth_jc::TypeConstraint::Array, true);
    schema.addConstraint("sender", lth_jc::TypeConstraint::String, true);
    schema.addConstraint("destination_report", lth_jc::TypeConstraint::Bool, false);
    schema.addConstraint("in-reply-to", lth_jc::TypeConstraint::String, false);
    return schema;
}

lth_jc::Schema AssociateResponseSchema() {
    lth_jc::Schema schema { ASSOCIATE_RESP_TYPE, lth_jc::ContentType::Json };
    schema.addConstraint("id", lth_jc::TypeConstraint::String, true);
    schema.addConstraint("success", lth_jc::TypeConstraint::Bool, true);
    schema.addConstraint("reason", lth_jc::TypeConstraint::String, false);
    return schema;
}

lth_jc::Schema InventoryRequestSchema() {
    lth_jc::Schema schema { INVENTORY_REQ_TYPE, lth_jc::ContentType::Json };
    schema.addConstraint("query", lth_jc::TypeConstraint::String, true);
    return schema;
}

lth_jc::Schema InventoryResponseSchema() {
    lth_jc::Schema schema { INVENTORY_RESP_TYPE, lth_jc::ContentType::Json };
    // TODO(ale): add array item constraint once implemented in Schema
    schema.addConstraint("uris", lth_jc::TypeConstraint::Array, true);
    return schema;
}

lth_jc::Schema ErrorMessageSchema() {
    lth_jc::Schema schema { ERROR_MSG_TYPE, lth_jc::ContentType::Json };
    schema.addConstraint("description", lth_jc::TypeConstraint::String, true);
    schema.addConstraint("id", lth_jc::TypeConstraint::String, false);
    return schema;
}

lth_jc::Schema DestinationReportSchema() {
    lth_jc::Schema schema { DESTINATION_REPORT_TYPE, lth_jc::ContentType::Json };
    schema.addConstraint("id", lth_jc::TypeConstraint::String, true);
    // TODO(ale): add array item constraint once implemented in Schema
    schema.addConstraint("targets", lth_jc::TypeConstraint::Array, true);
    return schema;
}

lth_jc::Schema TTLExpiredSchema() {
    lth_jc::Schema schema { TTL_EXPIRED_TYPE, lth_jc::ContentType::Json };
    // NB: additionalProperties = false
    schema.addConstraint("id", lth_jc::TypeConstraint::String, true);
    return schema;
}

lth_jc::Schema VersionErrorSchema() {
    lth_jc::Schema schema { VERSION_ERROR_TYPE, lth_jc::ContentType::Json };
    // NB: additionalProperties = false
    schema.addConstraint("id", lth_jc::TypeConstraint::String, true);
    schema.addConstraint("target", lth_jc::TypeConstraint::String, true);
    schema.addConstraint("reason", lth_jc::TypeConstraint::String, true);
    return schema;
}

lth_jc::Schema DebugSchema() {
    lth_jc::Schema schema { DEBUG_SCHEMA_NAME, lth_jc::ContentType::Json };
    schema.addConstraint("hops", lth_jc::TypeConstraint::Array, true);
    return schema;
}

lth_jc::Schema DebugItemSchema() {
    lth_jc::Schema schema { DEBUG_ITEM_SCHEMA_NAME, lth_jc::ContentType::Json };
    schema.addConstraint("server", lth_jc::TypeConstraint::String, true);
    schema.addConstraint("time", lth_jc::TypeConstraint::String, true);
    schema.addConstraint("stage", lth_jc::TypeConstraint::String, false);
    return schema;
}

}  // namespace Protocol
}  // namespace PCPClient
