#include <cpp-pcp-client/protocol/v1/chunks.hpp>

namespace PCPClient {
namespace v1 {

namespace lth_jc = leatherman::json_container;

//
// MessageChunk
//

MessageChunk::MessageChunk() : descriptor { 0 },
                               size { 0 },
                               content {} {
}

MessageChunk::MessageChunk(uint8_t _descriptor,
                           uint32_t _size,
                           std::string _content)
        : descriptor { _descriptor },
          size { _size },
          content { _content } {
}

MessageChunk::MessageChunk(uint8_t _descriptor,
                           std::string _content)
        : MessageChunk(_descriptor, _content.size(), _content) {
}

bool MessageChunk::operator==(const MessageChunk& other_msg_chunk) const {
    return descriptor == other_msg_chunk.descriptor
           && size == other_msg_chunk.size
           && content == other_msg_chunk.content;
}

void MessageChunk::serializeOn(SerializedMessage& buffer) const {
    serialize<uint8_t>(descriptor, 1, buffer);
    serialize<uint32_t>(size, 4, buffer);
    serialize<std::string>(content, size, buffer);
}

std::string MessageChunk::toString() const {
     return "size: " + std::to_string(size) + " bytes - content: " + content;
}

}  // namespace v1
}  // namespace PCPClient
