#include "../common.h"

#include <cthun-client/connector/connector.h>  // Connector
#include <cthun-client/connector/errors.h>     // connection_config_error

#include <cthun-client/data_container/data_container.h>  // DataContainer

#include <string>
#include <iostream>
#include <memory>    // unique_ptr
#include <unistd.h>  // sleep

namespace Tutorial {

const std::string CA   { "../../resources/controller_certs/ca.pem" };
const std::string CERT { "../../resources/controller_certs/crt.pem" };
const std::string KEY  { "../../resources/controller_certs/key.pem" };

//
// controller_error
//

class controller_error : public std::runtime_error {
  public:
    explicit controller_error(std::string const& msg) : std::runtime_error(msg) {}
};

//
// Controller
//

class Controller {
  public:
    Controller();
    void sendRequests();

  private:
    int num_connect_attempts_;
    CthunClient::Schema response_schema_;
    CthunClient::Schema error_schema_;
    std::unique_ptr<CthunClient::Connector> connector_ptr_;

    void processResponse(const CthunClient::ParsedChunks& parsed_chunks);
    void processError(const CthunClient::ParsedChunks& parsed_chunks);
};

Controller::Controller()
    try
        : num_connect_attempts_ { 2 },
          response_schema_ { getResponseMessageSchema() },
          error_schema_ { getErrorMessageSchema() },
          connector_ptr_ { new CthunClient::Connector { SERVER_URL,
                                                        CONTROLLER_CLIENT_TYPE,
                                                        CA,
                                                        CERT,
                                                        KEY } } {
    // Connector::registerMessageCallback()

    connector_ptr_->registerMessageCallback(
        response_schema_,
        [this](const CthunClient::ParsedChunks& parsed_chunks) {
            processResponse(parsed_chunks);
        });

    connector_ptr_->registerMessageCallback(
        error_schema_,
        [this](const CthunClient::ParsedChunks& parsed_chunks) {
            processError(parsed_chunks);
        });
} catch (CthunClient::connection_config_error& e) {
    std::string err_msg { "failed to configure the Cthun Connector: " };
    throw controller_error { err_msg + e.what() };
}

void Controller::sendRequests() {
    // Connector::connect()

    try {
        connector_ptr_->connect(num_connect_attempts_);
    } catch (CthunClient::connection_config_error& e) {
        std::string err_msg { "failed to configure WebSocket: " };
        throw controller_error { err_msg + e.what() };
    } catch (CthunClient::connection_fatal_error& e) {
        std::string err_msg { "failed to connect to " + SERVER_URL + " after "
                              + std::to_string(num_connect_attempts_)
                              + "attempts: " };
        throw controller_error { err_msg + e.what() };
    }

    // Connector::isConnected()

    if (connector_ptr_->isConnected()) {
        std::cout << "Successfully connected to " << SERVER_URL << "\n";
    } else {
        // The connection has dropped; we can't send anything
        throw controller_error { "connection dropped" };
    }

    // Send a valid request - a response is expected

    CthunClient::DataContainer track { "{\"artist\" : \"Captain Beefheart\"}" };
    CthunClient::DataContainer data_entries {};
    data_entries.set<CthunClient::DataContainer>("request", track);
    data_entries.set<std::string>("details", "please send some good music");

    std::vector<std::string> endpoints { "cth://*/" + AGENT_CLIENT_TYPE };

    try {
        connector_ptr_->send(endpoints,
                             REQUEST_SCHEMA_NAME,
                             MSG_TIMEOUT_S,
                             data_entries);
        std::cout << "Valid request message sent\n";
    } catch (CthunClient::connection_processing_error& e) {
        std::string err_msg { "failed to send the request message: " };
        throw controller_error { err_msg + e.what() };
    }

    // Send an invalid request - an error message should arrive

    CthunClient::DataContainer bad_json { "{\"genre\" : \"experimental rock\"}" };
    CthunClient::DataContainer bad_data_entries {};
    bad_data_entries.set<CthunClient::DataContainer>("request", bad_json);
    bad_data_entries.set<std::string>("details", "I'm not sure about this");

    try {
        connector_ptr_->send(endpoints,
                             REQUEST_SCHEMA_NAME,
                             MSG_TIMEOUT_S,
                             bad_data_entries);
        std::cout << "Bad request message sent\n";
    } catch (CthunClient::connection_processing_error& e) {
        std::string err_msg { "failed to send the request message: " };
        throw controller_error { err_msg + e.what() };
    }

    // Wait for the response and error messages
    sleep(2);
}

void Controller::processResponse(const CthunClient::ParsedChunks& parsed_chunks) {
    auto response_id = parsed_chunks.envelope.get<std::string>("id");
    auto agent_endpoint = parsed_chunks.envelope.get<std::string>("sender");

    std::cout << "Received response " << response_id
              << " from " << agent_endpoint << ":\n  "
              << parsed_chunks.data.get<std::string>("response") << "\n";
}

void Controller::processError(const CthunClient::ParsedChunks& parsed_chunks) {
    auto response_id = parsed_chunks.envelope.get<std::string>("id");
    auto agent_endpoint = parsed_chunks.envelope.get<std::string>("sender");

    std::cout << "Received error " << response_id
              << " from " << agent_endpoint;

    if (parsed_chunks.data.includes("id")) {
        std::cout << " for request "
                  << parsed_chunks.data.get<std::string>("id");
    }

    std::cout << ":\n  "
              << parsed_chunks.data.get<std::string>("message") << "\n";
}

//
// main
//

int main(int argc, char *argv[]) {
    try {
        Controller controller {};

        try {
            controller.sendRequests();
        } catch (controller_error& e) {
            std::cout << "Failed to process requests: " << e.what() << std::endl;
            return 2;
        }
    } catch (controller_error& e) {
        std::cout << "Failed to initialize the controller: " << e.what()
                  << std::endl;
        return 1;
    }

    return 0;
}

}  // namespace Tutorial

int main(int argc, char** argv) {
    return Tutorial::main(argc, argv);
}
