#include "client.h"
#include "connection.h"
#include "endpoint.h"
#include "connection_manager.h"
#include "common/file_utils.h"

#include <boost/program_options.hpp>

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>

namespace Cthun {

namespace po = boost::program_options;

static std::string DEFAULT_CA { "./test-resources/ssl/ca/ca_crt.pem" };
static std::string DEFAULT_CERT { "./test-resources/ssl/certs/cthun-client.pem" };
static std::string DEFAULT_KEY { "./test-resources/ssl/private_keys/cthun-client.pem" };

int runTestConnection(std::string url,
                      int num_connections,
                      std::vector<std::string> messages,
                      std::string ca_crt_path,
                      std::string client_crt_path,
                      std::string client_key_path) {
    // Configure the Endpoint to use TLS
    Client::CONNECTION_MANAGER.configureSecureEndpoint(
        ca_crt_path, client_crt_path, client_key_path);

    Client::Connection c { url };

    std::vector<Client::Connection::Ptr> connections;

    for (auto i = 0; i < num_connections; i++) {
        try {
            // Create and configure a Connection
            Client::Connection::Ptr c_p {
                Client::CONNECTION_MANAGER.createConnection(url) };

            Client::Connection::Event_Callback onFail_c =
                [&](Client::Client_Type* client_ptr,
                        Client::Connection::Ptr connection_ptr) {
                    auto hdl = connection_ptr->getConnectionHandle();
                    std::cout << "### onFail callback -"
                              << " id: " << connection_ptr->getID()
                              << " server: " << connection_ptr->getRemoteServer()
                              << " error: " << connection_ptr->getErrorReason()
                              << " state: " << connection_ptr->getState() << "\n";
                };

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

            c_p->setOnFailCallback(onFail_c);
            c_p->setOnOpenCallback(onOpen_c);

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
    std::cout << "\n\n### Waiting to let the handshakes complete\n";
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

    // Sleep to get the messages back
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
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "display help")
        ("server", po::value<std::string>()->default_value(
            "wss://localhost:8090/cthun/"), "address of the cthun server")
        ("num_connections", po::value<int>()->default_value(1),
            "number of connections")
        ("ca", po::value<std::string>()->default_value(DEFAULT_CA),
            "CA certificate")
        ("cert", po::value<std::string>()->default_value(DEFAULT_CERT),
            "client certificate")
        ("key", po::value<std::string>()->default_value(DEFAULT_KEY),
            "client private key")
        ("messages,M", po::value<std::vector<std::string>>()->default_value(
            std::vector<std::string>(), ""), "messages");

    po::positional_options_description p;
    p.add("messages", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
              .options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc;
        return 0;
    }

    // TODO(ale): does boost offer something similar?
    auto expandPath = Common::FileUtils::expandAsDoneByShell;

    return runTestConnection(vm["server"].as<std::string>(),
                             vm["num_connections"].as<int>(),
                             vm["messages"].as<std::vector<std::string>>(),
                             expandPath(vm["ca"].as<std::string>()),
                             expandPath(vm["cert"].as<std::string>()),
                             expandPath(vm["key"].as<std::string>()));
}

}  // namespace Cthun


int main(int argc, char** argv) {
    try {
        return Cthun::main(argc, argv);
    } catch (std::runtime_error& e) {
        std::cout << "UNEXPECTED EXCEPTION: " << e.what() << std::endl;
        return 1;
    }
}
