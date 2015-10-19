#include <cpp-pcp-client/connector/connector.hpp>  // Connector
#include <cpp-pcp-client/connector/errors.hpp>     // connection_config_error

#include <string>
#include <iostream>

namespace Tutorial {

const std::string BROKER_URL { "wss://127.0.0.1:8090/cthun/" };

const std::string AGENT_CLIENT_TYPE { "tutorial_agent" };

const std::string CA   { "../../resources/agent_certs/ca.pem" };
const std::string CERT { "../../resources/agent_certs/crt.pem" };
const std::string KEY  { "../../resources/agent_certs/key.pem" };

int main(int argc, char *argv[]) {
    // Connector constructor

    PCPClient::Connector connector { BROKER_URL,
                                     AGENT_CLIENT_TYPE,
                                     CA,
                                     CERT,
                                     KEY };

    std::cout << "Configured the connector\n";

    return 0;
}

}  // namespace Tutorial

int main(int argc, char** argv) {
    return Tutorial::main(argc, argv);
}
