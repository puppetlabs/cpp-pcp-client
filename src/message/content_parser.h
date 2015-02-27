#ifndef CTHUN_CLIENT_SRC_CONTENT_PARSER_H_
#define CTHUN_CLIENT_SRC_CONTENT_PARSER_H_

#include "./message.h"

#include "../data_container/data_container.h"
#include "../schemas.h"

namespace CthunClient {

namespace ContentParser {

DataContainer parseAndValidateChunk(const MessageChunk& chunk,
                                    CthunContentFormat format =
                                            CthunContentFormat::json);

}  // namespace ContentParser

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_CONTENT_PARSER_H_
