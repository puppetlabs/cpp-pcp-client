## Tutorial

*TODO(ale):* update urls

This tutorial shows how to create a PCP agent / controller pair with
cpp-pcp-client.

To build on OS X:
```
    g++ -std=c++11 -o agent -L /usr/local/lib -lcpp-pcp-client \
    -lleatherman_json_container -I ../../../lib/inc main.cpp
```

You need to install PCP and [leatherman][1] before that: `make` then
`sudo make install`; `libcpp-pcp-client.so` and `libleatherman_json_container.a`
should be then in /usr/local/lib, otherwise you must modify the above -L option
argument appropriately.

To test the agent / controller pair you will need a running PCP server.
Please refer to the [PCP server repo][2] for setting the required SSL
certificates infrastructure; the certificates located in the tutorial/resources
directory will not work out of the box.

### Step 1

Create an instance of PCPClient::Connector that will manage the connection.

Note that the paths to the certificate files are hardcoded.

### Step 2

 - Connect to the specified server; the server url is hardcoded.
 The Connector::connect() method will establish the underlying transport
 connection (WebSockets) and login into the PCP server.
 - Ensure that the connection is up by calling Connector::isConnected();
 - Enable the connection persistence by starting the monitoring task.
 Connector::monitorConnection() will periodically check the connection and,
 depending on its state, send a keepalive message to the server or reconnect.

Note that the connector will attempt to connect or reconnect 4 times at most.

### Step 3

Add exception handling.

### Step 4

 - Refactor the connection logic to the new Agent class.
 - Define the request message schema with PCPClient::Schema.
 - Register a callback (processRequest) that will be executed when request
 messages are received; the callback will simply log a message event. The
 registration is done by calling Connector::registerMessageCallback().

More in detail, calling Connector::registerMessageCallback() ensures that all
received messages with a `data_schema` entry equals to `"tutorial_request_schema"`
(which is the name we gave to the request Schema object) are processed by
Agent::processRequest; `data_schema` is  required entry of envelope chunks
(refer to the [PCP specs][3].

### Step 5

 - Create a controller, similar to the agent. At this point, two different
 executables should be built.
 - The Controller 1) connects to server, 2) sends a request to the Agent, and
 3) logs the received responses messages. The request is sent by simply
 specifying the message entries; the connector will then take care of creating
 message chunks.

Note that Connector::send has different overloads.

### Step 6

Create a Controller class and refactor common code.

### Step 7

 - Define the response message schema.
 - For each incoming request message, the Agent::processRequest callback will
 send a response message back to the requester endpoint.
 - Register a Controller::processResponse callback that will notify received
 responses.

 Note that the controller will simply wait for a predefined time interval; no
 concurrency logic will be employed to manage the asynchronous callbacks.

### Step 8

 - Use the error message schema defined in protocol/schemas.
 - For each incoming request message, the Agent::processRequest callback will
 validate the content of the request by using PCPClient::Validator.
 - In case the request content is invalid, the agent will respond to the
 controller with an error message.
 - Add a callback to the Controller to process error messages.

### Step 9

The controller retrieves the list of the agent nodes connected to the message
fabric (inventory) by relying on the associate response sent by the server. This
is done by 1) registering an associate response callback for processing and
displaying the inventory list and 2) sending an inventory request to the server.

[1]: https://github.com/puppetlabs/leatherman
[2]: https://github.com/puppetlabs/cthun
[3]: https://github.com/puppetlabs/cthun-specifications
