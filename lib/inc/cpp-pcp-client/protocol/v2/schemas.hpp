#pragma once

#include <cpp-pcp-client/validator/schema.hpp>
#include <cpp-pcp-client/export.h>

namespace PCPClient {
namespace v2 {
namespace Protocol {

//
// envelope
//

static const std::string ENVELOPE_SCHEMA_NAME { "envelope_schema" };
LIBCPP_PCP_CLIENT_EXPORT Schema EnvelopeSchema();

//
// data
//

// inventory
static const std::string INVENTORY_REQ_TYPE  { "http://puppetlabs.com/inventory_request" };
static const std::string INVENTORY_RESP_TYPE { "http://puppetlabs.com/inventory_response" };
LIBCPP_PCP_CLIENT_EXPORT Schema InventoryRequestSchema();
LIBCPP_PCP_CLIENT_EXPORT Schema InventoryResponseSchema();

// error
static const std::string ERROR_MSG_TYPE { "http://puppetlabs.com/error_message" };
LIBCPP_PCP_CLIENT_EXPORT Schema ErrorMessageSchema();

}  // namespace Protocol
}  // namespace v2
}  // namespace PCPClient
