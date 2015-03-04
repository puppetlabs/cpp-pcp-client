# cthun_client

The cthun_client contains common C++ code used by the Cthun client components.

## Requirements
 - a C++11 compiler (clang/gcc 4.7)
 - gnumake
 - cmake
 - boost

`cthun-client` uses the following libraries, which are stored in the vendor directory:
 - [websocketpp](https://github.com/zaphoyd/websocketpp)
 - [rapidjson](https://github.com/miloyip/rapidjson)
 - [valijson](https://github.com/tristanpenman/valijson)

## Usage

### Import

TODO

### Test

Just build `cthun-client` with make; unit tests will be then executed. `cthun-client` uses [catch](https://github.com/philsquared/Catch) as its test framework.

## Libraries

### connector

The `connector` library provides an interface to establish and manage connections towards a Cthun server. The underlying communications layer is based on WebSocket.
For a given Connector instance, the user can set two different callback functions that will be executed, respectively, after sending the login message and when a message is received.
Once a client node is logged into the Cthun server, the `monitorConnectionState` method ensures that the WebSocket connection is not dropped for inactivity; this is done by sending ping messages.

### data_container

The `data_container` library is a JSON based storage class.

### message

The `message` library implements the Cthun message format, by offering serialization and parsing functionalities.

### validator

The `validator` library implements the logic to validate a number of different JSON objects defined in the Cthun environment, such as message content and input / output of actions.

## Scripts

#### FindDependency.cmake

#### cthun_test.py

#### travis scripts
