#include "client.h"
#include "test_client.h"
#include "connection_metadata.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>

namespace Cthun {

static const int MAX_WAIT_INTERVALS { 5 };

void showHelp() {
    std::cout << "Invalid command line arguments.\n"
              << "Usage: ctunn-client [url] [num connections] "
              << "{message} {message} ... \n";
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

    // Create client
    Client::TestClient the_client { messages };

    // Set the connections
    std::vector<Client::Connection_ID> connection_ids;
    for (auto i = 0; i < num_connections; i++) {
        try {
            // Connect to server
            Client::Connection_ID id = the_client.connect(url);
            connection_ids.push_back(id);
        } catch(Client::client_error& e) {
            std::cout << "### ERROR (connecting): " << e.what() << std::endl;
            return 1;
        }
    }

    // Sleep to allow sending async messages
    sleep(6);

    // Send messages
    try {
        for (auto id : connection_ids) {
            int wait_intervals { 0 };
            while (wait_intervals < MAX_WAIT_INTERVALS) {
                if (the_client.getStateOf(id) == websocketpp::session::state::open) {
                    std::cout << "### We're connected!\n";
                    std::string sync_message { "### Message payload (SYNC) "
                                               " on connection " + id };
                    the_client.send(id, sync_message);
                    std::cout << "### Message sent (SYNCHRONOUS - MAIN THREAD) "
                              << "on connection " << id << "\n";
                    break;
                }
                wait_intervals++;
                sleep(1);
            }
        }
    } catch(Client::client_error& e) {
        std::cout << "### ERROR (sending): " << e.what() << std::endl;
        return 1;
    }

    // Close connections synchronously
    try {
        // NB: this is done by the destructor
        sleep(6);
        std::cout << "### ### ###\n"
                  << "### Done sending; about to close all connections\n";
        the_client.closeAllConnections();
    } catch(Client::client_error& e) {
        std::cout << "### ERROR (closing): " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

}  // namespace Cthun


int main(int argc, char** argv) {
    return Cthun::main(argc, argv);
}
