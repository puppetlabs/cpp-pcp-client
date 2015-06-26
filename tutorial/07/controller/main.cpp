#include "../common.h"

#include <cthun-client/connector/connector.hpp>  // Connector
#include <cthun-client/connector/errors.hpp>     // connection_config_error

#include  <leatherman/json_container/json_container.hpp>  // JsonContainer

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
    void sendRequest();

  private:
    int num_connect_attempts_;
    CthunClient::Schema response_schema_;
    std::unique_ptr<CthunClient::Connector> connector_ptr_;

    void processResponse(const CthunClient::ParsedChunks& parsed_chunks);
};

Controller::Controller()
    try
        : num_connect_attempts_ { 2 },
          response_schema_ { getResponseMessageSchema() },
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
} catch (CthunClient::connection_config_error& e) {
    std::string err_msg { "failed to configure the Cthun Connector: " };
    throw controller_error { err_msg + e.what() };
}

void Controller::sendRequest() {
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

    // Connector::send() - specify message fields; use JsonContainer

    leatherman::json_container::JsonContainer track {
        "{\"artist\" : \"Captain Beefheart\"}" };
    leatherman::json_container::JsonContainer data_entries {};
    data_entries.set<leatherman::json_container::JsonContainer>("request", track);
    data_entries.set<std::string>("details", "please send some good music");

    std::vector<std::string> endpoints { "cth://*/" + AGENT_CLIENT_TYPE };

    try {
        connector_ptr_->send(endpoints,
                             REQUEST_SCHEMA_NAME,
                             MSG_TIMEOUT_S,
                             data_entries);
        std::cout << "Request message sent\n";
    } catch (CthunClient::connection_processing_error& e) {
        std::string err_msg { "failed to send the request message: " };
        throw controller_error { err_msg + e.what() };
    }

    // Wait for the response
    sleep(2);
}

void Controller::processResponse(const CthunClient::ParsedChunks& parsed_chunks) {
    auto response_id = parsed_chunks.envelope.get<std::string>("id");
    auto agent_endpoint = parsed_chunks.envelope.get<std::string>("sender");

    std::cout << "Received response " << response_id
              << " from " << agent_endpoint << ":\n"
              << parsed_chunks.data.get<std::string>("response") << "\n";
}

//
// main
//

int main(int argc, char *argv[]) {
    try {
        Controller controller {};

        try {
            controller.sendRequest();
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
