#ifndef CTHUN_CLIENT_SRC_PROTOCOL_SCHEMAS_H_
#define CTHUN_CLIENT_SRC_PROTOCOL_SCHEMAS_H_

#include <cthun-client/validator/schema.hpp>

namespace CthunClient {
namespace Protocol {

//
// envelope
//

static const std::string ENVELOPE_SCHEMA_NAME { "envelope_schema" };
Schema EnvelopeSchema();

//
// data
//

// associate session
static const std::string ASSOCIATE_REQ_TYPE  { "http://puppetlabs.com/associate_request" };
static const std::string ASSOCIATE_RESP_TYPE { "http://puppetlabs.com/associate_response" };
// NB: associate requests don't have a data chunk
Schema AssociateResponseSchema();

// inventory
static const std::string INVENTORY_REQ_TYPE  { "http://puppetlabs.com/inventory_request" };
static const std::string INVENTORY_RESP_TYPE { "http://puppetlabs.com/inventory_response" };
Schema InventoryRequestSchema();
Schema InventoryResponseSchema();

// error
static const std::string ERROR_MSG_TYPE { "http://puppetlabs.com/error_message" };
Schema ErrorMessageSchema();

//
// debug
//

static const std::string DEBUG_SCHEMA_NAME { "debug_schema" };
Schema DebugSchema();

}  // namespace Protocol
}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_PROTOCOL_SCHEMAS_H_
