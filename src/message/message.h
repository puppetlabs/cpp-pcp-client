#ifndef CTHUN_CLIENT_SRC_MESSAGE_MESSAGE_H_
#define CTHUN_CLIENT_SRC_MESSAGE_MESSAGE_H_

#include "./errors.h"
#include "./serialization.h"

#include "../data_container/data_container.h"

#include "../validator/validator.h"

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
// ChunkDescriptor
//

namespace ChunkDescriptor {
    // Filter the chunk type bits (envelope, data, debug)
    static const uint8_t TYPE_MASK { 0x0F };

    // All the descriptors that use the JSON format
    static const std::vector<uint8_t> JSON_DESCRIPTORS { 0x00 };

    static const uint8_t ENVELOPE { 0x01 };
    static const uint8_t DATA { 0x02 };
    static const uint8_t DEBUG { 0x03 };

    static std::map<uint8_t, const std::string> names {
        { ENVELOPE, "envelope" },
        { DATA, "data" },
        { DEBUG, "debug" }
    };

}  // namespace ChunkDescriptor

//
// MessageChunk
//

struct MessageChunk {
    uint8_t descriptor;
    uint32_t size;  // [byte]
    std::string content;

    MessageChunk();

    MessageChunk(uint8_t _descriptor, uint32_t _size, std::string _content);

    MessageChunk(uint8_t _descriptor, std::string _content);

    bool operator==(const MessageChunk& other_msg_chunk) const;

    void serializeOn(SerializedMessage& buffer) const;

    std::string toString() const;
};

//
// ParsedContent
//

// TODO(ale): update the parsed debug format once we define specs

struct ParsedChunks {
    // Envelope
    DataContainer envelope;

    // Data
    bool has_data;
    ContentType data_type;
    DataContainer data;
    std::string binary_data;

    // Debug
    std::vector<std::string> debug;

    // Default ctor
    ParsedChunks()
            : envelope {},
              has_data { false },
              data_type { ContentType::Json },
              data {},
              binary_data { "" },
              debug {} {}

    // No data ctor
    ParsedChunks(DataContainer _envelope,
                 std::vector<std::string> _debug)
            : envelope { _envelope },
              has_data { false },
              data_type { ContentType::Json },
              data {},
              binary_data { "" },
              debug { _debug } {}

    // JSON data ctor
    ParsedChunks(DataContainer _envelope,
                 DataContainer _data,
                 std::vector<std::string> _debug)
            : envelope { _envelope },
              has_data { true },
              data_type { ContentType::Json },
              data { _data },
              binary_data { "" },
              debug { _debug } {}

    // Binary data ctor
    ParsedChunks(DataContainer _envelope,
                 std::string _binary_data,
                 std::vector<std::string> _debug)
            : envelope { _envelope },
              has_data { true },
              data_type { ContentType::Binary },
              data {},
              binary_data { _binary_data },
              debug { _debug } {}
};

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

    void parseMessage_(const std::string& transport_msg);

    void validateVersion_(const uint8_t& version) const;
    void validateChunk_(const MessageChunk& chunk) const;
};

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_MESSAGE_MESSAGE_H_
