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
// data - protocol layer
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

// destination report
static const std::string DESTINATION_REPORT_TYPE { "http://puppetlabs.com/destination_report" };
Schema DestinationReportSchema();

//
// data - RPC layer
//

// RPC blocking transaction
static const std::string BLOCKING_REQUEST_TYPE  { "http://puppetlabs.com/rpc_blocking_request" };
static const std::string BLOCKING_RESPONSE_TYPE { "http://puppetlabs.com/rpc_blocking_response" };
Schema BlockingRequestSchema();
Schema BlockingResponseSchema();

// RPC non blocking transaction
static const std::string NON_BLOCKING_REQUEST_TYPE  { "http://puppetlabs.com/rpc_non_blocking_request" };
static const std::string NON_BLOCKING_RESPONSE_TYPE { "http://puppetlabs.com/rpc_non_blocking_response" };
static const std::string PROVISIONAL_RESPONSE_TYPE  { "http://puppetlabs.com/rpc_provisional_response" };
Schema NonBlockingRequestSchema();
Schema NonBlockingResponseSchema();
Schema ProvisionalResponseSchema();

// RPC error
static const std::string RPC_ERROR_MSG_TYPE { "http://puppetlabs.com/rpc_error_message" };
Schema RPCErrorSchema();

//
// debug
//

static const std::string DEBUG_SCHEMA_NAME { "debug_schema" };
Schema DebugSchema();
// TODO(ale): remove this once we implement array item constraints
static const std::string DEBUG_ITEM_SCHEMA_NAME { "debug_item_schema" };
Schema DebugItemSchema();

}  // namespace Protocol
}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_PROTOCOL_SCHEMAS_H_
