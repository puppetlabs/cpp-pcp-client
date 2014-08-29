#ifndef CTHUN_CLIENT_SRC_MACROS_H_
#define CTHUN_CLIENT_SRC_MACROS_H_

// We need this hack to avoid the compilation error described in
// https://github.com/zaphoyd/websocketpp/issues/365
#define _WEBSOCKETPP_NULLPTR_TOKEN_ 0

// TODO(ale): check if we really need all C++11 macros

// See http://www.zaphoyd.com/websocketpp/manual/reference/cpp11-support
// for prepocessor directives to choose between boost and cpp11
// C++11 STL tokens
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_MEMORY_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_THREAD_

#endif  // CTHUN_CLIENT_SRC_MACROS_H_
