#include "client.h"
#include "connection_metadata.h"

#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>

namespace Cthun {

static const std::string URL { "ws://127.0.0.1:8080/cthun/" };

int main(int argc, char* argv[]) {
    std::cout << "### I'm inside main()!\n";
    std::string message { "### Message payload (synchronous) ###" };

    // Create client
    Client::TestClient the_client {};

    try {
        // Connect to server
        Client::Connection_ID id = the_client.connect(URL);

        std::cout << "### We're connected!\n";

        sleep(2);

        // Send a message
        the_client.send(id, message);

        std::cout << "### Message sent (SYNCHRONOUS - MAIN THREAD)\n";

        // Close connections
        the_client.closeAllConnections();
    } catch(Client::client_error& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    return 0;
}

}  // namespace Cthun


int main(int argc, char** argv) {
    return Cthun::main(argc, argv);
}
