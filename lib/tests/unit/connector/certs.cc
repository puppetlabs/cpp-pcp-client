#include "tests/test.hpp"
#include "tests/unit/connector/certs.hpp"

std::string getCertname() {
  static const std::string certname { "good" };
  return certname;
}

std::string getCaPath() {
    static const std::string ca {
        std::string { CPP_PCP_CLIENT_ROOT_PATH } + "/lib/tests/resources/ssl-new/ca.pem" };
    return ca;
}

std::string getCertPath() {
    static const std::string cert {
        std::string { CPP_PCP_CLIENT_ROOT_PATH } + "/lib/tests/resources/ssl-new/good_cert.pem" };
    return cert;
}

std::string getKeyPath() {
    static const std::string key {
        std::string { CPP_PCP_CLIENT_ROOT_PATH } + "/lib/tests/resources/ssl-new/good_key.pem" };
    return key;
}

std::string getMismatchedKeyPath() {
    static const std::string key {
        std::string { CPP_PCP_CLIENT_ROOT_PATH } + "/lib/tests/resources/mismatched_key.pem" };
    return key;
}

std::string getProtectedKeyPath() {
    static const std::string key {
        std::string { CPP_PCP_CLIENT_ROOT_PATH } + "/lib/tests/resources/protected_key.pem" };
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

std::string getBadCertPath() {
    static const std::string cert {
        std::string { CPP_PCP_CLIENT_ROOT_PATH } + "/lib/tests/resources/ssl-new/bad_cert.pem" };
    return cert;
}

std::string getBadKeyPath() {
    static const std::string key {
        std::string { CPP_PCP_CLIENT_ROOT_PATH } + "/lib/tests/resources/ssl-new/bad_key.pem" };
    return key;
}

std::string getEmptyCrlPath() {
    static const std::string crl {
        std::string { CPP_PCP_CLIENT_ROOT_PATH } + "/lib/tests/resources/ssl-new/crl_empty.pem" };
    return crl;
}

std::string getRevokedCrlPath() {
    static const std::string crl {
        std::string { CPP_PCP_CLIENT_ROOT_PATH } + "/lib/tests/resources/ssl-new/crl_bad_revoked.pem" };
    return crl;
}
