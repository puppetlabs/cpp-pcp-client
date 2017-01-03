#pragma once

#include <cpp-pcp-client/validator/validator.hpp>
#include <cpp-pcp-client/export.h>

#include <cpp-pcp-client/protocol/parsed_chunks.hpp>

#include <string>
#include <vector>
#include <map>
#include <initializer_list>

namespace PCPClient {
namespace v2 {

//
// Message
//

class LIBCPP_PCP_CLIENT_EXPORT Message {
  public:
    // The default ctor is deleted since, for the PCP protocol, a
    // valid message must have an envelope.
    Message() = delete;

    // Create a new message with a given envelope.
    explicit Message(lth_jc::JsonContainer envelope);

    // Construct a Message by parsing the payload delivered
    // by the transport layer as a std::string.
    //
    // Throw a data_parse_error in case the envelope content contains
    // invalid JSON text.
    explicit Message(const std::string& transport_payload);

    // Parse the content of all message chunks, validate, and return
    // them as a ParsedChunks instance. The data chunk will be
    // validated with the schema indicated in the envelope.
    //
    // Throw a data_parse_error in case the envelope content contains
    // invalid JSON text.
    // Throw a validation_error in case the envelope content does
    // not match the envelope schema (as in pcp-specifications).
    // Throw a schema_not_found_error in case the envelope schema
    // was not registered.
    //
    // Note that bad debug/data chunks are reported in the returned
    // ParsedChunks objects; no error will will be propagated.
    ParsedChunks getParsedChunks(const Validator& validator) const;

    // Getter
    lth_jc::JsonContainer const& getEnvelope() const;

    // Return a string representation of the message.
    std::string toString() const;

  private:
    lth_jc::JsonContainer envelope_;
};

}  // namespace v2
}  // namespace PCPClient
