#ifndef CTHUN_CLIENT_SRC_CLIENT_H_
#define CTHUN_CLIENT_SRC_CLIENT_H_

// Comment the CTHUN_CLIENT_SECURE_TRANSPORT def to not use TLS
#define CTHUN_CLIENT_SECURE_TRANSPORT

#include "common/uuid.h"
#include "errors.h"
#include "macros.h"

#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/client.hpp>
// NB: we must include asio_client.hpp even if CTHUN_CLIENT_SECURE_TRANSPORT
// is not defined in order to define Context_Ptr
#include <websocketpp/config/asio_client.hpp>

#ifndef CTHUN_CLIENT_SECURE_TRANSPORT
#include <websocketpp/config/asio_no_tls_client.hpp>
#endif  // CTHUN_CLIENT_SECURE_TRANSPORT

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <thread>

namespace Cthun {
namespace Client {

//
// Tokens
//

static const std::string DEFAULT_CLOSE_REASON { "Closed by client" };

//
// Aliases
//

// TODO(ale): remove unnecessary aliases, to avoid masking

// Define the transport layer configuration for the client endpoint;
// used by Endpoint and Connection classes.
#ifdef CTHUN_CLIENT_SECURE_TRANSPORT
using Client_Type = websocketpp::client<websocketpp::config::asio_tls_client>;
#else
using Client_Type = websocketpp::client<websocketpp::config::asio_client>;
#endif  // CTHUN_CLIENT_SECURE_TRANSPORT

using Context_Ptr = websocketpp::lib::shared_ptr<boost::asio::ssl::context>;

using Message = std::string;

using Connection_ID = Common::UUID;
using Connection_Handle = websocketpp::connection_hdl;

using Close_Code = websocketpp::close::status::value;
namespace Close_Code_Values = websocketpp::close::status;

using Connection_State = websocketpp::session::state::value;
namespace Connection_State_Values = websocketpp::session::state;

using Frame_Opcode = websocketpp::frame::opcode::value;
namespace Frame_Opcode_Values = websocketpp::frame::opcode;

}  // namespace Client
}  // namespace Cthun

#endif  // CTHUN_CLIENT_SRC_CLIENT_H_
