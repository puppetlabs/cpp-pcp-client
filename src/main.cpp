#include "client.h"
#include "connection_metadata.h"

#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>

namespace Cthun_Test_Client {

// static const int NUM_CONNECTIONS { 10 };

static const std::string URL { "ws://127.0.0.1:8080/cthun/" };


int main(int argc, char* argv[]) {
    std::cout << "### I'm inside main()!\n";
    std::string message { "### Message payload ###" };

    // Create client
    Client the_client {};

    // Connect
    websocketpp::connection_hdl hdl;
    try {
         hdl = the_client.connect(URL);
    } catch(connection_error& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    std::cout << "### We're connected!\n";

    sleep(2);

    // Send message
    try {
        the_client.send(hdl, message);
    } catch(message_error& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    std::cout << "### Message sent!\n";

    the_client.join_the_thread();

    return 0;
}

}  // namespace Cthun_Test_Client


int main(int argc, char** argv) {
    return Cthun_Test_Client::main(argc, argv);
}
