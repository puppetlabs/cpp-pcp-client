#include "client.h"
#include "connection.h"
#include "endpoint.h"
#include "connection_manager.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>

namespace Cthun {

void showHelp() {
    std::cout << "Invalid command line arguments.\n"
              << "Usage: ctunn-client [url] [num connections] "
              << "{message} {message} ... \n";
}

int runTestConnection(std::string url,
                      int num_connections,
                      std::vector<std::string> messages) {
    Client::Connection c { url };

    // using Lala = std::shared_ptr<Client::Connection<Client::Client_No_TLS>>;
    // std::vector<Lala> connections;

    std::vector<Client::Connection::Ptr> connections;

    for (auto i = 0; i < num_connections; i++) {
        try {
            // Create and configure a Connection

            Client::Connection::Ptr c_p {
                Client::CONNECTION_MANAGER.createConnection(url) };

            Client::Connection::Event_Callback onOpen_c =
                [&](Client::Client_Type* client_ptr,
                        Client::Connection::Ptr connection_ptr) {
                    auto hdl = connection_ptr->getConnectionHandle();
                    std::cout << "### onOpen callback: connection id: "
                              << connection_ptr->getID() << " server: "
                              << connection_ptr->getRemoteServer() << "\n";
                    for (auto msg : messages) {
                        client_ptr->send(hdl, msg,
                                         Client::Frame_Opcode_Values::text);
                    }
                };

            c_p->setOnOpenCallback(onOpen_c);

            std::cout << "### About to open!\n";

            // Connect to server
            Client::CONNECTION_MANAGER.open(c_p);

            // Store the Connection pointer
            connections.push_back(c_p);
        } catch(Client::client_error& e) {
            std::cout << "### ERROR (connecting): " << e.what() << std::endl;
            return 1;
        }
    }

    // Sleep a bit to let the handshakes complete
    std::cout << "\n\n### Waiting a bit to let the handshakes complete\n";
    sleep(4);
    std::cout << "### Done waiting!\n";

    // Send messages
    try {
        int connection_idx { 0 };

        for (auto c_p : connections) {
            connection_idx++;
            std::string sync_message { "### Message (SYNC) for connection "
                                       + std::to_string(connection_idx) };
            if (c_p->getState() == Client::Connection_State_Values::open) {
                Client::CONNECTION_MANAGER.send(c_p, sync_message);
                std::cout << "### Message sent (SYNCHRONOUS - MAIN THREAD) "
                          << "on connection " << c_p->getID() << "\n";
            } else {
                std::cout << "### Connection " << c_p->getID()
                          << " is not open yet... Current state = "
                          << c_p->getState() << " Skipping!\n";
            }
        }
    } catch(Client::client_error& e) {
        // NB: catching the base error class
        std::cout << "### ERROR (sending): " << e.what() << std::endl;
        return 1;
    }

    // Sleep a bit to get the messages back

    std::cout << "\n\n### Waiting a bit to let the messages come back\n";
    sleep(4);
    std::cout << "### Done waiting!\n";

    // Close connections synchronously
    try {
        // NB: this is done by the destructor
        sleep(6);
        std::cout << "### ### ###\n"
                  << "### Done sending; about to close all connections\n";
        Client::CONNECTION_MANAGER.closeAllConnections();
    } catch(Client::client_error& e) {
        std::cout << "### ERROR (closing): " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        showHelp();
        return 1;
    }

    std::string url { argv[1] };

    std::istringstream num_connections_stream { argv[2] };
    int num_connections;
    if (!(num_connections_stream >> num_connections)) {
        showHelp();
        return 1;
    }

    std::vector<std::string> messages {};
    for (auto i = 3; i < argc; i++) {
        messages.push_back(argv[i]);
    }

    return runTestConnection(url, num_connections, messages);
}

}  // namespace Cthun


int main(int argc, char** argv) {
    try {
        return Cthun::main(argc, argv);
    } catch(std::runtime_error& e) {
        std::cout << "UNEXPECTED EXCEPTION: " << e.what() << std::endl;
        return 1;
    }
}
