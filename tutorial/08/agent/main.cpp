#include "../common.h"

#include <cthun-client/connector/connector.hpp>  // Connector
#include <cthun-client/connector/errors.hpp>     // connection_config_error

#include <cthun-client/message/chunks.hpp>       // ParsedChunk

#include <cthun-client/validator/schema.hpp>     // Schema, ContentType
#include <cthun-client/validator/validator.hpp>  // Validator

#include <string>
#include <iostream>
#include <memory>  // unique_ptr
#include <vector>
#include <stdexcept>

namespace Tutorial {

const std::string CA   { "../../resources/agent_certs/ca.pem" };
const std::string CERT { "../../resources/agent_certs/crt.pem" };
const std::string KEY  { "../../resources/agent_certs/key.pem" };

const std::string SONG_SCHEMA { "song schema" };

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
    CthunClient::Schema request_schema_;
    CthunClient::Validator validator_;
    std::unique_ptr<CthunClient::Connector> connector_ptr_;

    void processRequest(const CthunClient::ParsedChunks& parsed_chunks);
};

Agent::Agent()
    try
        : num_connect_attempts_ { 4 },
          request_schema_ { getRequestMessageSchema() },
          connector_ptr_ { new CthunClient::Connector { SERVER_URL,
                                                        AGENT_CLIENT_TYPE,
                                                        CA,
                                                        CERT,
                                                        KEY } } {
    // Validator::registerSchema()
    CthunClient::Schema song_schema { SONG_SCHEMA,
                                      CthunClient::ContentType::Json };
    song_schema.addConstraint("artist", T_C::String, true);  // mandatory
    song_schema.addConstraint("album", T_C::String, false);  // optional
    song_schema.addConstraint("year", T_C::String, false);   // optional

    validator_.registerSchema(song_schema);
} catch (CthunClient::connection_config_error& e) {
    std::string err_msg { "failed to configure the Cthun Connector: " };
    throw agent_error { err_msg + e.what() };
}

void Agent::start() {
    // Connector::registerMessageCallback()

    connector_ptr_->registerMessageCallback(
        request_schema_,
        [this](const CthunClient::ParsedChunks& parsed_chunks) {
            processRequest(parsed_chunks);
        });

    // Connector::connect()

    try {
        connector_ptr_->connect(num_connect_attempts_);
    } catch (CthunClient::connection_config_error& e) {
        std::string err_msg { "failed to configure WebSocket: " };
        throw agent_error { err_msg + e.what() };
    } catch (CthunClient::connection_fatal_error& e) {
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
    } catch (CthunClient::connection_fatal_error& e) {
        std::string err_msg { "failed to reconnect to " + SERVER_URL + ": " };
        throw agent_error { err_msg + e.what() };
    }
}

void Agent::processRequest(const CthunClient::ParsedChunks& parsed_chunks) {
    auto request_id = parsed_chunks.envelope.get<std::string>("id");
    auto requester_endpoint = parsed_chunks.envelope.get<std::string>("sender");
    auto request = parsed_chunks.data.get<CthunClient::DataContainer>("request");

    std::cout << "Received message " << request_id
              << " from " << requester_endpoint << ":\n"
              << parsed_chunks.toString() << "\n";

    // Validator::validate()

    try {
        validator_.validate(request, SONG_SCHEMA);
    } catch (CthunClient::validation_error& e) {
        std::string err { "Failed to validate request: " };
        err += e.what();
        std::cout << err << "\n";
        CthunClient::DataContainer err_data {};
        err_data.set<std::string>("message", err);
        err_data.set<std::string>("id", request_id);

        try {
            connector_ptr_->send(std::vector<std::string> { requester_endpoint },
                                 ERROR_SCHEMA_NAME,
                                 MSG_TIMEOUT_S,
                                 err_data);
            std::cout << "Error message sent to " << requester_endpoint << "\n";
        } catch (CthunClient::connection_processing_error& e) {
            std::cout << "Failed to send the error message: "
                      << e.what() << "\n";
            // NB: as below, we don't want to throw anything here; we
            // must not stop the event handler thread
        }

        return;
    }

    // Prepare data chunk

    std::string response_txt;

    if (request.includes("artist")) {
        auto artist = request.get<std::string>("artist");
        if (artist == "Captain Beefheart") {
            response_txt = "as requested: http://goo.gl/7RWu4G";
        } else {
            response_txt = "unknown artist - get some of: http://goo.gl/0DWhp6";
        }
    }

    if (parsed_chunks.data.includes("details")) {
        auto details_txt = parsed_chunks.data.get<std::string>("details");
        response_txt += std::string { " (details: \"" + details_txt + "\")" };
    }

    CthunClient::DataContainer response_data {};
    response_data.set<std::string>("response", response_txt);

    // Prepare debug chunk
    std::vector<CthunClient::DataContainer> response_debug {};
    for (auto& debug_txt : parsed_chunks.debug) {
        CthunClient::DataContainer debug_entry {};
        response_debug.push_back(debug_entry);
    }

    // Connector::send()

    try {
        connector_ptr_->send(std::vector<std::string> { requester_endpoint },
                             RESPONSE_SCHEMA_NAME,
                             MSG_TIMEOUT_S,
                             response_data,
                             response_debug);
        std::cout << "Response message sent to " << requester_endpoint << "\n";
    } catch (CthunClient::connection_processing_error& e) {
        std::cout << "Failed to send the response message: "
                  << e.what() << "\n";

        // NB: as above, we don't want to throw anything here
    }
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
