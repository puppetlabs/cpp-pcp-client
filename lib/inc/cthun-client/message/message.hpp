#ifndef CTHUN_CLIENT_SRC_MESSAGE_MESSAGE_H_
#define CTHUN_CLIENT_SRC_MESSAGE_MESSAGE_H_

#include <cthun-client/message/chunks.hpp>
#include <cthun-client/message/errors.hpp>
#include <cthun-client/message/serialization.hpp>
#include <cthun-client/data_container/data_container.hpp>
#include <cthun-client/validator/validator.hpp>

#include <string>
#include <vector>
#include <map>
#include <initializer_list>
#include <stdint.h>  // uint8_t

namespace CthunClient {

//
// Constants
//

static const std::string ENVELOPE_SCHEMA_NAME { "envelope" };

static const std::string CTHUN_LOGIN_SCHEMA_NAME { "http://puppetlabs.com/loginschema" };
static const std::string CTHUN_REQUEST_SCHEMA_NAME { "http://puppetlabs.com/cnc_request" };
static const std::string CTHUN_RESPONSE_SCHEMA_NAME { "http://puppetlabs.com/cnc_response" };

//
// Message
//

class Message {
  public:
    // The default ctor is deleted since, for the Cthun protocol, a
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

    // Parse, validate, and return the content of chunks by using
    // the specified validator; the data chunk will be validated
    // with the schema indicated in the envelope.
    // Throw a schema_not_found_error in case the schema indicated in
    // the envelope is not registred in the validator.
    // Throw a validation_error in case of invalid message.
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

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_MESSAGE_MESSAGE_H_
