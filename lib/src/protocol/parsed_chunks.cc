#include <cpp-pcp-client/protocol/parsed_chunks.hpp>

namespace PCPClient {

//
// ParsedChunks
//

// Default ctor
ParsedChunks::ParsedChunks()
        : envelope {},
          has_data { false },
          invalid_data { false },
          data_type { ContentType::Json },
          data {},
          binary_data { "" },
          debug {},
          num_invalid_debug { 0 } {
}

// No data ctor
ParsedChunks::ParsedChunks(lth_jc::JsonContainer _envelope,
                           std::vector<lth_jc::JsonContainer> _debug,
                           unsigned int _num_invalid_debug)
        : envelope { _envelope },
          has_data { false },
          invalid_data { false },
          data_type { ContentType::Json },
          data {},
          binary_data { "" },
          debug { _debug },
          num_invalid_debug { _num_invalid_debug } {
}

// Invalid data ctor
ParsedChunks::ParsedChunks(lth_jc::JsonContainer _envelope,
                           bool _invalid_data,
                           std::vector<lth_jc::JsonContainer> _debug,
                           unsigned int _num_invalid_debug)
        : envelope { _envelope },
          has_data { _invalid_data },
          invalid_data { _invalid_data },
          data_type { ContentType::Json },
          data {},
          binary_data { "" },
          debug { _debug },
          num_invalid_debug { _num_invalid_debug } {
}

// JSON data ctor
ParsedChunks::ParsedChunks(lth_jc::JsonContainer _envelope,
                           lth_jc::JsonContainer _data,
                           std::vector<lth_jc::JsonContainer> _debug,
                           unsigned int _num_invalid_debug)
        : envelope { _envelope },
          has_data { true },
          invalid_data { false },
          data_type { ContentType::Json },
          data { _data },
          binary_data { "" },
          debug { _debug },
          num_invalid_debug { _num_invalid_debug } {
}

// Binary data ctor
ParsedChunks::ParsedChunks(lth_jc::JsonContainer _envelope,
                           std::string _binary_data,
                           std::vector<lth_jc::JsonContainer> _debug,
                           unsigned int _num_invalid_debug)
        : envelope { _envelope },
          has_data { true },
          invalid_data { false },
          data_type { ContentType::Binary },
          data {},
          binary_data { _binary_data },
          debug { _debug },
          num_invalid_debug { _num_invalid_debug } {
}

std::string ParsedChunks::toString() const {
    auto s = "ENVELOPE: " + envelope.toString();

    if (has_data) {
        s += "\nDATA: ";
        if (invalid_data) {
            s += "INVALID";
        } else if (data_type == ContentType::Json) {
            s += data.toString();
        } else {
            s += binary_data;
        }
    }

    for (auto& d : debug) {
        s += ("\nDEBUG: " + d.toString());
    }

    return s;
}

}  // namespace PCPClient
