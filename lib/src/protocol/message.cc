#include <cpp-pcp-client/protocol/message.hpp>
#include <cpp-pcp-client/protocol/schemas.hpp>

#define LEATHERMAN_LOGGING_NAMESPACE CPP_PCP_CLIENT_LOGGING_PREFIX".message"

#include <leatherman/logging/logging.hpp>

#include <leatherman/locale/locale.hpp>

#include <algorithm>  // find

// TODO(ale): disable assert() once we're confident with the code...
// To disable assert()
// #define NDEBUG
#include <cassert>

namespace PCPClient {

namespace lth_jc  = leatherman::json_container;
namespace lth_loc = leatherman::locale;

//
// Constants
//

// TODO(ale): use a greater min value based on the expected json entries
// Min size of the envelope chunk data [byte]
static const size_t MIN_ENVELOPE_SIZE { 6 };

// Size of descriptor and size fields [byte]
static const size_t CHUNK_METADATA_SIZE { 5 };

// Size of version field [byte]
static const size_t VERSION_FIELD_SIZE { 1 };

// Ordered list of supported protocol versions; the last entry should
// be used when creating new messages
static std::vector<uint8_t> SUPPORTED_VERSIONS { 1 };

//
// Message
//

// Constructors

Message::Message(const std::string& transport_msg) : version_ {},
                                                     envelope_chunk_ {},
                                                     data_chunk_ {},
                                                     debug_chunks_ {} {
    parseMessage(transport_msg);
}

Message::Message(MessageChunk envelope_chunk)
        : version_ { SUPPORTED_VERSIONS.back() },
          envelope_chunk_ { envelope_chunk },
          data_chunk_ {},
          debug_chunks_ {} {
    validateChunk(envelope_chunk);
}

Message::Message(MessageChunk envelope_chunk, MessageChunk data_chunk)
        : version_ { SUPPORTED_VERSIONS.back() },
          envelope_chunk_ { envelope_chunk },
          data_chunk_ { data_chunk },
          debug_chunks_ {} {
    validateChunk(envelope_chunk);
    validateChunk(data_chunk);
}

Message::Message(MessageChunk envelope_chunk, MessageChunk data_chunk,
                 MessageChunk debug_chunk)
        : version_ { SUPPORTED_VERSIONS.back() },
          envelope_chunk_ { envelope_chunk },
          data_chunk_ { data_chunk },
          debug_chunks_ { debug_chunk } {
    validateChunk(envelope_chunk);
    validateChunk(data_chunk);
    validateChunk(debug_chunk);
}

// Add chunks

void Message::setDataChunk(MessageChunk data_chunk) {
    validateChunk(data_chunk);

    if (hasData()) {
        LOG_WARNING("Resetting data chunk");
    }

    data_chunk_ = data_chunk;
}

void Message::addDebugChunk(MessageChunk debug_chunk) {
    validateChunk(debug_chunk);
    debug_chunks_.push_back(debug_chunk);
}

// Getters

uint8_t Message::getVersion() const {
    return version_;
}

MessageChunk Message::getEnvelopeChunk() const {
    return envelope_chunk_;
}

MessageChunk Message::getDataChunk() const {
    return data_chunk_;
}

std::vector<MessageChunk> Message::getDebugChunks() const {
    return debug_chunks_;
}

// Inspectors

bool Message::hasData() const {
    return data_chunk_.descriptor != 0;
}

bool Message::hasDebug() const {
    return !debug_chunks_.empty();
}

// Get the serialized message

SerializedMessage Message::getSerialized() const {
    SerializedMessage buffer;

    // Version (mandatory)
    serialize<uint8_t>(version_, 1, buffer);

    // Envelope (mandatory)
    envelope_chunk_.serializeOn(buffer);

    if (hasData()) {
        // Data (optional)
        data_chunk_.serializeOn(buffer);
    }

    if (hasDebug()) {
        for (const auto& d_c : debug_chunks_)
            // Debug (optional; mutiple)
            d_c.serializeOn(buffer);
    }

    return buffer;
}

// Parse JSON, validate schema, and return the content of chunks

ParsedChunks Message::getParsedChunks(const lth_jc::Validator& validator) const {
    // Envelope
    lth_jc::JsonContainer envelope_content { envelope_chunk_.content };
    validator.validate(envelope_content, Protocol::ENVELOPE_SCHEMA_NAME);
    auto msg_id = envelope_content.get<std::string>("id");

    // Debug
    std::vector<lth_jc::JsonContainer> debug_content {};
    unsigned int num_invalid_debug { 0 };
    for (const auto& d_c : debug_chunks_) {
        try {
            // Parse the JSON text
            lth_jc::JsonContainer parsed_debug { d_c.content };

            // Validate entire content (array)
            validator.validate(parsed_debug, Protocol::DEBUG_SCHEMA_NAME);

            // Validate each hop entry
            for (auto &hop : parsed_debug.get<std::vector<lth_jc::JsonContainer>>("hops")) {
                validator.validate(hop, Protocol::DEBUG_ITEM_SCHEMA_NAME);
            }

            debug_content.push_back(parsed_debug);
        } catch (lth_jc::data_parse_error& e) {
            num_invalid_debug++;
            LOG_DEBUG("Invalid debug in message {1}: {2}", msg_id, e.what());
        } catch (lth_jc::validator_error& e) {
            num_invalid_debug++;
            LOG_DEBUG("Invalid debug in message {1}: {2}", msg_id, e.what());
        }
    }

    // Data
    if (hasData()) {
        auto message_type = envelope_content.get<std::string>("message_type");
        auto content_type = validator.getSchemaContentType(message_type);

        if (content_type == lth_jc::ContentType::Json) {
            std::string err_msg {};
            try {
                lth_jc::JsonContainer data_content_json { data_chunk_.content };
                validator.validate(data_content_json, message_type);

                // Valid JSON data content
                return ParsedChunks { envelope_content,
                                      data_content_json,
                                      debug_content,
                                      num_invalid_debug };
            } catch (lth_jc::data_parse_error& e) {
                err_msg = e.what();
            } catch (lth_jc::validator_error& e) {
                err_msg = e.what();
            }

            LOG_DEBUG("Invalid data in message {1}: {2}", msg_id, err_msg);

            // Bad JSON data content
            return ParsedChunks { envelope_content,
                                  true,
                                  debug_content,
                                  num_invalid_debug };
        } else if (content_type == lth_jc::ContentType::Binary) {
            auto data_content_binary = data_chunk_.content;

            // Binary data content
            return ParsedChunks { envelope_content,
                                  data_content_binary,
                                  debug_content,
                                  num_invalid_debug };
        } else {
            assert(false);
        }
    }

    return ParsedChunks { envelope_content, debug_content, num_invalid_debug };
}

// toString

std::string Message::toString() const {
    auto s = std::to_string(version_) + envelope_chunk_.toString();

    if (hasData()) {
        s += data_chunk_.toString();
    }

    for (const auto& debug_chunk : debug_chunks_) {
        s += debug_chunk.toString();
    }

    return s;
}

//
// Message - private interface
//

void Message::parseMessage(const std::string& transport_msg) {
    auto msg_size = transport_msg.size();

    if (msg_size < MIN_ENVELOPE_SIZE) {
        LOG_ERROR("Invalid msg; envelope is too small");
        LOG_TRACE("Invalid msg content (unserialized): '{1}'", transport_msg);
        throw message_serialization_error {
            lth_loc::translate("invalid msg: envelope too small") };
    }

    // Serialization buffer

    auto buffer = SerializedMessage(transport_msg.begin(), transport_msg.end());
    SerializedMessage::const_iterator next_itr { buffer.begin() };

    // Version

    auto msg_version = deserialize<uint8_t>(1, next_itr);
    validateVersion(msg_version);

    // Envelope (mandatory chunk)

    auto envelope_desc = deserialize<uint8_t>(1, next_itr);
    auto envelope_desc_bit = envelope_desc & ChunkDescriptor::TYPE_MASK;
    if (envelope_desc_bit != ChunkDescriptor::ENVELOPE) {
        LOG_ERROR("Invalid msg; missing envelope descriptor");
        LOG_TRACE("Invalid msg content (unserialized): '{1}'", transport_msg);
        throw message_serialization_error {
            lth_loc::translate("invalid msg: no envelope descriptor") };
    }

    auto envelope_size = deserialize<uint32_t>(4, next_itr);
    if (msg_size < VERSION_FIELD_SIZE + CHUNK_METADATA_SIZE + envelope_size) {
        LOG_ERROR("Invalid msg; missing envelope content");
        LOG_TRACE("Invalid msg content (unserialized): '{1}'", transport_msg);
        throw message_serialization_error {
            lth_loc::translate("invalid msg: no envelope") };
    }

    auto envelope_content = deserialize<std::string>(envelope_size, next_itr);

    // Data and debug (optional chunks)

    auto still_to_parse =  msg_size - (VERSION_FIELD_SIZE + CHUNK_METADATA_SIZE
                                       + envelope_size);

    while (still_to_parse > CHUNK_METADATA_SIZE) {
        auto chunk_desc = deserialize<uint8_t>(1, next_itr);
        auto chunk_desc_bit = chunk_desc & ChunkDescriptor::TYPE_MASK;

        if (chunk_desc_bit != ChunkDescriptor::DATA
                && chunk_desc_bit != ChunkDescriptor::DEBUG) {
            LOG_ERROR("Invalid msg; invalid chunk descriptor {1}",
                      static_cast<int>(chunk_desc));
            LOG_TRACE("Invalid msg content (unserialized): '{1}'", transport_msg);
            throw message_serialization_error {
                lth_loc::translate("invalid msg: invalid chunk descriptor") };
        }

        auto chunk_size = deserialize<uint32_t>(4, next_itr);
        auto missing_bytes = still_to_parse - CHUNK_METADATA_SIZE;
        if (chunk_size > missing_bytes) {
            // TODO(ale): deal with locale & plural
            assert(chunk_size > 1);
            if (missing_bytes == 1) {
                LOG_ERROR("Invalid msg; missing part of the {1} chunk content ({2} "
                          "bytes declared - missing {3} byte)",
                          ChunkDescriptor::names[chunk_desc_bit], chunk_size,
                          missing_bytes);
            } else {
                LOG_ERROR("Invalid msg; missing part of the {1} chunk content ({2} "
                          "bytes declared - missing {3} bytes)",
                          ChunkDescriptor::names[chunk_desc_bit], chunk_size,
                          missing_bytes);
            }
            LOG_TRACE("Invalid msg content (unserialized): '{1}'", transport_msg);
            throw message_serialization_error {
                lth_loc::translate("invalid msg: missing chunk content") };
        }

        auto chunk_content = deserialize<std::string>(chunk_size, next_itr);
        MessageChunk chunk { chunk_desc, chunk_size, chunk_content };

        if (chunk_desc_bit == ChunkDescriptor::DATA) {
            if (hasData()) {
                LOG_ERROR("Invalid msg; multiple data chunks");
                LOG_TRACE("Invalid msg content (unserialized): '{1}'",
                          transport_msg);
                throw message_serialization_error {
                    lth_loc::translate("invalid msg: multiple data chunks") };
            }

            data_chunk_ = chunk;
        } else {
            debug_chunks_.push_back(chunk);
        }

        still_to_parse -= CHUNK_METADATA_SIZE + chunk_size;
    }

    if (still_to_parse > 0) {
        // TODO(ale): deal with locale & plural
        if (still_to_parse == 1) {
            LOG_ERROR("Failed to parse the entire msg (ignoring last {1} byte); "
                      "the msg will be processed anyway", still_to_parse);
        } else {
            LOG_ERROR("Failed to parse the entire msg (ignoring last {1} bytes); "
                      "the msg will be processed anyway", still_to_parse);
        }
        LOG_TRACE("Msg content (unserialized): '{1}'", transport_msg);
    }

    version_ = msg_version;
    envelope_chunk_ = MessageChunk  { envelope_desc,
                                      envelope_size,
                                      envelope_content };
}

void Message::validateVersion(const uint8_t& version) const {
    auto found = std::find(SUPPORTED_VERSIONS.begin(), SUPPORTED_VERSIONS.end(),
                           version);
    if (found == SUPPORTED_VERSIONS.end()) {
        auto version_num = static_cast<int>(version);
        LOG_ERROR("Unsupported message version: {1}", version_num);
        throw unsupported_version_error {
            lth_loc::format("unsupported message version: {1}", version_num) };
    }
}

void Message::validateChunk(const MessageChunk& chunk) const {
    auto desc_bit = chunk.descriptor & ChunkDescriptor::TYPE_MASK;

    if (ChunkDescriptor::names.find(desc_bit) == ChunkDescriptor::names.end()) {
        LOG_ERROR("Unknown chunk descriptor: {1}",
                  static_cast<int>(chunk.descriptor));
        throw invalid_chunk_error { lth_loc::translate("unknown descriptor") };
    }

    if (chunk.size != static_cast<uint32_t>(chunk.content.size())) {
        // TODO(ale): deal with locale and plural
        if (chunk.size == 1) {
            assert(chunk.content.size() != 1);
            LOG_ERROR("Incorrect size for {1} chunk; declared {2} byte, got {3} bytes",
                      ChunkDescriptor::names[desc_bit], chunk.size, chunk.content.size());
        } else {
            if (chunk.content.size() == 1) {
                LOG_ERROR("Incorrect size for {1} chunk; declared {2} bytes, got {3} byte",
                          ChunkDescriptor::names[desc_bit], chunk.size, chunk.content.size());
            } else {
                LOG_ERROR("Incorrect size for {1} chunk; declared {2} bytes, got {3} bytes",
                          ChunkDescriptor::names[desc_bit], chunk.size, chunk.content.size());
            }
        }
        throw invalid_chunk_error { lth_loc::translate("invalid size") };
    }
}

}  // namespace PCPClient
