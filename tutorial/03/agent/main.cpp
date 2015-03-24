#include <cthun-client/connector/connector.h>  // Connector
#include <cthun-client/connector/errors.h>     // connection_config_error

#include <string>
#include <iostream>
#include <memory>  // unique_ptr

namespace Tutorial {

const std::string SERVER_URL { "wss://127.0.0.1:8090/cthun/" };

const std::string AGENT_CLIENT_TYPE { "tutorial_agent" };

const std::string CA   { "../../resources/agent_certs/ca.pem" };
const std::string CERT { "../../resources/agent_certs/crt.pem" };
const std::string KEY  { "../../resources/agent_certs/key.pem" };

int main(int argc, char *argv[]) {
    std::unique_ptr<CthunClient::Connector> connector_ptr;

    // Connector constructor

    try {
        connector_ptr.reset(new CthunClient::Connector { SERVER_URL,
                                                         AGENT_CLIENT_TYPE,
                                                         CA,
                                                         CERT,
                                                         KEY });
        std::cout << "Configured the connector\n";
    } catch (CthunClient::connection_config_error& e) {
        std::cout << "Failed to configure the Cthun connector: "
                  << e.what() << "\n";
        return 1;
    }

    // Connector::connect()

    int num_connect_attempts { 4 };

    try {
        connector_ptr->connect(num_connect_attempts);
    } catch (CthunClient::connection_config_error& e) {
        std::cout << "Failed to configure WebSocket: " << e.what() << "\n";
        return 2;
    } catch (CthunClient::connection_fatal_error& e) {
        std::cout << "Failed to connect to " << SERVER_URL << " after "
                  << num_connect_attempts << " attempts: " << e.what() << "\n";
        return 2;
    }

    // Connector::isConnected()

    if (connector_ptr->isConnected()) {
        std::cout << "Successfully connected to " << SERVER_URL << "\n";
    } else {
        std::cout << "The connection has dropped; the monitoring task "
                     "will take care of re-establishing it\n";
    }

    // Conneection::monitorConnection()

    try {
        connector_ptr->monitorConnection(num_connect_attempts);
    } catch (CthunClient::connection_fatal_error& e) {
        std::cout << "Failed to reconnect to " << SERVER_URL << " after "
                  << num_connect_attempts << " attempts: " << e.what() << "\n";
        return 2;
    }

    return 0;
}

}  // namespace Tutorial

int main(int argc, char** argv) {
    return Tutorial::main(argc, argv);
}
