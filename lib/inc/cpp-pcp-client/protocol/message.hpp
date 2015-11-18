#ifndef CPP_PCP_CLIENT_SRC_PROTOCOL_MESSAGE_H_
#define CPP_PCP_CLIENT_SRC_PROTOCOL_MESSAGE_H_

#include <cpp-pcp-client/protocol/chunks.hpp>
#include <cpp-pcp-client/protocol/errors.hpp>
#include <cpp-pcp-client/protocol/serialization.hpp>
#include <cpp-pcp-client/validator/validator.hpp>
#include <cpp-pcp-client/export.h>

#include <string>
#include <vector>
#include <map>
#include <initializer_list>
#include <stdint.h>  // uint8_t

namespace PCPClient {

//
// Message
//

class LIBCPP_PCP_CLIENT_EXPORT Message {
  public:
    // The default ctor is deleted since, for the PCP protocol, a
    // valid message must have an envelope chunk (invariant)
    Message() = delete;

    // Construct a Message by parsing the payload delivered
    // by the transport layer as a std::string.
    // Throw an unsupported_version_error in case the indicated
    // message format version is not supported.
    // Throw a message_serialization_error in case of invalid message.
    explicit Message(const std::string& transport_payload);

    // Create a new message with a given envelope.
    // Throw a invalid_chunk_error in case of invalid chunk
    // (unknown descriptor or wrong size).
    explicit Message(MessageChunk envelope);

    // ... and a data chunk; throw a invalid_chunk_error as above
    Message(MessageChunk envelope,
            MessageChunk data_chunk);

    // ... and a debug chunk; throw a invalid_chunk_error as above
    Message(MessageChunk envelope,
            MessageChunk data_chunk,
            MessageChunk debug_chunk);

    // Add optional chunks after validating it.
    // Throw a invalid_chunk_error in case of invalid chunk (unknown
    // descriptor or wrong size).
    void setDataChunk(MessageChunk data_chunk);
    void addDebugChunk(MessageChunk debug_chunk);

    // Getters
    uint8_t getVersion() const;
    MessageChunk getEnvelopeChunk() const;
    MessageChunk getDataChunk() const;
    std::vector<MessageChunk> getDebugChunks() const;

    // Inspectors
    bool hasData() const;
    bool hasDebug() const;

    // Return the buffer containing the bytes of the serialized
    // message.
    // Throw a message_processing_error in case it fails to allocate
    // memory for the buffer.
    SerializedMessage getSerialized() const;

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

    // Return a string representation of all message fields.
    std::string toString() const;

  private:
    uint8_t version_;
    MessageChunk envelope_chunk_;
    MessageChunk data_chunk_;
    std::vector<MessageChunk> debug_chunks_;

    void parseMessage(const std::string& transport_msg);

    void validateVersion(const uint8_t& version) const;
    void validateChunk(const MessageChunk& chunk) const;
};

}  // namespace PCPClient

#endif  // CPP_PCP_CLIENT_SRC_PROTOCOL_MESSAGE_H_
