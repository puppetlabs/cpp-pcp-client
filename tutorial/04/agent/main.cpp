#include <cpp-pcp-client/connector/connector.hpp>  // Connector
#include <cpp-pcp-client/connector/errors.hpp>     // connection_config_error

#include <cpp-pcp-client/protocol/chunks.hpp>       // ParsedChunk

#include <cpp-pcp-client/validator/schema.hpp>     // Schema, ContentType

#include <string>
#include <iostream>
#include <memory>  // unique_ptr
#include <vector>
#include <stdexcept>

namespace Tutorial {

const std::string SERVER_URL { "wss://127.0.0.1:8090/cthun/" };

const std::string AGENT_CLIENT_TYPE { "tutorial_agent" };

const std::string CA   { "../../resources/agent_certs/ca.pem" };
const std::string CERT { "../../resources/agent_certs/crt.pem" };
const std::string KEY  { "../../resources/agent_certs/key.pem" };

const std::string REQUEST_SCHEMA_NAME { "tutorial_request_schema" };

//
// agent_error
//

class agent_error : public std::runtime_error {
  public:
    explicit agent_error(std::string const& msg) : std::runtime_error(msg) {}
};

//
// Agent
//

class Agent {
  public:
    Agent();
    void start();

  private:
    int num_connect_attempts_;
    PCPClient::Schema request_schema_;
    std::unique_ptr<PCPClient::Connector> connector_ptr_;

    void processRequest(const PCPClient::ParsedChunks& parsed_chunks);
};

Agent::Agent()
    try
        : num_connect_attempts_ { 4 },
          request_schema_ { REQUEST_SCHEMA_NAME,
                            PCPClient::ContentType::Json },
          connector_ptr_ { new PCPClient::Connector { SERVER_URL,
                                                      AGENT_CLIENT_TYPE,
                                                      CA,
                                                      CERT,
                                                      KEY } } {
    // Add constraints to the request message schema
    using T_C = PCPClient::TypeConstraint;
    request_schema_.addConstraint("request", T_C::Object, true);   // mandatory
    request_schema_.addConstraint("details", T_C::String, false);  // optional
} catch (PCPClient::connection_config_error& e) {
    std::string err_msg { "failed to configure the PCP Connector: " };
    throw agent_error { err_msg + e.what() };
}

void Agent::start() {
    // Connector::registerMessageCallback()

    connector_ptr_->registerMessageCallback(
        request_schema_,
        [this](const PCPClient::ParsedChunks& parsed_chunks) {
            processRequest(parsed_chunks);
        });

    // Connector::connect()

    try {
        connector_ptr_->connect(num_connect_attempts_);
    } catch (PCPClient::connection_config_error& e) {
        std::string err_msg { "failed to configure WebSocket: " };
        throw agent_error { err_msg + e.what() };
    } catch (PCPClient::connection_fatal_error& e) {
        std::string err_msg { "failed to connect to " + SERVER_URL + " after "
                              + std::to_string(num_connect_attempts_)
                              + "attempts: " };
        throw agent_error { err_msg + e.what() };
    }

    // Connector::isConnected()

    if (connector_ptr_->isConnected()) {
        std::cout << "Successfully connected to " << SERVER_URL << "\n";
    } else {
        std::cout << "The connection has dropped; the monitoring task "
                     "will take care of re-establishing it\n";
    }

    // Conneection::monitorConnection()

    try {
        connector_ptr_->monitorConnection(num_connect_attempts_);
    } catch (PCPClient::connection_fatal_error& e) {
        std::string err_msg { "failed to reconnect to " + SERVER_URL + ": " };
        throw agent_error { err_msg + e.what() };
    }
}

void Agent::processRequest(const PCPClient::ParsedChunks& parsed_chunks) {
    auto request_id = parsed_chunks.envelope.get<std::string>("id");
    auto requester_endpoint = parsed_chunks.envelope.get<std::string>("sender");

    std::cout << "Received message " << request_id
              << " from " << requester_endpoint << ":\n"
              << parsed_chunks.toString() << "\n";
}

//
// main
//

int main(int argc, char *argv[]) {
    try {
        Agent agent {};

        try {
            agent.start();
        } catch (agent_error& e) {
            std::cout << "Failed to start the agent: " << e.what() << std::endl;
            return 2;
        }
    } catch (agent_error& e) {
        std::cout << "Failed to initialize the agent: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

}  // namespace Tutorial

int main(int argc, char** argv) {
    return Tutorial::main(argc, argv);
}
