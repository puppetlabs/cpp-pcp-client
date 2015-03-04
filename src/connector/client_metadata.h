#ifndef CTHUN_CLIENT_SRC_CONNECTOR_CLIENT_METADATA_H_
#define CTHUN_CLIENT_SRC_CONNECTOR_CLIENT_METADATA_H_

#include "./errors.h"

#include <openssl/X509v3.h>
#include <openssl/ssl.h>

#include <string>
#include <stdio.h>  // std::fopen

namespace CthunClient {

//
// Utility functions
//

// Get rid of OSX 10.7 and greater deprecation warnings.
#if defined(__APPLE__) && defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

std::string getClientIdentityFromCert(const std::string& client_crt_path) {
    std::unique_ptr<std::FILE, int(*)(std::FILE*)> fp {
        std::fopen(client_crt_path.data(), "r"), std::fclose };
    if (fp == nullptr) {
        throw connection_config_error { "certificate file '" + client_crt_path
                                       + "' does not exist." };
    }
    std::unique_ptr<X509, void(*)(X509*)> cert {
        PEM_read_X509(fp.get(), NULL, NULL, NULL), X509_free };
    X509_NAME* subj = X509_get_subject_name(cert.get());
    X509_NAME_ENTRY* entry = X509_NAME_get_entry(subj, 0);
    ASN1_STRING* asn1_string = X509_NAME_ENTRY_get_data(entry);
    unsigned char* tmp = ASN1_STRING_data(asn1_string);
    int string_size = ASN1_STRING_length(asn1_string);
    return "cth://" + std::string(tmp, tmp + string_size) + "/cthun-agent";
}

#if defined(__APPLE__) && defined(__clang__)
#pragma clang diagnostic pop
#endif

//
// ClientMetadata
//

struct ClientMetadata {
    std::string type;
    std::string ca;
    std::string crt;
    std::string key;
    std::string id;

    ClientMetadata(const std::string& _type,
                   const std::string& _ca,
                   const std::string& _crt,
                   const std::string& _key)
            : type { _type },
              ca { _ca },
              crt { _crt },
              key { _key },
              id { getClientIdentityFromCert(_crt) } {
        // TODO(ale): log the retrieved id at debug level
    }
};

}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_CONNECTOR_CLIENT_METADATA_H_
