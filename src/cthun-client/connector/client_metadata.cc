#include "./client_metadata.h"
#include "./errors.h"

#define LEATHERMAN_LOGGING_NAMESPACE CTHUN_CLIENT_LOGGING_PREFIX".client_metadata"

#include <leatherman/logging/logging.hpp>

#include <openssl/x509v3.h>
#include <openssl/ssl.h>

#include <stdio.h>  // std::fopen

namespace CthunClient {

// Get rid of OSX 10.7 and greater deprecation warnings.
#if defined(__APPLE__) && defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

std::string getClientIdentityFromCert(const std::string& client_crt_path,
                                      const std::string& type) {
    LOG_INFO("Retrieving the client identity from %1%", client_crt_path);

    std::unique_ptr<std::FILE, int(*)(std::FILE*)> fp {
        std::fopen(client_crt_path.data(), "r"), std::fclose };

    if (fp == nullptr) {
        throw connection_config_error { "certificate file '" + client_crt_path
                                        + "' does not exist" };
    }

    std::unique_ptr<X509, void(*)(X509*)> cert {
        PEM_read_X509(fp.get(), NULL, NULL, NULL), X509_free };

    if (cert == nullptr) {
        throw connection_config_error { "certificate file '" + client_crt_path
                                        + "' is invalid" };
    }

    X509_NAME* subj = X509_get_subject_name(cert.get());
    X509_NAME_ENTRY* name_entry = X509_NAME_get_entry(subj, 0);

    if (name_entry == nullptr) {
        throw connection_config_error { "failed to retrieve the client name "
                                        "from " + client_crt_path };
    }

    ASN1_STRING* asn1_name = X509_NAME_ENTRY_get_data(name_entry);
    unsigned char* name_ptr = ASN1_STRING_data(asn1_name);
    int name_size = ASN1_STRING_length(asn1_name);

    return "cth://" + std::string(name_ptr, name_ptr + name_size) + "/" + type;
}

#if defined(__APPLE__) && defined(__clang__)
#pragma clang diagnostic pop
#endif

ClientMetadata::ClientMetadata(const std::string& _type,
                               const std::string& _ca,
                               const std::string& _crt,
                               const std::string& _key)
        : type { _type },
          ca { _ca },
          crt { _crt },
          key { _key },
          id { getClientIdentityFromCert(_crt, _type) } {
    LOG_INFO("Obtained the client identity from %1%: %2%", _crt, id);
}

}  // namespace CthunClient
