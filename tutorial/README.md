## Tutorial

This tutorial shows how to create a Cthun agent / controller pair with
CthunClient.

To build on OS X:
`g++ -std=c++11 -o agent -L /usr/local/lib  -lcthun-client -I ../../../src  main.cpp`

You need to install CthunClient before that: `make` then `make install`; the
`libcthun-client.so` file should be then in /usr/local/lib, otherwise you must
modify the above -L option argument appropriately.

To test the agent / controller pair you will need a running Cthun server.
Please refer to the [Cthun server repo](https://github.com/puppetlabs/cthun) for
setting the required SSL certificates infrastructure; the certificates located
in the tutorial/resources directory will not work out of the box.

### Step 1

Create an instance of CthunClient::Connector that will manage the connection.

Note that the paths to the certificate files are hardcoded.

### Step 2

 - Connect to the specified server; the server url is hardcoded.
 The Connector::connect() method will establish the underlying transport
 connection (WebSockets) and login into the Cthun server.
 - Ensure that the connection is up by calling Connector::isConnected();
 - Enable the connection persistence by starting the monitoring task.
 Connector::monitorConnection() will periodically check the connection and,
 depending on its state, send a keepalive message to the server or reconnect.

Note that the connector will attempt to connect or reconnect 4 times at most.

### Step 3

Add exception handling.

### Step 4

 - Refactor the connection logic to the new Agent class.
 - Define the request message schema with CthunClient::Schema.
 - Register a callback (processRequest) that will be executed when request
 messages are received; the callback will simply log a message event. The
 registration is done by calling Connector::registerMessageCallback().

More in detail, calling Connector::registerMessageCallback() ensures that all
received messages with a `data_schema` entry equals to `"tutorial_request_schema"`
(which is the name we gave to the request Schema object) are processed by
Agent::processRequest; `data_schema` is  required entry of envelope chunks
(refer to the Cthun specs).

### Step 5

 - Create a controller, similar to the agent. At this point, two different
 executables should be built.
 - The Controller connects to server, sends a request to the Agent, and logs the
 received responses messages. The request is sent by specifying the message
 entries; the connector will then take care of creating message chunks.

Note that Connector::send has different overloads.

### Step 6

Create a Controller class and refactor common code.

### Step 7

 - Define the response message schema.
 - For each incoming request message, the Agent::processRequest callback will
 send a response message back to the requester endpoint.
 - Register a Controller::processResponse callback that will notify received
 responses.

 Note that the controller will simply wait for a predifined time interval; no
 concurrency logic will be employed to manage the asynchronous callbacks.

### Step 8

 - Define the error message schema.
 - For each incoming request message, the Agent::processRequest callback will
 validate the content of the request by using CthunClient::Validator.
 - In case the request content is invalid, the agent will respond to the
 controller with an error message.
 - Add a callback to the Controller to process error messages.
