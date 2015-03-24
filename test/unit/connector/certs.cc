#include "test/test.h"
#include "test/unit/connector/certs.h"

std::string getCaPath() {
    static const std::string ca {
        ROOT_PATH + "/test/resources/ca_crt.pem" };
    return ca;
}

std::string getCertPath() {
    static const std::string cert {
        ROOT_PATH + "/test/resources/test_crt.pem" };
    return cert;
}

std::string getKeyPath() {
    static const std::string key {
        ROOT_PATH + "/test/resources/test_key.pem" };
    return key;
}

std::string getNotACertPath() {
    static const std::string tmp {
        ROOT_PATH + "/test/resources/not_a_cert" };
    return tmp;
}

std::string getNotExistentFilePath() {
    static const std::string tmp {
        ROOT_PATH + "/test/resources/nothing.here" };
    return tmp;
}
