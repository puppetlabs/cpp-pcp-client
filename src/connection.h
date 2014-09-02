#ifndef CTHUN_CLIENT_SRC_CONNECTION_H_
#define CTHUN_CLIENT_SRC_CONNECTION_H_

#include "client.h"

#include <websocketpp/common/connection_hdl.hpp>

#include <memory>
#include <string>

namespace Cthun {
namespace Client {

class Connection {
  public:
    using Ptr = std::shared_ptr<Connection>;
    using Event_Callback = std::function<void(
        Client_Type* client_ptr, Connection* connection_ptr)>;

    using OnMessage_Callback = std::function<void(
        Client_Type* client_ptr, Connection* connection_ptr, std::string message)>;


    explicit Connection(std::string url);

    //
    // Configuration
    //

    /// Connection handle setter.
    void setConnectionHandle(Connection_Handle hdl);

    /// Callback called by onOpen.
    void setOnOpenCallback(Event_Callback callback);

    /// Callback called by onClose.
    void setOnCloseCallback(Event_Callback callback);

    /// Callback called by onFail.
    void setOnFailCallback(Event_Callback callback);

    /// Callback called by onMessage.
    void setOnMessageCallback(OnMessage_Callback callback);

    //
    // Accessors
    //

    std::string getURL() const;
    Connection_ID getID() const;
    Connection_Handle getConnectionHandle() const;
    Connection_State getState() const;
    std::string getErrorReason() const;
    std::string getRemoteServer() const;
    std::string getRemoteCloseReason() const;
    Close_Code getRemoteCloseCode() const;

    //
    // Event handlers
    //

    // NB: the TLS initialization handler is bound to the Endpoint, so
    // it's applied to all connections. Binding it here doesn't work.

    /// Event handler: on open.
    virtual void onOpen(Client_Type* client_ptr, Connection_Handle hdl);

    /// Event handler: on close.
    virtual void onClose(Client_Type* client_ptr, Connection_Handle hdl);

    /// Event handler: on fail.
    virtual void onFail(Client_Type* client_ptr, Connection_Handle hdl);

    /// Event handler: on message.
    virtual void onMessage(Client_Type* client_ptr, Connection_Handle hdl,
                           Client_Type::message_ptr msg);

  protected:
    std::string url_;
    Connection_ID id_;
    Connection_Handle connection_hdl_;
    Connection_State state_;
    std::string error_reason_;
    std::string remote_server_;
    std::string remote_close_reason_;
    Close_Code remote_close_code_;

    Event_Callback onOpen_callback_ {
        [](Client_Type* client_ptr, Connection* connection_ptr) {} };
    Event_Callback onClose_callback_ {
        [](Client_Type* client_ptr, Connection* connection_ptr) {} };
    Event_Callback onFail_callback_ {
        [](Client_Type* client_ptr, Connection* connection_ptr) {} };
    OnMessage_Callback onMessage_callback_ {
        [](Client_Type* client_ptr, Connection* connection_ptr,
           std::string message) {} };
};

}  // namespace Client
}  // namespace Cthun

#endif  // CTHUN_CLIENT_SRC_CONNECTION_H_
