#include "tests/test.hpp"
#include "tests/unit/connector/certs.hpp"

std::string getCaPath() {
    static const std::string ca {
        std::string { CPP_PCP_CLIENT_ROOT_PATH } + "/lib/tests/resources/ca_crt.pem" };
    return ca;
}

std::string getCertPath() {
    static const std::string cert {
        std::string { CPP_PCP_CLIENT_ROOT_PATH } + "/lib/tests/resources/test_crt.pem" };
    return cert;
}

std::string getKeyPath() {
    static const std::string key {
        std::string { CPP_PCP_CLIENT_ROOT_PATH } + "/lib/tests/resources/test_key.pem" };
    return key;
}

std::string getNotACertPath() {
    static const std::string tmp {
        std::string { CPP_PCP_CLIENT_ROOT_PATH } + "/lib/tests/resources/not_a_cert" };
    return tmp;
}

std::string getNotExistentFilePath() {
    static const std::string tmp {
        std::string { CPP_PCP_CLIENT_ROOT_PATH } + "/lib/tests/resources/nothing.here" };
    return tmp;
}
