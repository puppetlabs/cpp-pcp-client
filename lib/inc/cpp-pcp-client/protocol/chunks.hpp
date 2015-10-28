#ifndef CPP_PCP_CLIENT_SRC_PROTOCOL_CHUNKS_H_
#define CPP_PCP_CLIENT_SRC_PROTOCOL_CHUNKS_H_

#include <cpp-pcp-client/protocol/serialization.hpp>
#include <cpp-pcp-client/validator/schema.hpp>
#include <cpp-pcp-client/export.h>

#include <string>
#include <stdint.h>  // uint8_t

namespace PCPClient {

//
// ChunkDescriptor
//
namespace lth_jc = leatherman::json_container;

namespace ChunkDescriptor {
    // Filter the chunk type bits (envelope, data, debug)
    static const uint8_t TYPE_MASK { 0x0F };

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

struct LIBCPP_PCP_CLIENT_EXPORT MessageChunk {
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
// ParsedChunks
//

struct LIBCPP_PCP_CLIENT_EXPORT ParsedChunks {
    // Envelope
    lth_jc::JsonContainer envelope;

    // Data
    bool has_data;
    bool invalid_data;
    ContentType data_type;
    lth_jc::JsonContainer data;
    std::string binary_data;

    // Debug
    std::vector<lth_jc::JsonContainer> debug;
    unsigned int num_invalid_debug;

    ParsedChunks();

    ParsedChunks(lth_jc::JsonContainer _envelope,
                 std::vector<lth_jc::JsonContainer> _debug,
                 unsigned int _num_invalid_debug);

    ParsedChunks(lth_jc::JsonContainer _envelope,
                 bool _invalid_data,                // invalid data
                 std::vector<lth_jc::JsonContainer> _debug,
                 unsigned int _num_invalid_debug);

    ParsedChunks(lth_jc::JsonContainer _envelope,
                 lth_jc::JsonContainer _data,               // JSON data
                 std::vector<lth_jc::JsonContainer> _debug,
                 unsigned int _num_invalid_debug);

    ParsedChunks(lth_jc::JsonContainer _envelope,
                 std::string _binary_data,          // binary data
                 std::vector<lth_jc::JsonContainer> _debug,
                 unsigned int _num_invalid_debug);

    std::string toString() const;
};

}  // namespace PCPClient

#endif  // CPP_PCP_CLIENT_SRC_PROTOCOL_CHUNKS_H_
