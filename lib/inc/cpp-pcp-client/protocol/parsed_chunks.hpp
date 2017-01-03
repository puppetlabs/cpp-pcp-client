/**
 * This is a shim to maintain compatibility with pxp-agent during development.
 */
#pragma once

#include <cpp-pcp-client/validator/schema.hpp>
#include <cpp-pcp-client/export.h>

#include <string>

namespace PCPClient {

//
// ParsedChunks
//
namespace lth_jc = leatherman::json_container;

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
                 lth_jc::JsonContainer _data,       // JSON data
                 std::vector<lth_jc::JsonContainer> _debug,
                 unsigned int _num_invalid_debug);

    ParsedChunks(lth_jc::JsonContainer _envelope,
                 std::string _binary_data,          // binary data
                 std::vector<lth_jc::JsonContainer> _debug,
                 unsigned int _num_invalid_debug);

    std::string toString() const;
};

}  // namespace PCPClient
