#include <cthun-client/connector/connector.h>  // Connector
#include <cthun-client/connector/errors.h>     // connection_config_error

#include <string>
#include <iostream>

namespace Tutorial {

const std::string SERVER_URL { "wss://127.0.0.1:8090/cthun/" };

const std::string AGENT_CLIENT_TYPE { "tutorial_agent" };

const std::string CA   { "../../resources/ca_crt.pem" };
const std::string CERT { "../../resources/test_crt.pem" };
const std::string KEY  { "../../resources/test_key.pem" };

int main(int argc, char *argv[]) {
    // Connector constructor

    CthunClient::Connector connector { SERVER_URL,
                                       AGENT_CLIENT_TYPE,
                                       CA,
                                       CERT,
                                       KEY };

    std::cout << "Configured the connector\n";

    // Connector::connect()

    int num_connect_attempts { 4 };

    connector.connect(num_connect_attempts);

    // Connector::isConnected()

    if (connector.isConnected()) {
        std::cout << "Successfully connected to " << SERVER_URL << "\n";
    } else {
        std::cout << "The connection has dropped; the monitoring task "
                     "will take care of re-establishing it\n";
    }

    // Conneection::monitorConnection()

    connector.monitorConnection(num_connect_attempts);

    return 0;
}

}  // namespace Tutorial

int main(int argc, char** argv) {
    return Tutorial::main(argc, argv);
}
