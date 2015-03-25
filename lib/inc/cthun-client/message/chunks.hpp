#ifndef CTHUN_CLIENT_SRC_CHUNKS_H_
#define CTHUN_CLIENT_SRC_CHUNKS_H_

#include <cthun-client/message/serialization.hpp>
#include <cthun-client/validator/schema.hpp>
#include <cthun-client/data_container/data_container.hpp>

#include <string>
#include <stdint.h>  // uint8_t

namespace CthunClient {

//
// ChunkDescriptor
//

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
// ParsedChunks
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

    ParsedChunks();

    ParsedChunks(DataContainer _envelope,
                 std::vector<std::string> _debug);

    ParsedChunks(DataContainer _envelope,
                 DataContainer _data,
                 std::vector<std::string> _debug);

    ParsedChunks(DataContainer _envelope,
                 std::string _binary_data,
                 std::vector<std::string> _debug);

    std::string toString() const;
};

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_CHUNKS_H_
